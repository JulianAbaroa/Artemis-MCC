// Helpers of the page (LS.3, D291): the call to the API with the token, element builder, status bar, state of the bench and the tabs registry.
const token = document.querySelector('meta[name="artemis-token"]').content;

export async function api(method, path, body) {
  const options = { method, headers: {} };
  if (method !== "GET") {
    options.headers["X-Artemis-Token"] = token;
    options.headers["Content-Type"] = "application/json";
    options.body = JSON.stringify(body ?? {});
  }
  const response = await fetch(path, options);
  let data = null;
  try { data = await response.json(); } catch (error) { data = null; }
  if (!response.ok) throw new Error(data?.error ?? `HTTP ${response.status}`);
  return data;
}

// h("button", {title: "..."}, "text", child): element with attributes and children (text is never read as HTML).
export function h(tag, attributes = {}, ...children) {
  const element = document.createElement(tag);
  for (const [key, value] of Object.entries(attributes)) {
    if (key.startsWith("on")) element.addEventListener(key.slice(2), value);
    else if (key === "class") element.className = value;
    else if (value === true) element.setAttribute(key, "");
    else if (value !== false && value !== null && value !== undefined) element.setAttribute(key, value);
  }
  for (const child of children.flat()) if (child !== null && child !== undefined && child !== false) element.append(child);
  return element;
}

// Every action leaves its state on the bar below: kind is "ok" or "error".
export function notify(kind, text) {
  const status = document.getElementById("status");
  status.className = "status " + kind;
  status.textContent = text;
}

export async function run(label, action) {
  try {
    const result = await action();
    notify("ok", label + ": ok");
    return result;
  } catch (error) {
    notify("error", label + ": " + error.message);
    return undefined;
  }
}

export const tabs = [];
export const bench = { state: null };

// A tab: name, tooltip of its button, render(view) that fills the view, onBench(state) called when the state of the bench changes.
export function registerTab(tab) {
  tabs.push(tab);
}