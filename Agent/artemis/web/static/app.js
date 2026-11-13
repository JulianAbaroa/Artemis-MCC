// The page of the lists builder (LS.3, D291): the tab bar and the chip of the bench. Every tab registers itself from its module.
import { api, bench, h, tabs } from "/static/core.js";
import "/static/maps.js";

let active = null;

async function show(tab) {
  active = tab;
  for (const button of document.getElementById("tabs").children) button.classList.toggle("active", button.textContent === tab.name);
  const view = document.getElementById("view");
  view.replaceChildren();
  await tab.render(view);
}

const CHIPS = {
  ready: ["bench: ready", "chip-ok", "MCC is open with Artemis and the bench is alive: tests can run."],
  no_mcc: ["bench: no MCC", "chip-warn", "The game watcher runs but MCC (or its bench) is not alive: open MCC to run tests."],
  watcher_off: ["bench: watcher off", "chip-error", "The game watcher is not running: open Tools/game_watcher.bat to run tests."],
};

async function refreshBench() {
  const chip = document.getElementById("bench-chip");
  try {
    bench.state = await api("GET", "/api/status");
    const [text, style, tooltip] = CHIPS[bench.state.state];
    chip.textContent = text;
    chip.className = "chip " + style;
    chip.title = tooltip;
  } catch (error) {
    bench.state = null;
    chip.textContent = "bench: unknown";
    chip.className = "chip chip-off";
    chip.title = "The server did not answer: " + error.message;
  }
  if (active?.onBench) active.onBench(bench.state);
}

const strip = document.getElementById("tabs");
for (const tab of tabs) strip.append(h("button", { class: "tab", title: tab.tooltip, onclick: () => show(tab) }, tab.name));
await refreshBench();
await show(tabs[0]);
setInterval(refreshBench, 5000);