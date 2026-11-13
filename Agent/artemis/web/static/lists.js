// Lists tab (LS.3, D291): builds a training list (Data/Training/<name>.json), shows its composition and tests one entry or the whole list.
import { api, bench, h, notify, registerTab, run } from "/static/core.js";

const NAME = /^[a-z0-9_]{1,31}$/;
const KEEP = "~"; // value of the "(no change)" option of the preset select: a preset name can never be "~"
let maps = [];
let presets = [];
let names = [];
let model = null; // {name, isNew, order, entries: [{map, preset, auto, rival, rounds, weight}], stale, error, dirty}
let view = null;
let body = null;
let resultBox = null;
let poll = 0;
let episodes = 3;

const text = (value) => (value === undefined || value === null ? "" : String(value));
const enc = encodeURIComponent;

function toRows(entries) {
  return entries.map((entry) => ({
    map: entry.map, preset: entry.preset, rounds: entry.rounds, weight: entry.weight,
    auto: entry.roster && entry.roster.auto !== undefined ? text(entry.roster.auto) : "", rival: entry.roster ? text(entry.roster.rival) : "",
  }));
}

// Blank same team + blank rival = roster null (no change); a blank same team is left out (0), as the files have it.
function fromRows(rows) {
  return rows.map((row) => {
    const roster = row.auto === "" && row.rival === "" ? null : { ...(row.auto === "" ? {} : { auto: Number(row.auto) }), rival: Number(row.rival === "" ? 0 : row.rival) };
    return { map: row.map, preset: row.preset, roster, rounds: Number(row.rounds), weight: Number(row.weight) };
  });
}

function share(row) {
  return model.order === "weighted" ? Number(row.weight) * Number(row.rounds) : Number(row.rounds);
}

function percentages(key) {
  const total = model.entries.reduce((sum, row) => sum + share(row), 0);
  const parts = new Map();
  for (const row of model.entries) {
    const info = maps.find((map) => map.config === row.map);
    const label = info ? info[key] : "unknown";
    parts.set(label, (parts.get(label) ?? 0) + share(row));
  }
  return [...parts].sort().map(([label, value]) => `${label} ${total > 0 ? Math.round((100 * value) / total) : 0}%`).join(", ") || "-";
}

function drawComposition(box) {
  const rounds = model.entries.reduce((sum, row) => sum + Number(row.rounds || 0), 0);
  const players = Math.max(0, ...model.entries.map((row) => 1 + Number(row.auto || 0) + Number(row.rival || 0)));
  const used = [...new Set(model.entries.map((row) => row.preset).filter((preset) => preset))];
  const rows = [
    ["Rounds in a cycle", `${rounds}`, "Total rounds of one pass over the list (the sum of Rounds)."],
    ["By size", percentages("size"), model.order === "weighted" ? "Share of the rounds drawn: weight x rounds of each entry." : "Share of the rounds of a cycle."],
    ["By stage", percentages("stage"), "First has no weapons or vehicles, Final has everything."],
    ["Max players", `${players} (the local player + bots)`, "The most players any entry needs: the local player plus the bots of both teams."],
    ["Presets", model.stale ? `stale: ${model.stale}` : used.length ? `up to date (${used.join(", ")})` : "none used", "Whether the compiled presets saved in the list are the ones of Data/Presets now."],
  ];
  box.replaceChildren(h("table", {}, h("tbody", {}, rows.map(([label, value, tip]) => h("tr", { title: tip }, h("td", {}, label), h("td", { class: label === "Presets" && model.stale ? "dirty" : "" }, value))))));
}

function select(options, value, onchange, title) {
  const element = h("select", { title }, options.map(([optionValue, label]) => h("option", { value: optionValue }, label)));
  element.value = value;
  element.addEventListener("change", () => onchange(element.value));
  return element;
}

function number(row, key, min, title, step = "1") {
  const input = h("input", { type: "number", min, step, title, size: "4" });
  input.value = row[key];
  input.addEventListener("input", () => { row[key] = input.value; markDirty(); drawComposition(compositionBox); });
  return input;
}

let compositionBox = null;

function markDirty() {
  model.dirty = true;
  const chip = document.getElementById("list-state");
  if (chip) { chip.className = "chip chip-warn"; chip.textContent = "unsaved changes"; }
}

