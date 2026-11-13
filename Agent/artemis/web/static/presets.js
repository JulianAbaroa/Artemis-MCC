// Presets tab (LS.3, D291): edits the XML presets of Data/Presets: own values against the ones inherited from the chain of bases.
import { api, h, notify, registerTab, run } from "/static/core.js";

const NAME = /^[a-z0-9_]{1,31}$/;
const NUMBER = /^[+-]?(0[xX][0-9a-fA-F]+|[0-9]+)$/;
const ui = { search: "", onlySet: false, open: new Set(), index: {} };
let catalog = null;
let names = [];
let model = null; // {name, isNew, base, doc, own, writes, checks, dirty}
let inherited = {}; // resolved values of the base: {key: {value, from}}
let view = null;
let groupsBox = null;
let status = null;

const keyOf = (group, values, field) => (values.length ? `${group}[${values.join(",")}].${field}` : `${group}.${field}`);
const enc = encodeURIComponent;

function parseKey(key) {
  const found = /^([a-z0-9_]+)(?:\[([\d,]+)\])?\./.exec(key);
  return { group: found[1], values: found[2] ? found[2].split(",").map(Number) : [] };
}

function setStatus(kind, text) {
  if (!status) return;
  status.className = "chip chip-" + kind;
  status.textContent = text;
}

function markDirty() {
  if (!model) return;
  model.dirty = true;
  setStatus("warn", "unsaved changes");
}

async function loadNames() {
  names = (await api("GET", "/api/presets")).presets.map((row) => row.name);
}

async function inheritedOf(base) {
  return base ? (await api("GET", `/api/presets/${enc(base)}`)).resolved : {};
}

async function open(name) {
  const result = await run(`Preset ${name}: load`, () => api("GET", `/api/presets/${enc(name)}`));
  if (!result) { model = null; inherited = {}; draw(); setStatus("error", "does not compile: see the bar below"); return; }
  model = { name, isNew: false, base: result.base, doc: result.doc, own: result.own, writes: result.writes, checks: result.checks, dirty: false };
  inherited = (await run(`Preset ${name}: base`, () => inheritedOf(model.base))) ?? {};
  draw();
  setStatus("ok", `compiles: ${model.writes} writes` + (model.checks.length ? `, checks: ${model.checks.join(", ")}` : ""));
}

function confirmLeave() {
  return !model?.dirty || confirm(`The preset ${model.name} has unsaved changes. Discard them?`);
}

async function create(copy) {
  if (!confirmLeave()) return;
  const name = prompt(copy ? `Name of the copy of ${model.name} (lowercase letters, digits and _; up to 31):` : "Name of the new preset (lowercase letters, digits and _; up to 31):");
  if (name === null) return;
  if (!NAME.test(name)) return notify("error", `Preset name "${name}" is not valid: lowercase letters, digits and _, 1 to 31 characters.`);
  if (names.includes(name)) return notify("error", `There is already a preset called ${name}.`);
  const source = copy ? model : { base: "", doc: "", own: {} };
  model = { name, isNew: true, base: source.base, doc: source.doc, own: { ...source.own }, writes: 0, checks: [], dirty: true };
  inherited = (await run(`Preset ${name}: base`, () => inheritedOf(model.base))) ?? {};
  draw();
  setStatus("warn", "new preset: not saved yet");
  notify("ok", `Preset ${name}: created in the page, press Save to write it`);
}

async function save() {
  if (!model) return;
  const result = await run(`Preset ${model.name}: save`, () => api("PUT", `/api/presets/${enc(model.name)}`, { base: model.base, doc: model.doc, own: model.own }));
  if (!result) return setStatus("error", "does not compile: see the bar below");
  await loadNames();
  await open(model.name);
  notify("ok", `Preset ${result.name}: saved, compiles to ${result.writes} writes`);
}

async function remove() {
  if (!model) return;
  if (model.isNew) { model = null; await start(); return notify("ok", "New preset discarded"); }
  if (!confirm(`Delete the preset ${model.name}? The file is moved to Data/Presets/_deleted.`)) return;
  const name = model.name;
  const result = await run(`Preset ${name}: delete`, () => api("POST", `/api/presets/${enc(name)}/delete`, {}));
  if (!result) return;
  model = null;
  await start();
  notify("ok", `Preset ${name}: moved to ${result.moved_to}`);
}

