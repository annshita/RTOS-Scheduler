(function () {
  "use strict";

  // ---------------------------------------------------------------------------
  // Palette
  // ---------------------------------------------------------------------------
  const PALETTE = ["#4f8cff","#33c17a","#e0a934","#e0554f","#a970ff","#38c6c6","#ff8fb1","#8bd450"];

  // ---------------------------------------------------------------------------
  // DOM refs — Input panel
  // ---------------------------------------------------------------------------
  const dropzone      = document.getElementById("dropzone");
  const csvInput      = document.getElementById("csvInput");
  const dropzoneHint  = document.getElementById("dropzoneHint");
  const useExampleBtn = document.getElementById("useExampleBtn");
  const algoInput     = document.getElementById("algoInput");
  const quantumInput  = document.getElementById("quantumInput");
  const horizonInput  = document.getElementById("horizonInput");
  const runBtn        = document.getElementById("runBtn");
  const btnText       = runBtn.querySelector(".btn-text");
  const spinner       = runBtn.querySelector(".spinner");
  const errorBanner   = document.getElementById("errorBanner");
  const errorText     = document.getElementById("errorText");
  const exampleBanner = document.getElementById("exampleBanner");

  // DOM refs — Results
  const resultsDiv    = document.getElementById("results");
  const algoSelect    = document.getElementById("algoSelect");
  const statusBadge   = document.getElementById("statusBadge");
  const summaryCards  = document.getElementById("summaryCards");
  const metricsBody   = document.querySelector("#metricsTable tbody");
  const canvas        = document.getElementById("ganttCanvas");
  const ctx           = canvas.getContext("2d");
  const tooltip       = document.getElementById("tooltip");

  // ---------------------------------------------------------------------------
  // State
  // ---------------------------------------------------------------------------
  let selectedFile = null; // File | null — null means "use example"
  let data         = null; // parsed simulation JSON

  // ---------------------------------------------------------------------------
  // Helpers
  // ---------------------------------------------------------------------------
  function colorForTask(taskId) {
    return PALETTE[taskId % PALETTE.length];
  }

  function fmt(n, digits) {
    return Number(n).toFixed(digits === undefined ? 1 : digits);
  }

  function setLoading(on) {
    runBtn.disabled = on;
    btnText.hidden  = on;
    spinner.hidden  = !on;
  }

  function showError(msg) {
    errorText.textContent = msg;
    errorBanner.hidden = false;
  }

  function clearError() {
    errorBanner.hidden = true;
    errorText.textContent = "";
  }

  function updateDropzoneLabel() {
    if (selectedFile) {
      dropzoneHint.textContent = `✓  ${selectedFile.name}  (${(selectedFile.size / 1024).toFixed(1)} KB)`;
      dropzone.classList.add("has-file");
    } else {
      dropzoneHint.textContent = "id, name, arrival, exec, period, deadline, priority";
      dropzone.classList.remove("has-file");
    }
    runBtn.disabled = false;
  }

  // ---------------------------------------------------------------------------
  // Dropzone interactions
  // ---------------------------------------------------------------------------
  dropzone.addEventListener("click", () => csvInput.click());
  dropzone.addEventListener("keydown", (e) => {
    if (e.key === "Enter" || e.key === " ") { e.preventDefault(); csvInput.click(); }
  });

  csvInput.addEventListener("change", () => {
    if (csvInput.files.length) {
      selectedFile = csvInput.files[0];
      updateDropzoneLabel();
      clearError();
    }
  });

  dropzone.addEventListener("dragover", (e) => {
    e.preventDefault();
    dropzone.classList.add("drag-over");
  });
  dropzone.addEventListener("dragleave", () => dropzone.classList.remove("drag-over"));
  dropzone.addEventListener("drop", (e) => {
    e.preventDefault();
    dropzone.classList.remove("drag-over");
    const file = e.dataTransfer.files[0];
    if (file && (file.type === "text/csv" || file.name.endsWith(".csv"))) {
      selectedFile = file;
      updateDropzoneLabel();
      clearError();
    } else {
      showError("Please drop a .csv file.");
    }
  });

  // ---------------------------------------------------------------------------
  // Use-example button — clear file selection and enable run
  // ---------------------------------------------------------------------------
  useExampleBtn.addEventListener("click", () => {
    selectedFile = null;
    csvInput.value = "";
    updateDropzoneLabel();
    clearError();
    runBtn.disabled = false;
  });

  // Enable run button initially (no file required — example fallback)
  runBtn.disabled = false;

  // ---------------------------------------------------------------------------
  // Run simulation
  // ---------------------------------------------------------------------------
  runBtn.addEventListener("click", runSimulation);

  async function runSimulation() {
    clearError();
    setLoading(true);
    exampleBanner.hidden = true;
    resultsDiv.hidden = true;

    const form = new FormData();
    if (selectedFile) {
      form.append("file", selectedFile);
    }
    form.append("algo",    algoInput.value    || "all");
    form.append("quantum", quantumInput.value || "");
    form.append("horizon", horizonInput.value || "");

    try {
      const res = await fetch("/api/simulate", { method: "POST", body: form });
      const json = await res.json();

      if (!res.ok || json.error) {
        showError(json.error || `Server error ${res.status}`);
        return;
      }

      data = json;
      exampleBanner.hidden = !json._usedExample;
      renderAll();
      resultsDiv.hidden = false;
      // Smooth scroll to results
      resultsDiv.scrollIntoView({ behavior: "smooth", block: "start" });
    } catch (err) {
      showError("Network error — is the server running? " + err.message);
    } finally {
      setLoading(false);
    }
  }

  // ---------------------------------------------------------------------------
  // Rendering
  // ---------------------------------------------------------------------------
  function renderAll() {
    algoSelect.innerHTML = data.results
      .map((r, i) => `<option value="${i}">${r.algorithm}</option>`)
      .join("");
    algoSelect.addEventListener("change", () => renderAlgorithm(Number(algoSelect.value)));
    renderAlgorithm(0);
  }

  function renderAlgorithm(index) {
    const result = data.results[index];
    renderSummary(result);
    renderMetricsTable(result);
    renderGantt(result, data.tasks);
  }

  function renderSummary(result) {
    const totalMissed   = result.metrics.reduce((s, m) => s + m.missedInstances, 0);
    const totalReleased = result.metrics.reduce((s, m) => s + m.totalInstances,  0);
    const cards = [
      { label: "CPU utilization",  value: fmt(result.cpuUtilization) + "%" },
      { label: "Idle ticks",       value: result.idleTime + " / " + result.horizon },
      { label: "Context switches", value: result.contextSwitches },
      { label: "Deadlines missed", value: totalMissed + " / " + totalReleased },
    ];
    summaryCards.innerHTML = cards
      .map((c) => `<div class="card"><p class="label">${c.label}</p><p class="value">${c.value}</p></div>`)
      .join("");

    statusBadge.textContent = result.allDeadlinesMet ? "All deadlines met" : "Deadlines missed";
    statusBadge.className   = "badge " + (result.allDeadlinesMet ? "ok" : "warn");
  }

  function renderMetricsTable(result) {
    metricsBody.innerHTML = result.metrics.map((m) => `
      <tr>
        <td><span class="task-dot" style="background:${colorForTask(m.taskId)}"></span>${m.taskId}</td>
        <td>${m.name}</td>
        <td>${m.totalInstances}</td>
        <td>${m.missedInstances}</td>
        <td>${fmt(m.missRate)}%</td>
        <td>${fmt(m.avgResponseTime, 2)}</td>
        <td>${m.totalRunTime}</td>
      </tr>`
    ).join("");
  }

  function renderGantt(result, tasks) {
    const rowHeight  = 34;
    const pxPerTick  = Math.max(6, Math.min(20, Math.floor(900 / Math.max(1, result.horizon))));
    const labelWidth = 100;
    const axisHeight = 24;
    const width      = labelWidth + result.horizon * pxPerTick + 10;
    const height     = axisHeight + tasks.length * rowHeight + 10;

    const dpr = window.devicePixelRatio || 1;
    canvas.width  = width  * dpr;
    canvas.height = height * dpr;
    canvas.style.width  = width  + "px";
    canvas.style.height = height + "px";
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, width, height);
    ctx.font = "11px Inter, -apple-system, sans-serif";
    ctx.textBaseline = "middle";

    // Row backgrounds + labels
    tasks.forEach((task, row) => {
      const y = axisHeight + row * rowHeight;
      ctx.fillStyle = row % 2 === 0 ? "rgba(255,255,255,0.025)" : "transparent";
      ctx.fillRect(0, y, width, rowHeight);

      // Coloured label dot
      ctx.fillStyle = colorForTask(task.id);
      ctx.beginPath();
      ctx.arc(14, y + rowHeight / 2, 4, 0, Math.PI * 2);
      ctx.fill();

      ctx.fillStyle = "#e7e9ee";
      ctx.fillText(task.name, 26, y + rowHeight / 2);
    });

    // Time axis
    ctx.strokeStyle = "#2a2e38";
    ctx.fillStyle   = "#9aa0ac";
    const tickEvery = Math.max(1, Math.ceil(20 / pxPerTick));
    for (let t = 0; t <= result.horizon; t += tickEvery) {
      const x = labelWidth + t * pxPerTick;
      ctx.beginPath();
      ctx.moveTo(x, axisHeight);
      ctx.lineTo(x, height);
      ctx.stroke();
      ctx.fillText(String(t), x + 2, axisHeight / 2);
    }

    // Build row index
    const rowOf = {};
    tasks.forEach((task, i) => (rowOf[task.id] = i));

    // Draw segments
    const segments = [];
    result.timeline.forEach((ev) => {
      if (ev.taskId === -1) return;
      const row = rowOf[ev.taskId];
      if (row === undefined) return;
      const x = labelWidth + ev.t * pxPerTick;
      const y = axisHeight + row * rowHeight + 5;
      const w = pxPerTick;
      const h = rowHeight - 10;
      ctx.fillStyle = colorForTask(ev.taskId);
      // Rounded segments for a premium feel (roundRect available Chrome 99+ / Firefox 112+)
      const segW = Math.max(1, w - 1);
      if (typeof ctx.roundRect === "function") {
        const r = Math.min(3, h / 2, segW / 2);
        ctx.beginPath();
        ctx.roundRect(x, y, segW, h, r);
        ctx.fill();
      } else {
        ctx.fillRect(x, y, segW, h);
      }
      segments.push({ x, y, w, h, ev });
    });

    canvas._segments   = segments;
    canvas._tasksById  = Object.fromEntries(tasks.map((t) => [t.id, t]));
  }

  // ---------------------------------------------------------------------------
  // Tooltip on Gantt hover
  // ---------------------------------------------------------------------------
  canvas.addEventListener("mousemove", (e) => {
    const rect = canvas.getBoundingClientRect();
    const mx = e.clientX - rect.left;
    const my = e.clientY - rect.top;
    const segs = canvas._segments || [];
    const hit = segs.find((s) => mx >= s.x && mx <= s.x + s.w && my >= s.y && my <= s.y + s.h);
    if (hit) {
      const task = canvas._tasksById[hit.ev.taskId];
      tooltip.hidden = false;
      tooltip.style.left = e.clientX + 14 + "px";
      tooltip.style.top  = e.clientY + 14 + "px";
      tooltip.textContent = `${task ? task.name : "Task " + hit.ev.taskId}  ·  t = ${hit.ev.t}  ·  instance #${hit.ev.instanceId}`;
    } else {
      tooltip.hidden = true;
    }
  });
  canvas.addEventListener("mouseleave", () => (tooltip.hidden = true));

})();
