"use strict";

const express = require("express");
const multer  = require("multer");
const { execFile } = require("child_process");
const fs   = require("fs");
const path = require("path");
const os   = require("os");
const { v4: uuidv4 } = require("uuid");

// ---------------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------------
const PORT        = parseInt(process.env.PORT || "3000", 10);
// BINARY_PATH can be overridden via env; defaults work for both local dev
// (binary in ../build/rtsched) and inside the Docker image (/usr/local/bin/rtsched).
const BINARY_PATH = process.env.BINARY_PATH || path.resolve(__dirname, "../build/rtsched");
const WEB_DIR     = path.resolve(__dirname, "../web");

// ---------------------------------------------------------------------------
// App
// ---------------------------------------------------------------------------
const app = express();

// Serve the static web frontend from web/
app.use(express.static(WEB_DIR));

// ---------------------------------------------------------------------------
// File upload — store incoming CSV in system temp dir, clean up after request
// ---------------------------------------------------------------------------
const storage = multer.diskStorage({
  destination: (_req, _file, cb) => cb(null, os.tmpdir()),
  filename:    (_req, _file, cb) => cb(null, `rtsched-${uuidv4()}.csv`),
});
const upload = multer({
  storage,
  limits: { fileSize: 1 * 1024 * 1024 }, // 1 MB max
  fileFilter: (_req, file, cb) => {
    if (file.mimetype === "text/csv" || file.originalname.endsWith(".csv")) {
      cb(null, true);
    } else {
      cb(new Error("Only .csv files are accepted"));
    }
  },
});

// ---------------------------------------------------------------------------
// Helper — run the C++ binary and return parsed JSON results
// ---------------------------------------------------------------------------
function runSimulator({ csvPath, algo, quantum, horizon }) {
  return new Promise((resolve, reject) => {
    const outPath = path.join(os.tmpdir(), `rtsched-out-${uuidv4()}.json`);

    const args = [
      "--tasks",   csvPath,
      "--algo",    algo || "all",
      "--out",     outPath,
    ];
    if (quantum && parseInt(quantum, 10) > 0) {
      args.push("--quantum", String(parseInt(quantum, 10)));
    }
    if (horizon && parseInt(horizon, 10) > 0) {
      args.push("--horizon", String(parseInt(horizon, 10)));
    }

    execFile(BINARY_PATH, args, { timeout: 15_000 }, (err, _stdout, stderr) => {
      // Always attempt to clean up the output file
      const cleanup = () => {
        try { fs.unlinkSync(outPath); } catch (_) {}
      };

      if (err) {
        cleanup();
        return reject(new Error(stderr || err.message));
      }

      let results;
      try {
        results = JSON.parse(fs.readFileSync(outPath, "utf8"));
      } catch (parseErr) {
        cleanup();
        return reject(new Error("Binary produced invalid JSON: " + parseErr.message));
      }

      cleanup();
      resolve(results);
    });
  });
}

// ---------------------------------------------------------------------------
// POST /api/simulate
// Accepts multipart form data:
//   - file  (optional .csv)  — if omitted, uses the built-in example task set
//   - algo   string          — edf | fps | rms | llf | rr | all  (default: all)
//   - quantum number         — time slice for Round-Robin
//   - horizon number         — override simulation length in ticks
// ---------------------------------------------------------------------------
app.post(
  "/api/simulate",
  upload.single("file"),
  async (req, res) => {
    let csvPath    = null;
    let usedExample = false;

    // If no CSV was uploaded, use the bundled example task set
    if (!req.file) {
      csvPath     = path.resolve(__dirname, "../examples/tasks.csv");
      usedExample = true;
    } else {
      csvPath = req.file.path;
    }

    try {
      const results = await runSimulator({
        csvPath,
        algo:    req.body.algo    || "all",
        quantum: req.body.quantum || "",
        horizon: req.body.horizon || "",
      });

      // Attach a flag so the frontend can show "Using example tasks" notice
      if (usedExample) results._usedExample = true;

      res.json(results);
    } catch (err) {
      res.status(422).json({ error: err.message });
    } finally {
      // Clean up uploaded temp file (not needed for the example path)
      if (req.file) {
        try { fs.unlinkSync(req.file.path); } catch (_) {}
      }
    }
  }
);

// ---------------------------------------------------------------------------
// GET /api/health  — for Railway / Render / Fly.io health checks
// ---------------------------------------------------------------------------
app.get("/api/health", (_req, res) => {
  // Also verify the binary actually exists
  const binaryExists = fs.existsSync(BINARY_PATH);
  if (!binaryExists) {
    return res.status(503).json({ ok: false, error: "Binary not found at: " + BINARY_PATH });
  }
  res.json({ ok: true, binary: BINARY_PATH });
});

// ---------------------------------------------------------------------------
// Fallback — send index.html for any other GET (SPA style)
// ---------------------------------------------------------------------------
app.get("*", (_req, res) => {
  res.sendFile(path.join(WEB_DIR, "index.html"));
});

// ---------------------------------------------------------------------------
// Multer error handler
// ---------------------------------------------------------------------------
// eslint-disable-next-line no-unused-vars
app.use((err, _req, res, _next) => {
  if (err instanceof multer.MulterError || err.message) {
    return res.status(400).json({ error: err.message });
  }
  res.status(500).json({ error: "Internal server error" });
});

// ---------------------------------------------------------------------------
// Start
// ---------------------------------------------------------------------------
app.listen(PORT, () => {
  console.log(`rtsched server listening on http://localhost:${PORT}`);
  console.log(`Binary : ${BINARY_PATH}`);
  console.log(`Web    : ${WEB_DIR}`);
});
