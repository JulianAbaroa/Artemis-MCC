"""
test_web.py
Tests of the training lists builder (LS.3, D291): the security of the server (S1-S3, S7, S8), the maps endpoints and the state of the bench.
The server runs in a thread on a free port with temporary copies of Data/Presets and Data/Training: nothing here touches the real Data/.
"""
import http.client
import json
import os
import shutil
import sys
import threading

import pytest

from tdump_writer import Repo

AGENT = os.path.join(Repo(), "Agent")
if AGENT not in sys.path:
    sys.path.insert(0, AGENT)

from artemis.training import load_catalog  # noqa: E402
from artemis.web import api, server  # noqa: E402

DATA = os.path.join(Repo(), "Data")
BENCH = {"state": "ready", "watcher": True, "mcc": True, "bench": True, "configs": ["Map CTF 1v1 First", "Map Unknown 3v3"]}


class Web:
    """A running server with its temporary folders and a client that sends the token unless told otherwise."""

    def __init__(self, folder):
        shutil.copytree(os.path.join(DATA, "Presets"), folder / "presets")
        shutil.copytree(os.path.join(DATA, "Training"), folder / "training")
        self.context = api.Context(str(folder / "presets"), str(folder / "training"), status_fn=lambda: BENCH)
        self.server = server.make_server(0, self.context)
        self.port = self.server.server_address[1]
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()

    @property
    def maps_path(self):
        return self.context.maps_path

    def call(self, method, path, body=None, token=True, host=None, headers=None, raw=None):
        """(status, json or text, headers) of one request. body is a dict (sent as JSON) or raw bytes."""
        connection = http.client.HTTPConnection("127.0.0.1", self.port, timeout=10)
        sent = dict(headers or {})
        sent["Host"] = f"127.0.0.1:{self.port}" if host is None else host
        if token is True:
            sent["X-Artemis-Token"] = self.server.token
        elif token:
            sent["X-Artemis-Token"] = token
        data = raw if raw is not None else (json.dumps(body).encode() if body is not None else None)
        connection.request(method, path, data, sent)
        response = connection.getresponse()
        text = response.read().decode("utf-8")
        result = (response.status, text, dict(response.getheaders()))
        connection.close()
        try:
            return result[0], json.loads(text), result[2]
        except ValueError:
            return result

    def close(self):
        self.server.shutdown()
        self.server.server_close()


@pytest.fixture
def web(tmp_path):
    instance = Web(tmp_path)
    yield instance
    instance.close()


def maps_bytes(web):
    with open(web.maps_path, "rb") as handle:
        return handle.read()


# S1: only 127.0.0.1

def test_listens_on_loopback_only(web):
    assert web.server.server_address[0] == "127.0.0.1"


# S2: the Host header

@pytest.mark.parametrize("host", ["evil.example.com", "evil.example.com:{port}", "127.0.0.1", "localhost", "127.0.0.1:1", "0.0.0.0:{port}", "192.168.1.5:{port}", ""])
def test_foreign_host_is_403(web, host):
    status, body, _ = web.call("GET", "/api/status", host=host.format(port=web.port) if host else "")
    assert status == 403 and "error" in body
    assert web.call("GET", "/", host=host.format(port=web.port) if host else "")[0] == 403


@pytest.mark.parametrize("host", ["127.0.0.1:{port}", "localhost:{port}", "LocalHost:{port}"])
def test_own_host_is_served(web, host):
    assert web.call("GET", "/api/status", host=host.format(port=web.port))[0] == 200


# S3: token on everything that is not GET, and no CORS

@pytest.mark.parametrize("method", ["POST", "PUT", "DELETE", "PATCH", "OPTIONS"])
@pytest.mark.parametrize("token", [False, "wrong", "", "x" * 200])
def test_change_without_a_good_token_is_403(web, method, token):
    before = maps_bytes(web)
    status, body, _ = web.call(method, "/api/maps", {"maps": []}, token=token)
    assert status == 403 and "error" in body
    assert maps_bytes(web) == before


def test_get_needs_no_token_and_changes_nothing(web):
    before = maps_bytes(web)
    assert web.call("GET", "/api/maps", token=False)[0] == 200
    assert web.call("GET", "/api/status", token=False)[0] == 200
    assert maps_bytes(web) == before


def test_token_is_random_per_run_and_in_the_page(tmp_path):
    first, second = Web(tmp_path / "a"), Web(tmp_path / "b")
    try:
        assert first.server.token != second.server.token and len(first.server.token) >= 32
        status, text, _ = first.call("GET", "/")
        assert status == 200 and first.server.token in text and server.TOKEN_PLACEHOLDER not in text
        assert second.server.token not in text
    finally:
        first.close()
        second.close()


