// Maps tab (LS.3, D291): the catalog of the training maps (Data/Training/Maps.json) and the recorded configs that are not in it yet.
import { api, bench, h, notify, registerTab, run } from "/static/core.js";

const STAGES = ["First", "Final"];
let rows = [];
let dirty = false;
let view = null;
let shownRecorded = "";

function guess(config) {
  const size = /(\d)v(\d)/.exec(config);
  return { config, name: config.replace(/^Map /, ""), size: size ? size[0] : "1v1", stage: /First/.test(config) ? "First" : "Final", used_by: [] };
}

function markDirty() {
  dirty = true;
  const mark = document.getElementById("maps-dirty");
  if (mark) mark.textContent = "unsaved changes";
}

function field(row, key) {
  const input = h("input", { title: { config: "Exact name of the match config recorded in the MCC lobby (capitals count).", name: "Name shown in the lists.", size: "Players per team, like 4v4." }[key] });
  input.value = row[key];
  input.addEventListener("input", () => { row[key] = input.value; markDirty(); });
  return input;
}

function stageSelect(row) {
  const select = h("select", { title: "First: no weapons or vehicles. Final: everything." }, STAGES.map((stage) => h("option", { value: stage }, stage)));
  select.value = row.stage;
  select.addEventListener("change", () => { row.stage = select.value; markDirty(); });
  return select;
}

function removeRow(row) {
  if (row.used_by.length && !confirm(`The map "${row.config}" is used by the lists: ${row.used_by.join(", ")}.\nRemove it anyway? (Those lists stop loading until you change their entries.)`)) return;
  rows = rows.filter((other) => other !== row);
  markDirty();
  draw();
}

function recordedNotInCatalog() {
  const configs = bench.state?.configs;
  return configs ? configs.filter((config) => !rows.some((row) => row.config === config)) : null;
}

function draw() {
  const body = h("tbody", {}, rows.map((row) => h("tr", {},
    h("td", {}, field(row, "config")), h("td", {}, field(row, "name")), h("td", {}, field(row, "size")), h("td", {}, stageSelect(row)),
    h("td", { class: "used", title: "Lists that have an entry with this map." }, row.used_by.length ? row.used_by.join(", ") : "-"),
    h("td", {}, h("button", { title: "Remove this map from the catalog (it is written when you press Save).", onclick: () => removeRow(row) }, "Remove")))));
  const table = h("table", {}, h("thead", {}, h("tr", {}, ["Config", "Name", "Size", "Stage", "Used by", ""].map((text) => h("th", {}, text)))), body);
  const recorded = recordedNotInCatalog();
  shownRecorded = JSON.stringify(recorded);
  const extra = recorded === null
    ? h("p", { class: "note" }, "MCC is not running, so the recorded configs are not available. You can still add a map by typing its config name with Add map.")
    : recorded.length === 0
      ? h("p", { class: "note" }, "Every recorded config is in the catalog.")
      : h("table", {}, h("thead", {}, h("tr", {}, ["Config", "Size", "Stage", ""].map((text) => h("th", {}, text)))),
        h("tbody", {}, recorded.map((config) => {
          const info = guess(config);
          return h("tr", {}, h("td", {}, config), h("td", {}, info.size), h("td", {}, info.stage), h("td", {},
            h("button", { title: "Add it to the catalog with the name, size and stage guessed from the config (edit them above before saving).",
              onclick: () => { rows.push(info); markDirty(); draw(); } }, "Add")));
        })));
  view.replaceChildren(
    h("div", { class: "toolbar" },
      h("button", { class: "primary", title: "Write the catalog to Data/Training/Maps.json (it is checked first).", onclick: save }, "Save"),
      h("button", { title: "Add an empty row: type the exact name of a config recorded in the lobby.", onclick: () => { rows.push(guess("")); markDirty(); draw(); } }, "Add map"),
      h("button", { title: "Read the catalog again and drop the unsaved changes.", onclick: () => load(true) }, "Reload"),
      h("span", { id: "maps-dirty", class: "dirty" }, dirty ? "unsaved changes" : "")),
    table,
    h("h2", {}, "Recorded configs not in the catalog"),
    extra);
}

async function load(announce) {
  const result = await run("Maps: load", () => api("GET", "/api/maps"));
  if (!result) return;
  rows = result.maps;
  dirty = false;
  draw();
  if (announce) notify("ok", `Maps: reloaded ${rows.length} maps`);
}

async function save() {
  const result = await run("Maps: save", () => api("PUT", "/api/maps", { maps: rows }));
  if (!result) return;
  rows = result.maps;
  dirty = false;
  draw();
  notify("ok", `Maps: saved ${rows.length} maps`);
}

registerTab({
  name: "Maps",
  tooltip: "Catalog of the training maps: one recorded match config per map.",
  render: async (container) => { view = container; await load(false); },
  onBench: () => { if (view && JSON.stringify(recordedNotInCatalog()) !== shownRecorded) draw(); },
});