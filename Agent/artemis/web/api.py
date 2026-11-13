"""
api.py
The endpoints of the lists builder (LS.3, D291) as a table of routes: ROUTES holds (method, pattern, handler) and dispatch() runs the first one
that matches. A handler takes (context, match, body) and returns (status, payload) or raises ApiError. The orchestrator (O.4) adds its own
with route(). Errors are {"error": "..."} with the status of the ApiError.

What it reads and writes is Data/Training (Maps.json and the lists) and Data/Presets only (D291 S5), through artemis.training.
"""
import importlib.util
import json
import os
import re
import sys
import threading
import time
from dataclasses import dataclass, field
from typing import Callable

from ..training.catalog import MAPS_PATH, ROOT, load_catalog

MAP_FIELDS = ("config", "name", "size", "stage")
MAPS_FILE = "Maps.json"
TOOLS = os.path.join(ROOT, "Tools")
DELETED = "_deleted"
_loaded = {}


class ApiError(Exception):
    """An answer that is an error: status (400, 403, 404, 409...) and the message the page shows."""

    def __init__(self, status, message):
        super().__init__(message)
        self.status = status
        self.message = message


def _tool(name):
    """Tools/<name>.py as a library, loaded once from the repo (as artemis.training.lists loads presets.py)."""
    if name not in _loaded:
        if TOOLS not in sys.path:
            sys.path.insert(0, TOOLS)
        spec = importlib.util.spec_from_file_location("artemis_tool_" + name, os.path.join(TOOLS, name + ".py"))
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        _loaded[name] = module
    return _loaded[name]


def game_tool():
    """Tools/game.py as a library (the bench files of the game watcher)."""
    return _tool("game")


def presets_tool():
    """Tools/presets.py as a library: the one parser and compiler of the presets."""
    return _tool("presets")


def valid_name(name) -> str:
    """The name of a preset or a list ([a-z0-9_], 1 to 31 characters, the rule of presets.py), or ApiError 400."""
    if not isinstance(name, str) or not presets_tool().NAME.fullmatch(name):
        raise ApiError(400, f"{name!r} is not a valid name (lowercase letters, digits and _, 1 to 31 characters)")
    return name


def safe_path(folder, name, extension) -> str:
    """folder/name+extension for a valid name, checked to stay inside the folder (D291 S4): every path of a preset or a list is built here."""
    path = os.path.join(folder, valid_name(name) + extension)
    inside = os.path.realpath(folder) + os.sep
    if not os.path.realpath(path).startswith(inside):
        raise ApiError(400, f"{name!r} is not a valid name")
    return path


def move_to_deleted(folder, path) -> str:
    """Deleting is moving the file to <folder>/_deleted/ (with the date in its name when that name is taken); nothing is ever removed (D291 S5).
    Returns the new path."""
    target = os.path.join(folder, DELETED)
    os.makedirs(target, exist_ok=True)
    stem, extension = os.path.splitext(os.path.basename(path))
    destination = os.path.join(target, stem + extension)
    count = 0
    while os.path.exists(destination):
        count += 1
        destination = os.path.join(target, f"{stem}_{time.strftime('%Y%m%d_%H%M%S')}{'' if count == 1 else '_' + str(count)}{extension}")
    os.replace(path, destination)
    return destination


def game_status() -> dict:
    """The state of the bench: {"state": "ready" | "no_mcc" | "watcher_off", "watcher", "mcc", "bench", "configs"}. configs are the match
    configs MCC has recorded (the list the bench reports), or None when the bench is not alive."""
    game = game_tool()
    watcher = game.Watcher()
    bench = game.Bench()
    alive = bool(bench and bench.get("_alive"))
    if alive:
        state = "ready"
    elif watcher:
        state = "no_mcc"
    else:
        state = "watcher_off"
    return {"state": state, "watcher": bool(watcher), "mcc": bool(watcher and watcher.get("mccRunning")), "bench": alive,
            "configs": list(bench.get("configs") or []) if alive else None}


@dataclass
class Context:
    """The folders the API reads and writes and where it asks for the state of the bench. Tests give temporary copies of Data/."""
    presets_dir: str
    training_dir: str
    status_fn: Callable = game_status
    lock: threading.RLock = field(default_factory=threading.RLock)

    @property
    def maps_path(self):
        return os.path.join(self.training_dir, MAPS_FILE)


def default_context() -> Context:
    return Context(os.path.join(ROOT, "Data", "Presets"), os.path.dirname(MAPS_PATH))


ROUTES = []


def route(method, pattern):
    """Decorator: registers the handler for the method and the regular expression of the whole path."""
    def register(handler):
        ROUTES.append((method, re.compile(pattern + r"\Z"), handler))
        return handler
    return register