function control(field, value, onchange) {
  let element;
  const values = field.enum ? catalog.enums[field.enum] : field.type === "bit" && field.max === 1 ? ["false", "true"] : null;
  if (values) {
    const options = values.includes(value) ? values : [value, ...values];
    element = h("select", {}, options.map((option) => h("option", { value: option }, option)));
    element.value = value;
    element.addEventListener("change", () => onchange(element.value));
  } else {
    element = h("input", { type: "text", size: "12", title: `${field.type}${field.min !== null ? `, from ${field.min} to ${field.max}` : ""}: a number, decimal or 0x hex` });
    element.value = value;
    element.addEventListener("input", () => {
      element.classList.toggle("invalid", !NUMBER.test(element.value.trim()));
      onchange(element.value);
    });
  }
  element.title = element.title || `${field.name}: ${field.enum ? "one of " + field.enum : field.type}`;
  return element;
}

function firstValue(field) {
  if (field.enum) return catalog.enums[field.enum][0];
  return field.type === "bit" && field.max === 1 ? "false" : "0";
}

function fieldRow(group, field, values) {
  const key = keyOf(group.name, values, field.name);
  const own = key in model.own;
  const parent = inherited[key];
  if (ui.search && !(field.name.includes(ui.search) || field.doc.toLowerCase().includes(ui.search))) return null;
  if (ui.onlySet && !own && !parent) return null;
  const shown = own ? model.own[key] : parent ? parent.value : firstValue(field);
  const input = control(field, shown, (value) => { model.own[key] = value; markDirty(); });
  input.disabled = !own;
  const set = (value) => () => { model.own[key] = value; markDirty(); drawGroups(); };
  const [origin, button] = own
    ? ["set here", h("button", { title: "Remove the value set here: the field goes back to what the base gives.", onclick: () => { delete model.own[key]; markDirty(); drawGroups(); } }, "Inherit")]
    : parent
      ? [`from ${parent.from}`, h("button", { title: `Set this field in ${model.name} (starts from the value of ${parent.from}).`, onclick: set(parent.value) }, "Override")]
      : ["not set (the game value)", h("button", { title: `Set this field in ${model.name}.`, onclick: set(firstValue(field)) }, "Set")];
  const unit = field.unit ? ` ${field.unit}` : "";
  return h("tr", { class: own ? "" : "inherited" },
    h("td", { title: `${field.doc || field.name}\n${field.type}${unit}; applied: ${field.when}` }, field.name),
    h("td", {}, input, unit),
    h("td", { class: "used" }, origin),
    h("td", {}, button));
}

function instancesWithValues(group) {
  const found = new Set();
  for (const key of [...Object.keys(model.own), ...Object.keys(inherited)]) {
    const parsed = parseKey(key);
    if (parsed.group === group.name) found.add(parsed.values.join(","));
  }
  return found;
}

function groupBlock(group) {
  const present = instancesWithValues(group);
  const chosen = (ui.index[group.name] ??= group.indices.map(() => 0));
  const selectors = group.indices.map((index, position) => {
    const prefix = chosen.slice(0, position).join(",");
    const marked = (value) => [...present].some((text) => (position === group.indices.length - 1 ? text === [...chosen.slice(0, position), value].join(",") : text.startsWith([...chosen.slice(0, position), value].join(",") + ",")));
    const select = h("select", { title: `Which ${index.name} of the group (0 to ${index.count - 1}). A * marks the ones with values.` },
      Array.from({ length: index.count }, (_, value) => h("option", { value }, `${index.name} ${value}${marked(value) ? " *" : ""}`)));
    select.value = chosen[position];
    select.addEventListener("change", () => { chosen[position] = Number(select.value); drawGroups(); });
    return h("label", { class: "note" }, select);
  });
  const rows = group.fields.map((field) => fieldRow(group, field, chosen)).filter(Boolean);
  const anyValues = present.size > 0;
  if (ui.onlySet && !anyValues) return null;
  if ((ui.search || ui.onlySet) && !rows.length) return null;
  const check = group.check
    ? h("p", { class: "note", title: `Written to the compiled preset when the index has values: the game checks that the Megalo script has that trait set.` },
      `check ${group.check} ${chosen[0]}: ` + (present.has(chosen.join(",")) ? "on (this index has values)" : "off (no values at this index)"))
    : null;
  const details = h("details", { open: ui.open.has(group.name) || !!ui.search },
    h("summary", { title: group.doc }, group.name + (group.traits ? " (traits)" : ""), h("span", { class: "used" }, `  ${group.doc}`)),
    selectors, check,
    h("table", {}, h("tbody", {}, rows)));
  details.addEventListener("toggle", () => { if (details.open) ui.open.add(group.name); else ui.open.delete(group.name); });
  return details;
}

