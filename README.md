# RTOS Scheduler Simulator

A real-time task scheduling simulator, written in modern C++17. Originally
an Arduino sketch that only implemented Earliest Deadline First; rebuilt as
a portable C++ library + CLI + web visualizer with five scheduling
algorithms, so it runs anywhere (Linux/macOS/Windows, or in CI) instead of
requiring physical hardware or the Wokwi simulator.

## Demo

https://rtos-scheduler.onrender.com/

## Why

Real-time scheduling theory (EDF, RMS, LLF, ...) is usually taught with
pen-and-paper Gantt charts. This project turns those algorithms into a
runnable simulator: define a task set once, run it under any of five
algorithms, and compare CPU utilization, missed deadlines, context
switches, and response times side by side -- with a Gantt-chart web viewer
to make the schedules visually inspectable.

## Algorithms implemented

| Algorithm | Type | Notes |
|---|---|---|
| **EDF** -- Earliest Deadline First | Dynamic priority | Optimal on a uniprocessor: schedulable whenever *any* algorithm can meet all deadlines. |
| **FPS** -- Preemptive Fixed-Priority Scheduling | Static priority | Priorities are user-assigned per task. |
| **RMS** -- Rate-Monotonic Scheduling | Static priority | A special case of FPS where priority is derived automatically from period (shorter period = higher priority). Optimal among static-priority algorithms. |
| **LLF** -- Least Laxity First | Dynamic priority | Runs the job with the least slack (`deadline - now - remaining`); more reactive than EDF, but can thrash when laxities tie. |
| **Round-Robin** with time slicing | Not deadline-aware | Classic fairness-oriented baseline for contrast; included to show *why* deadline-aware algorithms matter. |

## Project layout

```
include/scheduler/         Public headers (Task, Scheduler interface, Simulator, ...)
  algorithms/               One header per algorithm (EDF, FPS, RMS, LLF, Round-Robin)
src/                        Implementation (Simulator, TaskLoader, JsonExport, Report)
cli/main.cpp                Command-line entry point
server/                     Node.js / Express API server
  index.js                   POST /api/simulate — runs the binary, returns JSON
  package.json
  .env.example               PORT and BINARY_PATH env vars
tests/basic_test.cpp        Smoke tests (wired into `ctest`)
examples/tasks.csv          Sample task set
web/                        Static Gantt-chart / metrics viewer (HTML/CSS/JS)
Dockerfile                  Multi-stage: gcc build stage + node runtime stage
docker-compose.yml          One-command local dev
```

## Building (C++ CLI only)

Requires a C++17 compiler and CMake >= 3.15.

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
ctest            # optional: run the smoke tests
```

This produces `build/rtsched` (CLI) and `build/rtsched_tests`.

## Running the CLI

```bash
./rtsched --tasks examples/tasks.csv --algo all --out web/results.json
```

```
Usage:
  rtsched [--tasks <file.csv>] [--algo <name>] [--quantum <n>]
           [--horizon <n>] [--out <file.json>]

Options:
  --tasks <file>   CSV task file (id,name,arrival,exec,period,deadline,priority).
                   Defaults to a built-in 3-task example if omitted.
  --algo <name>    edf | fps | rms | llf | rr | all   (default: edf)
  --quantum <n>    Time slice for Round-Robin (default: 2)
  --horizon <n>    Override simulation length in ticks (default: hyperperiod)
  --out <file>     Write JSON results for the web viewer (default: web/results.json)
  --list           List available algorithms and exit
  --help           Show this message
```

`--algo all` runs every algorithm on the same task set and prints a
comparison table -- useful for showing, e.g., that Round-Robin can miss
deadlines that EDF/FPS/RMS/LLF meet on an otherwise-schedulable task set.

### Task file format

```csv
# id,name,arrival,exec,period,deadline,priority
0,SensorPoll,0,1,4,4,1
1,ControlLoop,0,2,5,5,0
2,Telemetry,0,2,10,10,2
3,Housekeeping,0,1,20,20,3
```

- `period` <= 0 marks a one-shot (aperiodic) task -- it is released once at
  `arrival` and never again.
- `priority` is only consulted by FPS (lower number = higher priority) and
  is optional; if omitted it defaults to the row's position in the file.
- Comment lines (`#`) and a header row are both fine and are skipped
  automatically.

