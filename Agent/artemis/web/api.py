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
import subprocess
import threading
import time
import uuid
from dataclasses import dataclass, field
from typing import Callable

from ..training.catalog import MAPS_PATH, ROOT, load_catalog
from ..training.lists import ListError, StaleListError, load_list, save_list

MAP_FIELDS = ("config", "name", "size", "stage")
MAPS_FILE = "Maps.json"
TOOLS = os.path.join(ROOT, "Tools")
DELETED = "_deleted"
ENV_CHECK = os.path.join(ROOT, "Agent", "scripts", "env_check.py")
ENTRY_SECONDS = 150
MAX_JOBS = 50
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


def bench_queue(job_id, text):
    """Queues a compiled scenario for the bench as `game.py run` does: exports the presets, asks the watcher to reload them and writes the
    scenario to build/game/inbox. The only request to the bench (no subprocess). Raises RuntimeError with what failed."""
    game = game_tool()
    if not game.Watcher():
        raise RuntimeError("the game watcher is not running (open Tools/game_watcher.bat)")
    if presets_tool().Export():
        raise RuntimeError("the presets do not compile (see the Presets tab)")
    ok, message = game.Send("presets")
    if not ok:
        raise RuntimeError(f"the watcher did not reload the presets: {message or 'no answer in 60 s'}")
    target = os.path.join(game.GAME, "inbox", job_id + ".txt")
    os.makedirs(os.path.dirname(target), exist_ok=True)
    with open(target + ".tmp", "w", encoding="utf-8", newline="\n") as handle:
        handle.write(text)
    os.replace(target + ".tmp", target)


def bench_report(job_id):
    """The report of a bench run, or None while it is not written."""
    return game_tool().ReadJson(os.path.join("report", job_id + ".json"))


def bench_state():
    """state.json of the bench (phase, configs...), or None."""
    return game_tool().Bench()


def env_launch(argv, on_line):
    """Runs env_check as a process (a list of arguments, never a shell) and sends its output lines to on_line. Returns the process."""
    process = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace", cwd=ROOT, shell=False)

    def pump():
        for line in process.stdout:
            on_line(line.rstrip())

    threading.Thread(target=pump, daemon=True).start()
    return process


@dataclass
class Context:
    """The folders the API reads and writes, where it asks for the state of the bench and the two things it can launch (D291 S6): the scenario
    of the bench (queue_fn, report_fn, bench_fn) and env_check (launch_fn). Tests give temporary copies of Data/ and fakes for the launchers."""
    presets_dir: str
    training_dir: str
    status_fn: Callable = game_status
    queue_fn: Callable = bench_queue
    report_fn: Callable = bench_report
    bench_fn: Callable = bench_state
    launch_fn: Callable = env_launch
    poll_seconds: float = 1.0
    start_seconds: float = 180.0
    lock: threading.RLock = field(default_factory=threading.RLock)
    jobs: dict = field(default_factory=dict)

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


# Lists

def _list_path(context, found):
    name = valid_name(found.group(1))
    if name.lower() + ".json" == MAPS_FILE.lower():
        raise ApiError(400, f"{name!r} is not a valid name for a list (Maps.json is the catalog of the maps)")
    return name, safe_path(context.training_dir, name, ".json")


def _list_files(context):
    """Names of the lists of the folder: the .json files that are not the catalog and have a valid name."""
    pattern = presets_tool().NAME
    return [file[:-5] for file in sorted(os.listdir(context.training_dir))
            if file.endswith(".json") and file.lower() != MAPS_FILE.lower() and pattern.fullmatch(file[:-5]) and os.path.isfile(os.path.join(context.training_dir, file))]


def _read_list(context, name, path):
    """{name, order, entries, stale, error} of a list file. A stale list (a preset changed since it was saved) and an invalid one are shown, not
    refused: stale is the reason (Save regenerates it) and error is what is wrong with it."""
    try:
        with open(path, encoding="utf-8") as handle:
            data = json.load(handle)
        rows = data["entries"]
        entries = [{"map": row["map"], "preset": row.get("preset"), "roster": row.get("roster"), "rounds": row.get("rounds", 1), "weight": row.get("weight", 1.0)} for row in rows]
        order = data["order"]
    except (OSError, ValueError, KeyError, TypeError, AttributeError) as error:
        raise ApiError(409, f"{name}.json is not a list: {error!r}") from None
    stale = problem = None
    try:
        load_list(path, maps_path=context.maps_path, presets_folder=context.presets_dir)
    except StaleListError as error:
        stale = str(error)
    except (ListError, OSError, ValueError) as error:
        problem = str(error).replace(path, name + ".json")
    return {"name": name, "order": order, "entries": entries, "presets": sorted({e["preset"] for e in entries if e["preset"]}), "stale": stale, "error": problem}