function drawGroups() {
  if (!groupsBox) return;
  groupsBox.replaceChildren(...(model ? catalog.groups.map(groupBlock).filter(Boolean) : [h("p", { class: "note" }, "Pick a preset or press New.")]));
}

function draw() {
  const picker = h("select", { title: "The preset to edit (Data/Presets/<name>.xml)." }, [...names, ...(model?.isNew ? [model.name] : [])].map((name) => h("option", { value: name }, name)));
  if (model) picker.value = model.name;
  picker.addEventListener("change", async () => { if (confirmLeave()) await open(picker.value); else picker.value = model?.name ?? ""; });
  const base = h("select", { title: "The preset this one starts from: it gives every value this one does not set." },
    [h("option", { value: "" }, "(none)"), ...names.filter((name) => name !== model?.name).map((name) => h("option", { value: name }, name))]);
  base.value = model?.base ?? "";
  base.disabled = !model;
  base.addEventListener("change", async () => {
    model.base = base.value;
    markDirty();
    inherited = (await run("Preset base", () => inheritedOf(model.base))) ?? {};
    drawGroups();
  });
  const doc = h("input", { type: "text", title: "What this preset is for (the doc attribute of the XML).", size: "60" });
  doc.value = model?.doc ?? "";
  doc.disabled = !model;
  doc.addEventListener("input", () => { model.doc = doc.value; markDirty(); });
  status = h("span", { class: "chip chip-off", title: "Whether the preset compiles with its bases (from the last load or save)." }, "-");
  const search = h("input", { type: "search", placeholder: "Search field", title: "Show only the fields whose name or description has this text." });
  search.value = ui.search;
  search.addEventListener("input", () => { ui.search = search.value.toLowerCase(); drawGroups(); });
  const only = h("input", { type: "checkbox" });
  only.checked = ui.onlySet;
  only.addEventListener("change", () => { ui.onlySet = only.checked; drawGroups(); });
  groupsBox = h("div", {});
  view.replaceChildren(
    h("div", { class: "toolbar" }, picker,
      h("button", { title: "Start a new preset (it is written when you press Save).", onclick: () => create(false) }, "New"),
      h("button", { title: "Copy this preset with another name (it is written when you press Save).", disabled: !model, onclick: () => create(true) }, "Duplicate"),
      h("button", { title: "Move this preset to Data/Presets/_deleted (refused when another preset or a list uses it).", disabled: !model, onclick: remove }, "Delete")),
    h("div", { class: "toolbar" }, h("label", { class: "note" }, "Base "), base, h("label", { class: "note" }, "Doc "), doc,
      h("button", { class: "primary", title: "Write the XML if it compiles with its bases.", disabled: !model, onclick: save }, "Save"), status),
    h("div", { class: "toolbar" }, search, h("label", { class: "note", title: "Hide the fields nobody sets (neither this preset nor its bases)." }, only, " only fields that are set")),
    groupsBox);
  drawGroups();
}

async function start() {
  await run("Presets: load", async () => {
    catalog ??= await api("GET", "/api/catalog");
    await loadNames();
  });
  if (!catalog) return;
  model = null;
  if (names.length) await open(names[0]); else draw();
}

registerTab({
  name: "Presets",
  tooltip: "Gametype presets: the values of the game variant a training entry can change.",
  render: async (container) => { view = container; await start(); },
});