def dispatch(context, method, path, body):
    """(status, payload) of the first route that matches. 404 when no path matches, 405 when the path exists for other methods."""
    known = False
    for route_method, pattern, handler in ROUTES:
        found = pattern.match(path)
        if found:
            if route_method == method:
                return handler(context, found, body)
            known = True
    raise ApiError(405 if known else 404, f"method {method} is not allowed here" if known else "not found")


@route("GET", r"/api/status")
def get_status(context, found, body):
    return 200, context.status_fn()


def lists_using(training_dir, key) -> dict:
    """{value of `key` of an entry (map or preset): [names of the lists that use it]} from the files of the lists in the folder (read as they
    are: a broken list is skipped)."""
    used = {}
    for file in sorted(os.listdir(training_dir)):
        path = os.path.join(training_dir, file)
        if not file.endswith(".json") or file == MAPS_FILE or not os.path.isfile(path):
            continue
        try:
            with open(path, encoding="utf-8") as handle:
                values = {entry[key] for entry in json.load(handle)["entries"] if entry.get(key) is not None}
        except (OSError, ValueError, KeyError, TypeError, AttributeError):
            continue
        for value in values:
            used.setdefault(value, []).append(file[:-len(".json")])
    return used


def _maps_payload(context):
    try:
        catalog = load_catalog(context.maps_path)
    except (OSError, ValueError) as error:
        raise ApiError(409, f"{MAPS_FILE} is not valid: {error}") from None
    used = lists_using(context.training_dir, "map")
    return {"maps": [{**{key: getattr(info, key) for key in MAP_FIELDS}, "used_by": used.get(config, [])} for config, info in catalog.items()]}


@route("GET", r"/api/maps")
def get_maps(context, found, body):
    return 200, _maps_payload(context)


def maps_text(rows) -> str:
    """The text of Maps.json: a list with one map per line, in the order of the keys of the catalog, CRLF and no final newline."""
    lines = []
    for row in rows:
        row = {key: row[key] for key in MAP_FIELDS} if set(row) == set(MAP_FIELDS) else row
        lines.append("  " + json.dumps(row))
    return "[\r\n" + ",\r\n".join(lines) + ("\r\n" if lines else "") + "]"


@route("PUT", r"/api/maps")
def put_maps(context, found, body):
    rows = body.get("maps") if isinstance(body, dict) else None
    if not isinstance(rows, list) or not all(isinstance(row, dict) for row in rows):
        raise ApiError(400, 'expected {"maps": [{config, name, size, stage}, ...]}')
    rows = [{key: value for key, value in row.items() if key != "used_by"} for row in rows]
    temporary = context.maps_path + ".tmp"
    with context.lock:
        try:
            with open(temporary, "wb") as handle:
                handle.write(maps_text(rows).encode("ascii"))
            try:
                load_catalog(temporary)
            except ValueError as error:
                raise ApiError(400, str(error).replace(temporary, MAPS_FILE)) from None
            os.replace(temporary, context.maps_path)
        finally:
            if os.path.exists(temporary):
                os.remove(temporary)
        return 200, _maps_payload(context)


# Presets

def _catalog(context):
    tool = presets_tool()
    try:
        return tool.Catalog(os.path.join(context.presets_dir, "Catalog.xml"))
    except tool.PresetError as error:
        raise ApiError(409, str(error)) from None


def _numeric_range(field):
    kind = field.Kind
    if kind == "bit":
        return 0, (1 << field.Width) - 1
    low, high = presets_tool().Range(kind)
    return (-(1 << 31) if kind == "u32" else low), high


@route("GET", r"/api/catalog")
def get_catalog(context, found, body):
    """The enums and the groups with their fields (what the editor of presets draws): {enums: {name: [values]}, groups: [...]}."""
    catalog = _catalog(context)
    groups = []
    for group in catalog.Groups.values():
        fields = []
        for field in group.Fields:
            low, high = (None, None) if field.Enum else _numeric_range(field)
            fields.append({"name": field.Name, "type": field.Kind, "enum": field.Enum, "unit": field.Unit, "doc": field.Doc,
                           "when": field.When or group.When, "min": low, "max": high})
        groups.append({"name": group.Name, "doc": group.Doc, "when": group.When, "check": group.Check, "traits": group.Traits,
                       "indices": [{"name": name, "count": count} for name, count, _ in group.Indices], "fields": fields})
    return 200, {"enums": {name: list(values) for name, values in catalog.Enums.items()}, "groups": groups}


def _preset_file(context, found):
    name = valid_name(found.group(1))
    return name, safe_path(context.presets_dir, name, ".xml")


def _preset_base(tool, path, catalog):
    try:
        return tool.ReadOwn(path, catalog)[1:]
    except tool.PresetError as error:
        return "", f"(error: {error})"