@route("GET", r"/api/lists")
def get_lists(context, found, body):
    rows = []
    for name in _list_files(context):
        try:
            data = _read_list(context, name, os.path.join(context.training_dir, name + ".json"))
            rows.append({"name": name, "order": data["order"], "entries": len(data["entries"]), "stale": data["stale"], "error": data["error"]})
        except ApiError as error:
            rows.append({"name": name, "order": None, "entries": 0, "stale": None, "error": error.message})
    return 200, {"lists": rows}


@route("GET", r"/api/lists/(.+)")
def get_list(context, found, body):
    name, path = _list_path(context, found)
    if not os.path.isfile(path):
        raise ApiError(404, f"there is no list {name}")
    return 200, _read_list(context, name, path)


@route("PUT", r"/api/lists/(.+)")
def put_list(context, found, body):
    """Writes the list with save_list: it is validated like load_list, and the compiled text of its presets is saved again (that clears stale)."""
    name, path = _list_path(context, found)
    if not isinstance(body, dict) or not isinstance(body.get("entries"), list) or not all(isinstance(row, dict) for row in body["entries"]):
        raise ApiError(400, 'expected {"order": "sequence" | "weighted", "entries": [{map, preset, roster, rounds, weight}, ...]}')
    entries = [{**row, "weight": float(row["weight"])} if isinstance(row.get("weight"), int) and not isinstance(row["weight"], bool) else row for row in body["entries"]]
    with context.lock:
        try:
            save_list(path, name, body.get("order"), entries, maps_path=context.maps_path, presets_folder=context.presets_dir)
        except (ListError, TypeError) as error:
            raise ApiError(400, str(error).replace(path, name + ".json")) from None
        return 200, _read_list(context, name, path)


@route("POST", r"/api/lists/(.+)/delete")
def delete_list(context, found, body):
    name, path = _list_path(context, found)
    if not os.path.isfile(path):
        raise ApiError(404, f"there is no list {name}")
    with context.lock:
        moved = move_to_deleted(context.training_dir, path)
    return 200, {"deleted": name, "moved_to": f"{DELETED}/{os.path.basename(moved)}"}


# Tests (D291 S6): one entry through the bench, a whole list through env_check

def bench_roster(roster):
    """The roster of a list entry (relative to the local player, D287) as the bench writes it: the local player is on team 0, so the bots of its
    team are "auto" and the rival ones are team 1 (as the ls2 scenarios)."""
    return {"auto": roster.get("auto", 0), "1": roster["rival"]}


def _preset_word(preset):
    """The preset of an entry for the bench: None (no change) is left out by the caller, "" (the base values) is none."""
    return preset or None


def entry_scenario(list_name, index, entry) -> dict:
    """The bench scenario that tests one entry: a match on the config of its map that starts with the entry (preset, map and bots at load) and
    has it as the setup of the next round, checked with no_errors and setup_ok."""
    roster = entry.get("roster")
    fields = {}
    if entry.get("preset") is not None:
        fields["preset"] = _preset_word(entry["preset"])
    fields["map"] = entry["map"]
    if roster is not None:
        fields["roster"] = bench_roster(roster)
    match = {"name": f"{list_name} entry {index}", "config": entry["map"], **fields, "setups": [dict(fields)],
             "checks": ["no_errors", "setup_ok"], "max_seconds": ENTRY_SECONDS}
    return {"name": f"ls3_{list_name}_{index}", "matches": [match]}


def list_scenario(list_name, first, episodes) -> dict:
    """The bench scenario env_check plays on (as ls2_env_list): the match of the first entry with a bot, long enough for the episodes."""
    match = {"name": f"env_check --list {list_name}", "config": first["map"], "map": first["map"], "bots": [1], "events": True,
             "checks": ["no_errors"], "max_seconds": min(7200, 240 + 110 * episodes)}
    if first.get("preset") is not None:
        match["preset"] = _preset_word(first["preset"])
    return {"name": f"ls3_{list_name}_list", "matches": [match]}


def _job_id(prefix=""):
    return prefix + time.strftime("%Y%m%d-%H%M%S-", time.gmtime()) + uuid.uuid4().hex[:4]


def _refresh(context, job):
    """Brings a job up to date from the report (an entry) or the process (a list)."""
    if job["state"] != "running":
        return
    job["seconds"] = round(time.time() - job["started"], 1)
    if job["kind"] == "entry":
        report = context.report_fn(job["id"])
        if report and report.get("done"):
            matches = [{"name": m.get("name"), "result": m.get("result"), "end": m.get("end"), "ticks": m.get("ticks"),
                        "failed": [f"{c.get('name')}: {c.get('detail')}" for c in m.get("checks", []) if not c.get("ok")], "log": m.get("log", [])[:8]}
                       for m in report.get("matches", [])]
            job.update(state="done", matches=matches, message=report.get("message", ""),
                       passed=report.get("result") == "ok" and not any(m["failed"] for m in matches))
        elif job["seconds"] > ENTRY_SECONDS + 240:
            job.update(state="error", passed=False, message=f"no report after {int(job['seconds'])} s: is MCC on the custom games screen?")
    else:
        process = job.get("process")
        code = process.poll() if process else None
        if code is not None:
            job.update(state="done", passed=code == 0, message=f"env_check exited with {code}")