def test_no_cors_headers(web):
    answers = [web.call("GET", "/", headers={"Origin": "http://evil.example.com"}), web.call("GET", "/api/maps", headers={"Origin": "http://evil.example.com"}),
               web.call("GET", "/static/app.js"), web.call("GET", "/nothing"), web.call("PUT", "/api/maps", {"maps": "no"}, token=False),
               web.call("OPTIONS", "/api/maps", headers={"Origin": "http://evil.example.com", "Access-Control-Request-Method": "PUT"}),
               web.call("GET", "/api/maps", host="evil.example.com")]
    for _, _, headers in answers:
        assert not [key for key in headers if key.lower().startswith("access-control")]


# S7: static files by exact name

def test_static_files_are_served(web):
    for name in server.STATIC_FILES:
        status, text, _ = web.call("GET", f"/static/{name}")
        assert status == 200 and text
    status, text, headers = web.call("GET", "/")
    assert status == 200 and "<title>" in text and "default-src 'self'" in headers["Content-Security-Policy"]


@pytest.mark.parametrize("path", ["/static/../server.py", "/static/%2e%2e/server.py", "/static/..%2fserver.py", "/static/", "/static", "/static/index.html",
                                  "/static/nothing.js", "/static/sub/app.js", "/server.py", "/etc/passwd", "/../etc/passwd", "/static/app.js/", "/static/App.js",
                                  "/static/app.js%00.css", "//etc/passwd", "/nothing"])
def test_nothing_else_of_the_disk_is_served(web, path):
    assert web.call("GET", path)[0] == 404


# S8: body size and JSON

def test_body_over_one_megabyte_is_400(web):
    before = maps_bytes(web)
    status, body, _ = web.call("PUT", "/api/maps", raw=b'{"maps": [], "x": "' + b"a" * (server.MAX_BODY + 10) + b'"}')
    assert status == 400 and "error" in body
    assert maps_bytes(web) == before
    assert web.call("GET", "/api/status")[0] == 200


def test_invalid_json_is_400(web):
    before = maps_bytes(web)
    for raw in (b"{not json", b"\xff\xfe", b'{"maps": [}'):
        status, body, _ = web.call("PUT", "/api/maps", raw=raw)
        assert status == 400 and "error" in body
    assert web.call("PUT", "/api/maps", raw=b"[]")[0] == 400
    assert web.call("PUT", "/api/maps", {"maps": "x"})[0] == 400
    assert web.call("PUT", "/api/maps", {"maps": [1]})[0] == 400
    assert web.call("PUT", "/api/maps", raw=b"")[0] == 400
    assert maps_bytes(web) == before


def test_chunked_body_is_400(web):
    status, body, _ = web.call("PUT", "/api/maps", raw=b"", headers={"Transfer-Encoding": "chunked"})
    assert status == 400 and "error" in body


# Routes

def test_unknown_route_is_404_and_wrong_method_is_405(web):
    assert web.call("GET", "/api/nothing")[0] == 404
    assert web.call("POST", "/api/nothing", {})[0] == 404
    assert web.call("POST", "/api/status", {})[0] == 405
    assert web.call("DELETE", "/api/maps")[0] == 405
    assert web.call("GET", "/api/maps/")[0] == 404


def test_a_route_can_be_added_to_the_table(web):
    @api.route("GET", r"/api/_test_extra")
    def extra(context, found, body):
        return 200, {"ok": True}

    try:
        assert web.call("GET", "/api/_test_extra")[1] == {"ok": True}
    finally:
        api.ROUTES.pop()


def test_a_bug_of_a_handler_is_a_500_answer(web):
    @api.route("GET", r"/api/_test_bug")
    def bug(context, found, body):
        raise RuntimeError("boom")

    try:
        status, body, _ = web.call("GET", "/api/_test_bug")
        assert status == 500 and "boom" in body["error"]
        assert web.call("GET", "/api/status")[0] == 200
    finally:
        api.ROUTES.pop()


# Status

def test_status_is_the_one_of_the_context(web):
    assert web.call("GET", "/api/status")[1] == BENCH


class FakeGame:
    def __init__(self, watcher, bench):
        self._watcher, self._bench = watcher, bench

    def Watcher(self):
        return self._watcher

    def Bench(self):
        return self._bench


@pytest.mark.parametrize("watcher,bench,state,configs", [
    ({"mccRunning": True}, {"_alive": True, "configs": ["Map A"]}, "ready", ["Map A"]),
    ({"mccRunning": True}, {"_alive": True}, "ready", []),
    ({"mccRunning": True}, {"_alive": False, "configs": ["Map A"]}, "no_mcc", None),
    ({"mccRunning": False}, None, "no_mcc", None),
    (None, None, "watcher_off", None),
    (None, {"_alive": True, "configs": ["Map A"]}, "ready", ["Map A"]),
])
def test_game_status_states(monkeypatch, watcher, bench, state, configs):
    monkeypatch.setattr(api, "game_tool", lambda: FakeGame(watcher, bench))
    result = api.game_status()
    assert result["state"] == state and result["configs"] == configs
    assert result["watcher"] is bool(watcher) and result["bench"] is (state == "ready")