function testReason(index) {
  if (!model || model.isNew || model.dirty) return "Save the list first: the test uses the saved file.";
  if (bench.state?.state !== "ready") return bench.state?.state === "watcher_off" ? "The game watcher is not running (open Tools/game_watcher.bat)." : "MCC is not open with Artemis: open it to run tests.";
  return "";
}

function entryRow(row, index) {
  const count = model.entries.length;
  const mapOptions = maps.map((map) => [map.config, `${map.name}`]);
  if (!maps.some((map) => map.config === row.map)) mapOptions.unshift([row.map, `${row.map} (not in the catalog)`]);
  const presetOptions = [[KEEP, "(no change)"], ["", "(base values)"], ...presets.map((preset) => [preset, preset])];
  if (row.preset && !presets.includes(row.preset)) presetOptions.push([row.preset, `${row.preset} (missing)`]);
  const reason = testReason(index);
  const move = (by) => () => { const other = index + by; [model.entries[index], model.entries[other]] = [model.entries[other], model.entries[index]]; markDirty(); drawBody(); };
  return h("tr", {},
    h("td", {}, String(index)),
    h("td", {}, select(mapOptions, row.map, (value) => { row.map = value; markDirty(); drawComposition(compositionBox); }, "The map: a recorded match config of the catalog (Maps tab).")),
    h("td", {}, select(presetOptions, row.preset === null ? KEEP : row.preset, (value) => { row.preset = value === KEEP ? null : value; markDirty(); drawComposition(compositionBox); },
      "(no change) keeps the preset of the match, (base values) goes back to the values of the gametype without a preset.")),
    h("td", {}, number(row, "auto", "0", "Bots on the team of the local player (blank = none). The local player is a passive rival and does not count in NvM.")),
    h("td", {}, number(row, "rival", "1", "Bots on the other team (at least 1). Leave both this and Same team blank to keep the bots as they are.")),
    h("td", {}, number(row, "rounds", "1", "Rounds played with this entry before the next one.")),
    model.order === "weighted" ? h("td", {}, number(row, "weight", "0", "Chance of drawing this entry, relative to the others (above 0).", "any")) : null,
    h("td", {}, h("button", { title: reason || "Play this entry in the bench (a match with this setup) and check it applied.", disabled: !!reason, onclick: () => testEntry(index) }, "Test")),
    h("td", {}, h("div", { class: "row-actions" },
      h("button", { title: "Move this entry up.", disabled: index === 0, onclick: move(-1) }, "Up"),
      h("button", { title: "Move this entry down.", disabled: index === count - 1, onclick: move(1) }, "Down"),
      h("button", { title: "Remove this entry (written when you press Save).", onclick: () => { model.entries.splice(index, 1); markDirty(); drawBody(); } }, "Remove"))));
}

function drawBody() {
  if (!body) return;
  if (!model) { body.replaceChildren(h("p", { class: "note" }, "Pick a list or press New.")); return; }
  const weighted = model.order === "weighted";
  const heads = ["#", "Map", "Preset", "Same team", "Rival", "Rounds", ...(weighted ? ["Weight"] : []), "", ""];
  compositionBox = h("div", {});
  const episodesInput = h("input", { type: "number", min: "1", max: "50", step: "1", size: "4", title: "How many episodes env_check plays (1 to 50)." });
  episodesInput.value = episodes;
  episodesInput.addEventListener("input", () => { episodes = episodesInput.value; });
  const listReason = !model || model.isNew || model.dirty ? "Save the list first: the test uses the saved file." : model.stale ? "The list is stale: save it first." : testReason(0);
  resultBox = h("div", {});
  body.replaceChildren(...[
    model.stale ? h("p", { class: "note dirty", title: "A preset changed after the list was saved. Save regenerates the compiled presets." }, `Stale: ${model.stale}. Press Save to compile the presets again.`) : null,
    model.error ? h("p", { class: "note chip-error", title: "The list does not load as it is: fix it and save." }, `Does not load: ${model.error}`) : null,
    h("table", {}, h("thead", {}, h("tr", {}, heads.map((head) => h("th", {}, head)))), h("tbody", {}, model.entries.map(entryRow))),
    h("div", { class: "toolbar" }, h("button", { title: "Add an entry at the end.", onclick: () => {
      model.entries.push({ map: maps[0]?.config ?? "", preset: null, auto: "", rival: "1", rounds: 1, weight: 1 }); markDirty(); drawBody();
    } }, "Add entry")),
    h("h2", {}, "Composition"), compositionBox,
    h("h2", {}, "Test the list"),
    h("div", { class: "toolbar" }, h("label", { class: "note" }, "Episodes "), episodesInput,
      h("button", { title: listReason || "Play the whole list with env_check (the seek policy) in the bench. It takes several minutes.", disabled: !!listReason, onclick: testList }, "Test the list")),
    resultBox].filter(Boolean));
  drawComposition(compositionBox);
  drawResult();
}