@route("GET", r"/api/presets")
def get_presets(context, found, body):
    tool, catalog = presets_tool(), _catalog(context)
    names, _ = tool.Sources(context.presets_dir)
    rows = []
    for name in names:
        base, doc = _preset_base(tool, os.path.join(context.presets_dir, name + ".xml"), catalog)
        rows.append({"name": name, "base": base, "doc": doc})
    return 200, {"presets": rows}


def _checks(text):
    return [line[len("check "):] for line in text.splitlines() if line.startswith("check ")]


@route("GET", r"/api/presets/(.+)")
def get_preset(context, found, body):
    name, path = _preset_file(context, found)
    if not os.path.isfile(path):
        raise ApiError(404, f"there is no preset {name}")
    tool, catalog = presets_tool(), _catalog(context)
    try:
        text, base, doc, writes = tool.CompilePreset(name, context.presets_dir, catalog)
        own = tool.ReadOwn(path, catalog)[0]
        resolved = tool.Resolve(name, context.presets_dir, catalog)
    except tool.PresetError as error:
        raise ApiError(409, str(error)) from None
    return 200, {"name": name, "base": base, "doc": doc, "own": own, "writes": writes, "checks": _checks(text),
                 "resolved": {key: {"value": value, "from": origin} for key, (value, origin) in resolved.items()}}


def _leading_comment(path):
    """The comments between the XML declaration and <preset> of the file as they are (a rewrite keeps them), or ""."""
    try:
        with open(path, encoding="utf-8") as handle:
            text = handle.read().replace("\r\n", "\n")
    except OSError:
        return ""
    found = re.match(r"\s*<\?xml[^>]*\?>\s*((?:<!--.*?-->\s*)+)", text, re.S)
    return found.group(1).strip() if found else ""


def _preset_body(body):
    if not isinstance(body, dict) or not isinstance(body.get("own", {}), dict) or not isinstance(body.get("doc", ""), str):
        raise ApiError(400, 'expected {"base": "", "doc": "", "own": {"group.field": "value"}}')
    base = body.get("base") or ""
    if base:
        valid_name(base)
    own = {}
    for key, value in body.get("own", {}).items():
        if isinstance(value, bool) or not isinstance(value, (str, int)) or str(value).strip() == "":
            raise ApiError(400, f"the value of {key!r} is a text or a number")
        own[key] = str(value)
    return base, body.get("doc", ""), own


@route("PUT", r"/api/presets/(.+)")
def put_preset(context, found, body):
    """Writes the preset when it compiles with its bases: the candidate goes to <name>.xml.tmp, is compiled in place of the file and replaces
    it only if it compiles. Returns the compiled lines (writes and checks)."""
    name, path = _preset_file(context, found)
    base, doc, own = _preset_body(body)
    tool, catalog = presets_tool(), _catalog(context)
    temporary = path + ".tmp"
    with context.lock:
        try:
            xml = tool.PresetXml(base, doc, own, catalog, _leading_comment(path))
        except ValueError as error:
            raise ApiError(400, str(error)) from None
        try:
            with open(temporary, "wb") as handle:
                handle.write(xml.replace("\n", "\r\n").encode("utf-8"))
            try:
                text, _, _, writes = tool.CompilePreset(name, context.presets_dir, catalog, paths={name: temporary})
            except tool.PresetError as error:
                raise ApiError(400, str(error).replace(tool.Relative(temporary), tool.Relative(path))) from None
            os.replace(temporary, path)
        finally:
            if os.path.exists(temporary):
                os.remove(temporary)
    return 200, {"name": name, "writes": writes, "checks": _checks(text), "lines": text.splitlines()}


@route("POST", r"/api/presets/(.+)/delete")
def delete_preset(context, found, body):
    """Moves the preset to _deleted/. 409 when another preset has it as its base or a list uses it (names them)."""
    name, path = _preset_file(context, found)
    if not os.path.isfile(path):
        raise ApiError(404, f"there is no preset {name}")
    tool, catalog = presets_tool(), _catalog(context)
    with context.lock:
        children = [other for other in tool.Sources(context.presets_dir)[0] if other != name
                    and _preset_base(tool, os.path.join(context.presets_dir, other + ".xml"), catalog)[0] == name]
        in_lists = lists_using(context.training_dir, "preset").get(name, [])
        if children or in_lists:
            used = ([f"the base of the presets {', '.join(children)}"] if children else []) + ([f"used by the lists {', '.join(in_lists)}"] if in_lists else [])
            raise ApiError(409, f"preset {name} cannot be deleted: it is " + " and ".join(used))
        moved = move_to_deleted(context.presets_dir, path)
    return 200, {"deleted": name, "moved_to": f"{DELETED}/{os.path.basename(moved)}"}