def test_game_tool_loads_the_real_bench_library():
    game = api.game_tool()
    assert callable(game.Watcher) and callable(game.Bench)


# Maps

def test_maps_get_then_put_leaves_the_file_identical(web):
    before = maps_bytes(web)
    assert before == open(os.path.join(DATA, "Training", "Maps.json"), "rb").read()
    status, body, _ = web.call("GET", "/api/maps")
    assert status == 200 and len(body["maps"]) == len(load_catalog(web.maps_path))
    status, written, _ = web.call("PUT", "/api/maps", body)
    assert status == 200 and written == body
    assert maps_bytes(web) == before
    assert not os.path.exists(web.maps_path + ".tmp")


def test_maps_text_is_the_format_of_the_repo():
    before = open(os.path.join(DATA, "Training", "Maps.json"), "rb").read()
    assert api.maps_text(json.loads(before)).encode() == before
    assert api.maps_text([]) == "[\r\n]"


def test_maps_put_writes_and_reloads(web):
    maps = web.call("GET", "/api/maps")[1]["maps"]
    maps.append({"config": "Map New 2v2", "name": "New (2v2)", "size": "2v2", "stage": "Final", "used_by": []})
    maps[0]["name"] = "Renamed"
    status, body, _ = web.call("PUT", "/api/maps", {"maps": maps})
    assert status == 200 and body["maps"][-1]["config"] == "Map New 2v2"
    catalog = load_catalog(web.maps_path)
    assert catalog["Map New 2v2"].size == "2v2" and catalog["Map CTF 1v1 First"].name == "Renamed"
    assert maps_bytes(web).count(b"\r\n") == len(catalog) + 1 and not maps_bytes(web).endswith(b"\n")
    removed = [row for row in maps if row["config"] != "Map New 2v2"]
    assert web.call("PUT", "/api/maps", {"maps": removed})[0] == 200
    assert "Map New 2v2" not in load_catalog(web.maps_path)


def mutate(web, change):
    maps = web.call("GET", "/api/maps")[1]["maps"]
    change(maps)
    return web.call("PUT", "/api/maps", {"maps": maps})


@pytest.mark.parametrize("change", [
    lambda maps: maps.append(dict(maps[0])),
    lambda maps: maps.append({**maps[0], "config": "Map Other 1v1"}),
    lambda maps: maps[0].update(size="big"),
    lambda maps: maps[0].update(stage="Middle"),
    lambda maps: maps[0].update(name=""),
    lambda maps: maps[0].update(config=3),
    lambda maps: maps[0].update(extra="x"),
    lambda maps: maps[0].pop("size"),
])
def test_maps_put_that_is_not_a_catalog_is_400_and_changes_nothing(web, change):
    before = maps_bytes(web)
    status, body, _ = mutate(web, change)
    assert status == 400 and "error" in body and web.maps_path not in body["error"]
    assert maps_bytes(web) == before
    assert not os.path.exists(web.maps_path + ".tmp")


def test_maps_used_by_names_the_lists(web):
    expected = {}
    for file in os.listdir(web.context.training_dir):
        if file.endswith(".json") and file != "Maps.json":
            for entry in json.load(open(os.path.join(web.context.training_dir, file), encoding="utf-8"))["entries"]:
                expected.setdefault(entry["map"], set()).add(file[:-5])
    assert expected
    shutil.copy(os.path.join(web.context.training_dir, "ctf_curriculo_inicial.json"), os.path.join(web.context.training_dir, "second.json"))
    with open(os.path.join(web.context.training_dir, "broken.json"), "w") as handle:
        handle.write("{not json")
    body = web.call("GET", "/api/maps")[1]
    for row in body["maps"]:
        names = set(expected.get(row["config"], ()))
        assert set(row["used_by"]) == (names | {"second"} if names else names)
        assert row["used_by"] == sorted(row["used_by"])


def test_maps_get_of_a_broken_catalog_is_409(web):
    with open(web.maps_path, "w") as handle:
        handle.write("{broken")
    status, body, _ = web.call("GET", "/api/maps")
    assert status == 409 and "Maps.json" in body["error"]


def test_the_real_data_is_untouched():
    """The tests write in temporary copies: the folders of the context are never the ones of the repo."""
    assert api.default_context().training_dir == os.path.join(DATA, "Training")
    assert api.default_context().presets_dir == os.path.join(DATA, "Presets")