def _public(job):
    shown = {key: value for key, value in job.items() if key not in ("process", "started")}
    if "output" in shown:
        shown["output"] = list(shown["output"])
    return shown


def _start_job(context, kind, list_name, scenario, **extra):
    """Registers a job and queues its scenario in a thread (the watcher can take up to 60 s to answer). 409 when another test is running:
    the bench plays one scenario at a time. Returns the job."""
    status = context.status_fn()
    if status["state"] != "ready":
        raise ApiError(409, "the game watcher is not running (open Tools/game_watcher.bat)" if status["state"] == "watcher_off"
                       else "MCC is not open with Artemis: open it to run tests")
    try:
        text, _ = game_tool().Compile(scenario, status.get("configs"))
    except ValueError as error:
        raise ApiError(400, str(error)) from None
    with context.lock:
        for other in context.jobs.values():
            _refresh(context, other)
            if other["state"] in ("queued", "running"):
                raise ApiError(409, f"the test {other['id']} is still running: wait for it to finish")
        job = {"id": _job_id("list-" if kind == "list" else ""), "kind": kind, "list": list_name, "state": "queued", "passed": None, "message": "",
               "started": time.time(), "seconds": 0.0, "scenario": scenario["name"], **extra}
        if kind == "list":
            job["output"] = []
        context.jobs[job["id"]] = job
        for old in [key for key, value in context.jobs.items() if value["state"] in ("done", "error")][:-MAX_JOBS]:
            del context.jobs[old]
    threading.Thread(target=_run_job, args=(context, job, text), daemon=True).start()
    return job


def _keep(output, line):
    """The last 200 lines of the output of a process."""
    output.append(line)
    del output[:-200]


def _run_job(context, job, text):
    try:
        context.queue_fn(job["id"], text)
        if job["kind"] == "list":
            deadline = time.time() + context.start_seconds
            while (context.bench_fn() or {}).get("phase") != "Running":
                if time.time() > deadline:
                    raise RuntimeError(f"the match did not start in {int(context.start_seconds)} s: is MCC on the custom games screen?")
                time.sleep(context.poll_seconds)
            job["process"] = context.launch_fn(job["command"], lambda line: _keep(job["output"], line))
        job["started"] = time.time()
        job["state"] = "running"
    except (RuntimeError, OSError, SystemExit) as error:
        job.update(state="error", passed=False, message=str(error))


def _entry_of(context, body):
    if not isinstance(body, dict):
        raise ApiError(400, 'expected {"list": name, "index": n}')
    name = valid_name(body.get("list"))
    path = safe_path(context.training_dir, name, ".json")
    if not os.path.isfile(path):
        raise ApiError(404, f"there is no list {name}")
    return name, path


@route("POST", r"/api/test/entry")
def test_entry(context, found, body):
    name, path = _entry_of(context, body)
    entries = _read_list(context, name, path)["entries"]
    index = body.get("index")
    if isinstance(index, bool) or not isinstance(index, int) or not 0 <= index < len(entries):
        raise ApiError(400, f"index must be an integer from 0 to {len(entries) - 1}")
    job = _start_job(context, "entry", name, entry_scenario(name, index, entries[index]), index=index)
    return 200, {"id": job["id"], "state": job["state"]}


@route("POST", r"/api/test/list")
def test_list(context, found, body):
    name, path = _entry_of(context, body)
    episodes = body.get("episodes")
    if isinstance(episodes, bool) or not isinstance(episodes, int) or not 1 <= episodes <= 50:
        raise ApiError(400, "episodes must be an integer from 1 to 50")
    data = _read_list(context, name, path)
    if data["stale"] or data["error"]:
        raise ApiError(409, f"the list cannot be tested: {data['stale'] or data['error']} (save it first)")
    command = [sys.executable, ENV_CHECK, "--list", path, "--policy", "seek", "--episodes", str(episodes)]
    job = _start_job(context, "list", name, list_scenario(name, data["entries"][0], episodes), episodes=episodes, command=command)
    return 200, {"id": job["id"], "state": job["state"], "command": command}


@route("GET", r"/api/test/(.+)")
def get_test(context, found, body):
    with context.lock:
        job = context.jobs.get(found.group(1))
        if job is None:
            raise ApiError(404, f"there is no test {found.group(1)}")
        _refresh(context, job)
        return 200, _public(job)