let lastJob = null;

function drawResult() {
  if (!resultBox) return;
  const job = lastJob;
  if (!job) { resultBox.replaceChildren(h("p", { class: "note" }, "No test run yet.")); return; }
  const verdict = job.passed === true ? "PASS" : job.passed === false ? "FAIL" : job.state;
  const lines = [`${job.kind === "entry" ? `Entry ${job.index}` : "List"} ${job.list}: ${verdict}${job.seconds ? ` (${job.seconds} s)` : ""}${job.message ? " - " + job.message : ""}`];
  for (const match of job.matches ?? []) {
    lines.push(`match ${match.name}: ${match.result} (${match.end}, ${match.ticks} ticks)`, ...match.failed.map((failure) => "  FAIL " + failure), ...match.log.map((line) => "  log: " + line));
  }
  lines.push(...(job.output ?? []).slice(-30));
  resultBox.replaceChildren(h("pre", { class: job.passed === false ? "chip-error" : job.passed ? "chip-ok" : "note", title: "Result of the last test." }, lines.join("\n")));
}

async function follow(id) {
  const mine = ++poll;
  while (mine === poll) {
    const job = await run("Test", () => api("GET", `/api/test/${enc(id)}`));
    if (!job) return;
    lastJob = job;
    drawResult();
    if (job.state === "done" || job.state === "error") {
      notify(job.passed ? "ok" : "error", `Test ${job.kind === "entry" ? "entry " + job.index : "list"} ${job.list}: ${job.passed ? "PASS" : "FAIL"}${job.message ? " - " + job.message : ""}`);
      return;
    }
    await new Promise((resolve) => setTimeout(resolve, 2000));
  }
}

async function testEntry(index) {
  if (model.dirty) return notify("error", "Save the list first: the test uses the saved file.");
  const started = await run(`Test entry ${index}`, () => api("POST", "/api/test/entry", { list: model.name, index }));
  if (started) { lastJob = { id: started.id, kind: "entry", index, list: model.name, state: started.state, passed: null }; drawResult(); follow(started.id); }
}

async function testList() {
  if (model.dirty) return notify("error", "Save the list first: the test uses the saved file.");
  const count = Number(episodes);
  if (!Number.isInteger(count) || count < 1 || count > 50) return notify("error", "Episodes must be a whole number from 1 to 50.");
  const started = await run("Test the list", () => api("POST", "/api/test/list", { list: model.name, episodes: count }));
  if (started) { lastJob = { id: started.id, kind: "list", list: model.name, state: started.state, passed: null }; drawResult(); follow(started.id); }
}

function setModel(data, isNew) {
  model = { name: data.name, isNew, order: data.order, entries: toRows(data.entries), stale: data.stale, error: data.error, dirty: isNew };
}