---

## Running as a web app

The Node.js server accepts CSV uploads, spawns the C++ binary, and serves
the Gantt viewer on a single port -- no separate static hosting needed.

### Option A — Docker Compose (recommended for local dev)

```bash
docker compose up --build
# Open http://localhost:3000
```

On first run Docker compiles the C++ binary inside the image automatically.
Subsequent `up` calls reuse the cached image unless source files change.

### Option B — Without Docker (local dev)

Terminal 1 — build the binary:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j rtsched
```

Terminal 2 — start the server:
```bash
cd server
npm install
BINARY_PATH=../build/rtsched node index.js
# Open http://localhost:3000
```

On Windows replace `BINARY_PATH=...` with:
```powershell
$env:BINARY_PATH = "..\build\rtsched.exe"; node index.js
```

### Environment variables

| Variable | Default | Description |
|---|---|---|
| `PORT` | `3000` | Port the server listens on |
| `BINARY_PATH` | `../build/rtsched` | Absolute or relative path to the compiled binary |

Copy `server/.env.example` to `server/.env` and adjust values for local
development without Docker.

---

## Hosting (Railway / Render / Fly.io)

Because the Dockerfile compiles the C++ binary at image build time,
deploying is exactly the same as deploying any Dockerised Node.js app.

### Railway

1. Push the repo to GitHub.
2. Create a new Railway project → **Deploy from GitHub repo**.
3. Railway auto-detects the `Dockerfile` and builds it.
4. Set `PORT` in Railway's environment variables panel (Railway injects
   `PORT` automatically for web services -- no manual step needed).
5. Done -- Railway provides a public HTTPS URL.

### Render

1. New Web Service → connect GitHub repo.
2. **Environment**: Docker.
3. Set `PORT=3000` in the environment panel.
4. Deploy.

### Fly.io

```bash
fly launch          # detects Dockerfile, prompts for app name / region
fly deploy
fly open
```

---

## API reference

### `POST /api/simulate`

Multipart form fields:

| Field | Type | Required | Description |
|---|---|---|---|
| `file` | `.csv` file | No | Task set CSV. Omit to use the built-in example. |
| `algo` | string | No | `edf` \| `fps` \| `rms` \| `llf` \| `rr` \| `all` (default: `all`) |
| `quantum` | integer | No | Round-Robin time slice (default: 2) |
| `horizon` | integer | No | Override simulation length in ticks |

Returns the same JSON structure that the CLI writes to `--out`, or
`{ "error": "..." }` with an appropriate HTTP status on failure.

### `GET /api/health`

Returns `{ "ok": true, "binary": "<path>" }`. Useful for hosting health checks.

---

## Metrics reported

For each algorithm run:
- **CPU utilization** -- percentage of ticks the processor was busy.
- **Context switches** -- how often the running task changed.
- **Per task**: instances released, missed deadlines, miss rate, average
  response time (time from release to first execution), and total CPU
  time consumed.
- **All deadlines met** -- overall pass/fail for the run.

## Extending it

- **New algorithm**: implement `rts::Scheduler` (see
  `include/scheduler/Scheduler.hpp`) and add it to the `makeScheduler()`
  switch in `cli/main.cpp`. The simulator itself doesn't need to change.
- **New task input format**: add a loader alongside `TaskLoader.cpp` that
  returns `std::vector<rts::TaskConfig>`.

## Origin

This project began as an [Arduino EDF scheduler](https://github.com/annshita/RTOS-Scheduler-using-Arduino)
built and visualized in the Wokwi simulator with an LCD readout. This
version keeps the same core scheduling ideas but drops the hardware
dependency, generalizes the engine to support multiple algorithms, and
replaces the LCD/Wokwi front end with a portable CLI and a hostable web
viewer backed by a Node.js API server.
