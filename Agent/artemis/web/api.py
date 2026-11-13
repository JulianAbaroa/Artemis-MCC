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
from dataclasses import dataclass, field
from typing import Callable

from ..training.catalog import MAPS_PATH, ROOT, load_catalog

MAP_FIELDS = ("config", "name", "size", "stage")
MAPS_FILE = "Maps.json"
_GAME_TOOL = os.path.join(ROOT, "Tools", "game.py")
_game = None


class ApiError(Exception):
    """An answer that is an error: status (400, 403, 404, 409...) and the message the page shows."""

    def __init__(self, status, message):
        super().__init__(message)
        self.status = status
        self.message = message


def game_tool():
    """Tools/game.py as a library (the bench files of the game watcher), loaded once from the repo."""
    global _game
    if _game is None:
        tools = os.path.dirname(_GAME_TOOL)
        if tools not in sys.path:
            sys.path.insert(0, tools)
        spec = importlib.util.spec_from_file_location("artemis_game_tool", _GAME_TOOL)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        _game = module
    return _game


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


def lists_using_maps(training_dir) -> dict:
    """{map config: [names of the lists that use it]} from the files of the lists in the folder (read as they are: a broken list is skipped)."""
    used = {}
    for file in sorted(os.listdir(training_dir)):
        path = os.path.join(training_dir, file)
        if not file.endswith(".json") or file == MAPS_FILE or not os.path.isfile(path):
            continue
        try:
            with open(path, encoding="utf-8") as handle:
                entries = json.load(handle)["entries"]
            configs = {entry["map"] for entry in entries}
        except (OSError, ValueError, KeyError, TypeError):
            continue
        for config in configs:
            used.setdefault(config, []).append(file[:-len(".json")])
    return used


def _maps_payload(context):
    try:
        catalog = load_catalog(context.maps_path)
    except (OSError, ValueError) as error:
        raise ApiError(409, f"{MAPS_FILE} is not valid: {error}") from None
    used = lists_using_maps(context.training_dir)
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