function draw() {
  const picker = select([...names, ...(model?.isNew ? [model.name] : [])].map((name) => [name, name]), model?.name ?? "", async (value) => { if (confirmLeave()) await open(value); else draw(); },
    "The list to edit (Data/Training/<name>.json).");
  const order = select([["sequence", "sequence"], ["weighted", "weighted"]], model?.order ?? "sequence", (value) => { model.order = value; markDirty(); drawBody(); },
    "sequence plays every entry for its rounds in order and starts again. weighted draws an entry by its weight and plays it for its rounds.");
  order.disabled = !model;
  const stateText = !model ? "-" : model.dirty ? "unsaved changes" : model.stale ? "stale" : model.error ? "does not load" : "saved";
  const stateClass = !model ? "off" : model.dirty || model.stale ? "warn" : model.error ? "error" : "ok";
  body = h("div", {});
  view.replaceChildren(
    h("div", { class: "toolbar" }, picker,
      h("button", { title: "Start a new list (written when you press Save).", onclick: () => create(false) }, "New"),
      h("button", { title: "Copy this list with another name (written when you press Save).", disabled: !model, onclick: () => create(true) }, "Duplicate"),
      h("button", { title: "Move this list to Data/Training/_deleted.", disabled: !model, onclick: remove }, "Delete"),
      h("label", { class: "note" }, "Order "), order,
      h("button", { class: "primary", title: "Write the list (it is checked first and its presets are compiled again).", disabled: !model, onclick: save }, "Save"),
      h("span", { id: "list-state", class: "chip chip-" + stateClass, title: "Whether the file is as you see it, and whether its presets are up to date." }, stateText)),
    body);
  drawBody();
}

function confirmLeave() {
  return !model?.dirty || confirm(`The list ${model.name} has unsaved changes. Discard them?`);
}

async function open(name) {
  const data = await run(`List ${name}: load`, () => api("GET", `/api/lists/${enc(name)}`));
  if (!data) return;
  setModel(data, false);
  lastJob = null;
  poll++;
  draw();
}

async function create(copy) {
  if (!confirmLeave()) return;
  const name = prompt(copy ? `Name of the copy of ${model.name} (lowercase letters, digits and _; up to 31):` : "Name of the new list (lowercase letters, digits and _; up to 31):");
  if (name === null) return;
  if (!NAME.test(name) || name === "maps") return notify("error", `List name "${name}" is not valid: lowercase letters, digits and _, 1 to 31 characters (not "maps").`);
  if (names.includes(name)) return notify("error", `There is already a list called ${name}.`);
  const source = copy ? model : { order: "sequence", entries: [{ map: maps[0]?.config ?? "", preset: null, auto: "", rival: "1", rounds: 1, weight: 1 }] };
  model = { name, isNew: true, order: source.order, entries: copy ? source.entries.map((row) => ({ ...row })) : source.entries, stale: null, error: null, dirty: true };
  lastJob = null;
  poll++;
  draw();
  notify("ok", `List ${name}: created in the page, press Save to write it`);
}

async function save() {
  const sent = { order: model.order, entries: fromRows(model.entries) };
  const name = model.name;
  const data = await run(`List ${name}: save`, () => api("PUT", `/api/lists/${enc(name)}`, sent));
  if (!data) return;
  names = (await api("GET", "/api/lists")).lists.map((row) => row.name);
  setModel(data, false);
  draw();
  notify("ok", `List ${name}: saved (${data.entries.length} entries${data.stale ? ", still stale" : ""})`);
}

async function remove() {
  if (!model) return;
  if (model.isNew) { model = null; await start(); return notify("ok", "New list discarded"); }
  if (!confirm(`Delete the list ${model.name}? The file is moved to Data/Training/_deleted.`)) return;
  const name = model.name;
  const result = await run(`List ${name}: delete`, () => api("POST", `/api/lists/${enc(name)}/delete`, {}));
  if (!result) return;
  model = null;
  await start();
  notify("ok", `List ${name}: moved to ${result.moved_to}`);
}

async function start() {
  const loaded = await run("Lists: load", async () => {
    maps = (await api("GET", "/api/maps")).maps;
    presets = (await api("GET", "/api/presets")).presets.map((row) => row.name);
    names = (await api("GET", "/api/lists")).lists.map((row) => row.name);
    return true;
  });
  if (!loaded) return;
  model = null;
  if (names.length) await open(names[0]); else draw();
}

let shownBench = null;

registerTab({
  name: "Lists",
  tooltip: "Training lists: the maps, presets and bots a training run goes through.",
  render: async (container) => { view = container; shownBench = bench.state?.state ?? null; await start(); },
  onBench: (state) => { if (view && model && (state?.state ?? null) !== shownBench) { shownBench = state?.state ?? null; drawBody(); } },
});