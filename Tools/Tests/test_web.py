"""
test_web.py
Tests of the training lists builder (LS.3, D291): the security of the server (S1-S3, S7, S8), the maps endpoints and the state of the bench.
The server runs in a thread on a free port with temporary copies of Data/Presets and Data/Training: nothing here touches the real Data/.
"""
import http.client
import json
import os
import re
import shutil
import sys
import time
from urllib.parse import quote
import threading

import pytest

from tdump_writer import Repo

AGENT = os.path.join(Repo(), "Agent")
if AGENT not in sys.path:
    sys.path.insert(0, AGENT)

from artemis.training import load_catalog  # noqa: E402
from artemis.web import api, server  # noqa: E402

sys.path.insert(0, os.path.join(Repo(), "Tools"))
import presets  # noqa: E402

DATA = os.path.join(Repo(), "Data")
BENCH = {"state": "ready", "watcher": True, "mcc": True, "bench": True, "configs": ["Map CTF 1v1 First", "Map Unknown 3v3"]}


class FakeProcess:
    """A process of env_check that has not finished until the test sets code."""

    def __init__(self):
        self.code = None

    def poll(self):
        return self.code


class Web:
    """A running server with its temporary folders and a client that sends the token unless told otherwise."""

    def __init__(self, folder):
        shutil.copytree(os.path.join(DATA, "Presets"), folder / "presets")
        shutil.copytree(os.path.join(DATA, "Training"), folder / "training")
        self.status = dict(BENCH)
        self.queued, self.launched, self.reports, self.phase = [], [], {}, "Running"
        self.process = FakeProcess()
        self.fail_queue = None
        self.context = api.Context(str(folder / "presets"), str(folder / "training"), status_fn=lambda: self.status, queue_fn=self._queue,
                                   report_fn=lambda job_id: self.reports.get(job_id), bench_fn=lambda: {"phase": self.phase},
                                   launch_fn=self._launch, poll_seconds=0.01, start_seconds=0.5)
        self.server = server.make_server(0, self.context)
        self.port = self.server.server_address[1]
        self.thread = threading.Thread(target=self.server.serve_forever, kwargs={"poll_interval": 0.02}, daemon=True)
        self.thread.start()

    def _queue(self, job_id, text):
        if self.fail_queue:
            raise RuntimeError(self.fail_queue)
        self.queued.append((job_id, text))

    def _launch(self, argv, on_line):
        self.launched.append(argv)
        on_line("episode 0 agent 0x1: 5 steps")
        return self.process

    def wait(self, job_id, states=("running", "done", "error")):
        """The answer of /api/test/<id> once its state is one of `states` (the jobs queue in a thread)."""
        for _ in range(300):
            status, body, _ = self.call("GET", f"/api/test/{job_id}")
            if status == 200 and body["state"] in states:
                return body
            time.sleep(0.02)
        raise AssertionError(f"the test {job_id} stayed {body['state']}")

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


# Names, paths and deleting (S4, S5)

BAD_NAMES = ["../x", "..", "a/b", "A", "a b", "x" * 32, "a.b", "%2e%2e", "a\n", "ñ", "..%2fx", "a\\b"]


@pytest.mark.parametrize("name", BAD_NAMES)
def test_valid_name_and_safe_path_refuse_names_that_are_not_names(tmp_path, name):
    with pytest.raises(api.ApiError) as error:
        api.valid_name(name)
    assert error.value.status == 400
    with pytest.raises(api.ApiError):
        api.safe_path(str(tmp_path), name, ".xml")


def test_safe_path_builds_the_path_inside_the_folder(tmp_path):
    assert api.safe_path(str(tmp_path), "a_1", ".xml") == os.path.join(str(tmp_path), "a_1.xml")
    assert api.valid_name("x" * 31) == "x" * 31
    for bad in (None, 3, ["a"], {"a": 1}):
        with pytest.raises(api.ApiError):
            api.valid_name(bad)
    link = tmp_path / "out"
    link.mkdir()
    inner = tmp_path / "inner"
    os.symlink(str(link), str(inner))
    with pytest.raises(api.ApiError):
        api.safe_path(str(inner), "..", "")


def test_move_to_deleted_never_overwrites_or_removes(tmp_path):
    names = []
    for round_ in range(3):
        file = tmp_path / "p.xml"
        file.write_text(f"v{round_}")
        names.append(api.move_to_deleted(str(tmp_path), str(file)))
        assert not file.exists()
    assert len(set(names)) == 3 and all(os.path.dirname(name) == str(tmp_path / "_deleted") for name in names)
    assert sorted(open(name).read() for name in names) == ["v0", "v1", "v2"]
    assert os.path.basename(names[0]) == "p.xml" and os.path.basename(names[1]).startswith("p_") and time.strftime("%Y") in os.path.basename(names[1])


# Catalog

def test_catalog_endpoint_expands_the_groups_and_fields(web):
    status, body, _ = web.call("GET", "/api/catalog")
    assert status == 200
    catalog = presets.Catalog(os.path.join(web.context.presets_dir, "Catalog.xml"))
    assert list(body["enums"]) == list(catalog.Enums) and body["enums"]["bool_trait"] == ["unchanged", "disabled", "enabled"]
    assert [group["name"] for group in body["groups"]] == list(catalog.Groups)
    names = {group["name"]: group for group in body["groups"]}
    assert names["team"]["indices"] == [{"name": "index", "count": 8}] and names["loadout"]["indices"][1] == {"name": "index", "count": 5}
    assert names["trait_set"]["check"] == "megalo_trait_set" and names["trait_set"]["traits"] and not names["general"]["traits"]
    assert [f["name"] for f in names["base_traits"]["fields"]] == [f["name"] for f in names["red_traits"]["fields"]]
    assert names["base_traits"]["when"] == "tick" and names["red_traits"]["fields"][0]["when"] == "pickup"
    field = next(f for f in names["base_traits"]["fields"] if f["name"] == "jump_height")
    assert field["type"] == "s32" and field["unit"] == "%" and field["enum"] is None and field["min"] == -(1 << 31) and field["doc"]
    gravity = next(f for f in names["base_traits"]["fields"] if f["name"] == "gravity")
    assert gravity["enum"] == "player_gravity" and gravity["min"] is None
    assert sum(len(group["fields"]) for group in body["groups"] if not group["traits"]) + len(names["base_traits"]["fields"]) == 77


# Presets

REAL_PRESETS = ["artemis_ctf_base", "artemis_ctf_colors", "artemis_ctf_fast", "artemis_ctf_rounds"]


def preset_path(web, name):
    return os.path.join(web.context.presets_dir, name + ".xml")


def preset_bytes(web, name):
    with open(preset_path(web, name), "rb") as handle:
        return handle.read()


def tree(web):
    """Every file of the presets folder with its bytes: what a refused request must leave as it was."""
    return {os.path.join(root, file): open(os.path.join(root, file), "rb").read() for root, _, files in os.walk(web.context.presets_dir) for file in files}


def compiled(web, name):
    return presets.CompilePreset(name, web.context.presets_dir, presets.Catalog(os.path.join(web.context.presets_dir, "Catalog.xml")))[0]


def test_presets_list(web):
    status, body, _ = web.call("GET", "/api/presets")
    rows = {row["name"]: row for row in body["presets"]}
    assert status == 200 and sorted(rows) == REAL_PRESETS
    assert rows["artemis_ctf_rounds"]["base"] == "artemis_ctf_fast" and rows["artemis_ctf_base"]["base"] == ""
    assert rows["artemis_ctf_fast"]["doc"].startswith("Short rounds")


@pytest.mark.parametrize("name", REAL_PRESETS)
def test_preset_get_then_put_leaves_the_file_and_the_compiled_lines_identical(web, name):
    before, lines = preset_bytes(web, name), compiled(web, name)
    status, body, _ = web.call("GET", f"/api/presets/{name}")
    assert status == 200 and body["name"] == name and body["writes"] == len([l for l in lines.splitlines() if l.startswith("write ")])
    status, written, _ = web.call("PUT", f"/api/presets/{name}", {"base": body["base"], "doc": body["doc"], "own": body["own"]})
    assert status == 200 and written["lines"] == lines.splitlines() and written["writes"] == body["writes"]
    assert preset_bytes(web, name) == before
    assert compiled(web, name) == lines
    assert not [file for file in os.listdir(web.context.presets_dir) if file.endswith(".tmp")]


def test_preset_resolved_tells_where_each_value_comes_from(web):
    body = web.call("GET", "/api/presets/artemis_ctf_rounds")[1]
    assert body["base"] == "artemis_ctf_fast" and body["own"] == {"general.round_limit": "4", "general.time_limit_minutes": "1"}
    resolved = body["resolved"]
    assert resolved["general.round_limit"] == {"value": "4", "from": "artemis_ctf_rounds"}
    assert resolved["general.time_limit_minutes"] == {"value": "1", "from": "artemis_ctf_rounds"}
    assert resolved["base_traits.gravity"] == {"value": "50%", "from": "artemis_ctf_fast"}
    assert resolved["base_traits.jump_height"] == {"value": "100", "from": "artemis_ctf_base"}
    assert body["checks"] == ["megalo_trait_set 0"]
    assert web.call("GET", "/api/presets/artemis_ctf_base")[1]["checks"] == []


def test_preset_put_creates_a_new_preset_in_the_repo_format(web):
    body = {"base": "artemis_ctf_fast", "doc": "A test.", "own": {"respawn.time_seconds": 7, "general.round_limit": "2", "base_traits.gravity": "100%"}}
    status, answer, _ = web.call("PUT", "/api/presets/my_test", body)
    assert status == 200 and answer["writes"] > 5 and answer["checks"] == ["megalo_trait_set 0"] and answer["lines"][0] == "preset my_test"
    assert preset_bytes(web, "my_test") == (
        b'<?xml version="1.0" encoding="utf-8"?>\r\n<preset base="artemis_ctf_fast" doc="A test.">\r\n  <general round_limit="2" />\r\n'
        b'  <respawn time_seconds="7" />\r\n  <base_traits gravity="100%" />\r\n</preset>')
    got = web.call("GET", "/api/presets/my_test")[1]
    assert got["resolved"]["respawn.time_seconds"] == {"value": "7", "from": "my_test"}
    assert got["resolved"]["general.time_limit_minutes"]["from"] == "artemis_ctf_fast"
    assert "my_test" in [row["name"] for row in web.call("GET", "/api/presets")[1]["presets"]]
    updated = web.call("PUT", "/api/presets/my_test", {"base": "", "doc": "", "own": {"general.round_limit": "9"}})
    assert updated[0] == 200 and preset_bytes(web, "my_test").endswith(b'<general round_limit="9" />\r\n</preset>') and b"base=" not in preset_bytes(web, "my_test")


def test_preset_put_keeps_the_comment_of_the_file(web):
    web.call("PUT", "/api/presets/artemis_ctf_base", {"base": "", "doc": "changed", "own": {"general.round_limit": "5"}})
    assert b"<!-- The Artemis CTF gametype as measured in Forge" in preset_bytes(web, "artemis_ctf_base")


@pytest.mark.parametrize("name,body,text", [
    ("artemis_ctf_fast", {"own": {"base_traits.gravity": "not_a_value"}}, "not a valid value"),
    ("artemis_ctf_fast", {"own": {"general.round_limit": "99999999"}}, "outside the range"),
    ("artemis_ctf_fast", {"own": {"general.nothing": "1"}}, "no field"),
    ("artemis_ctf_fast", {"own": {"nothing.x": "1"}}, "not a value of a preset"),
    ("artemis_ctf_fast", {"own": {"team.primary_color": "1"}}, "indices"),
    ("artemis_ctf_fast", {"own": {"team[8].primary_color": "1"}}, "indices"),
    ("artemis_ctf_fast", {"base": "no_such_base", "own": {}}, "does not exist"),
    ("artemis_ctf_fast", {"base": "artemis_ctf_fast", "own": {}}, "cycle"),
    ("artemis_ctf_base", {"base": "artemis_ctf_rounds", "own": {}}, "cycle"),
    ("artemis_ctf_fast", {"base": "../x", "own": {}}, "not a valid name"),
    ("artemis_ctf_fast", {"base": "A", "own": {}}, "not a valid name"),
    ("artemis_ctf_fast", {"own": {"general.round_limit": ""}}, "text or a number"),
    ("artemis_ctf_fast", {"own": {"general.round_limit": True}}, "text or a number"),
    ("artemis_ctf_fast", {"own": {"general.round_limit": None}}, "text or a number"),
    ("artemis_ctf_fast", {"own": ["x"]}, "expected"),
    ("artemis_ctf_fast", {"own": {}, "doc": 3}, "expected"),
    ("brand_new", {"base": "brand_new", "own": {}}, "cycle"),
])
def test_preset_put_that_does_not_compile_is_400_and_changes_nothing(web, name, body, text):
    before = tree(web)
    status, answer, _ = web.call("PUT", f"/api/presets/{name}", body)
    assert status == 400 and text in answer["error"] and ".tmp" not in answer["error"]
    assert tree(web) == before


def test_preset_put_error_names_the_file_and_the_field(web):
    """The error of an enum value names the field and the file the user is saving, not the temporary file."""
    answer = web.call("PUT", "/api/presets/artemis_ctf_fast", {"own": {"base_traits.gravity": "zzz"}})[1]
    assert "artemis_ctf_fast.xml" in answer["error"] and "gravity" in answer["error"]


@pytest.mark.parametrize("name", BAD_NAMES)
def test_preset_names_that_are_not_names_are_400_in_every_endpoint(web, name):
    before = tree(web)
    for method, suffix, body in (("GET", "", None), ("PUT", "", {"own": {}}), ("POST", "/delete", {})):
        status, answer, _ = web.call(method, f"/api/presets/{quote(name, safe='/')}{suffix}", body)
        assert status == 400 and "error" in answer, (method, name)
    assert tree(web) == before
    assert not os.path.exists(os.path.join(web.context.presets_dir, "..", "x.xml"))


def test_preset_that_does_not_exist_is_404(web):
    assert web.call("GET", "/api/presets/nothing")[0] == 404
    assert web.call("POST", "/api/presets/nothing/delete", {})[0] == 404


def test_preset_changes_need_the_token(web):
    before = tree(web)
    for method, path in (("PUT", "/api/presets/artemis_ctf_fast"), ("POST", "/api/presets/artemis_ctf_rounds/delete")):
        assert web.call(method, path, {"own": {}}, token=False)[0] == 403
    assert tree(web) == before


def test_preset_get_of_one_that_does_not_compile_is_409(web):
    with open(preset_path(web, "artemis_ctf_rounds"), "w") as handle:
        handle.write('<preset base="gone"><general round_limit="1" /></preset>')
    status, body, _ = web.call("GET", "/api/presets/artemis_ctf_rounds")
    assert status == 409 and "gone" in body["error"]


def test_preset_delete_moves_the_file_to_deleted(web):
    web.call("PUT", "/api/presets/scratch", {"base": "artemis_ctf_fast", "doc": "x", "own": {"general.round_limit": "2"}})
    original = preset_bytes(web, "scratch")
    status, body, _ = web.call("POST", "/api/presets/scratch/delete", {})
    assert status == 200 and body["deleted"] == "scratch" and body["moved_to"] == "_deleted/scratch.xml"
    assert not os.path.exists(preset_path(web, "scratch"))
    with open(os.path.join(web.context.presets_dir, "_deleted", "scratch.xml"), "rb") as handle:
        assert handle.read() == original
    assert web.call("GET", "/api/presets/scratch")[0] == 404
    assert "scratch" not in [row["name"] for row in web.call("GET", "/api/presets")[1]["presets"]]
    web.call("PUT", "/api/presets/scratch", {"base": "artemis_ctf_fast", "doc": "again", "own": {}})
    status, body, _ = web.call("POST", "/api/presets/scratch/delete", {})
    assert status == 200 and body["moved_to"] != "_deleted/scratch.xml"
    assert len(os.listdir(os.path.join(web.context.presets_dir, "_deleted"))) == 2


def test_preset_delete_of_a_base_or_a_preset_of_a_list_is_409(web):
    before = tree(web)
    status, body, _ = web.call("POST", "/api/presets/artemis_ctf_fast/delete", {})
    assert status == 409 and "artemis_ctf_rounds" in body["error"] and "base" in body["error"]
    assert web.call("POST", "/api/presets/artemis_ctf_base/delete", {})[0] == 409
    status, body, _ = web.call("POST", "/api/presets/artemis_ctf_rounds/delete", {})
    assert status == 409 and "ctf_curriculo_inicial" in body["error"] and os.path.exists(preset_path(web, "artemis_ctf_rounds"))
    web.call("PUT", "/api/presets/loose", {"base": "", "doc": "", "own": {}})
    with open(os.path.join(web.context.training_dir, "mine.json"), "w") as handle:
        json.dump({"entries": [{"map": "Map CTF 1v1 First", "preset": "loose"}, {"map": "x", "preset": None}]}, handle)
    status, body, _ = web.call("POST", "/api/presets/loose/delete", {})
    assert status == 409 and "mine" in body["error"] and os.path.exists(preset_path(web, "loose"))
    os.remove(os.path.join(web.context.training_dir, "mine.json"))
    assert web.call("POST", "/api/presets/loose/delete", {})[0] == 200
    after = tree(web)
    assert {path: data for path, data in after.items() if "_deleted" not in path and "loose" not in path} == {path: data for path, data in before.items()}


def test_presets_folder_gets_nothing_but_presets_and_deleted(web):
    web.call("PUT", "/api/presets/p1", {"own": {"general.round_limit": "1"}})
    web.call("POST", "/api/presets/p1/delete", {})
    files = {os.path.relpath(os.path.join(root, file), web.context.presets_dir) for root, _, names in os.walk(web.context.presets_dir) for file in names}
    assert files == {"Catalog.xml", *(name + ".xml" for name in REAL_PRESETS), os.path.join("_deleted", "p1.xml")}


# Lists

LIST = "ctf_curriculo_inicial"


def list_path(web, name=LIST):
    return os.path.join(web.context.training_dir, name + ".json")


def list_bytes(web, name=LIST):
    with open(list_path(web, name), "rb") as handle:
        return handle.read()


def list_body(web, name=LIST):
    data = web.call("GET", f"/api/lists/{name}")[1]
    return {"order": data["order"], "entries": data["entries"]}


def test_lists_index_and_get(web):
    status, body, _ = web.call("GET", "/api/lists")
    assert status == 200 and [row["name"] for row in body["lists"]] == [LIST]
    assert body["lists"][0]["entries"] == 3 and body["lists"][0]["stale"] is None and body["lists"][0]["error"] is None
    data = web.call("GET", f"/api/lists/{LIST}")[1]
    assert data["name"] == LIST and data["order"] == "sequence" and data["stale"] is None and data["error"] is None
    assert data["presets"] == ["artemis_ctf_rounds"] and data["entries"][0]["roster"] == {"rival": 1}
    assert set(data["entries"][1]) == {"map", "preset", "roster", "rounds", "weight"}
    assert web.call("GET", "/api/lists/nothing")[0] == 404


def test_lists_get_then_put_is_what_save_list_writes(web):
    before = list_bytes(web)
    assert before == open(os.path.join(DATA, "Training", LIST + ".json"), "rb").read()
    status, body, _ = web.call("PUT", f"/api/lists/{LIST}", list_body(web))
    assert status == 200 and body["stale"] is None and body["entries"] == web.call("GET", f"/api/lists/{LIST}")[1]["entries"]
    assert list_bytes(web) == before


def test_lists_put_creates_and_edits(web):
    entries = [{"map": "Map CTF 1v1 First", "preset": None, "roster": {"rival": 1}, "rounds": 2, "weight": 3},
               {"map": "Map CTF 2v2 Final", "preset": "", "roster": None, "rounds": 1, "weight": 1.5},
               {"map": "Map CTF 1v1 Final", "preset": "artemis_ctf_fast", "roster": {"auto": 1, "rival": 2}, "rounds": 4, "weight": 1}]
    status, body, _ = web.call("PUT", "/api/lists/mine", {"order": "weighted", "entries": entries})
    assert status == 200 and body["name"] == "mine" and body["order"] == "weighted" and body["presets"] == ["artemis_ctf_fast"]
    expected = tmp_save(web, "expected", "weighted", [dict(entry, weight=float(entry["weight"])) for entry in entries])
    assert list_bytes(web, "mine").replace(b'"name": "mine"', b'"name": "expected"') == expected
    assert [row["name"] for row in web.call("GET", "/api/lists")[1]["lists"]] == [LIST, "mine"]


def tmp_save(web, name, order, entries):
    from artemis.training import save_list
    path = os.path.join(os.path.dirname(web.context.training_dir), name + ".json")
    save_list(path, name, order, entries, maps_path=web.maps_path, presets_folder=web.context.presets_dir)
    return open(path, "rb").read()


@pytest.mark.parametrize("change,text", [
    (lambda body: body["entries"][0].update(roster={"auto": 1, "rival": 0}), "rival"),
    (lambda body: body["entries"][0].update(roster={"rival": 0}), "rival"),
    (lambda body: body["entries"][0].update(roster={"rival": 1, "x": 1}), "roster"),
    (lambda body: body["entries"][0].update(roster={"rival": 20}), "fit"),
    (lambda body: body["entries"][0].update(rounds=0), "rounds"),
    (lambda body: body["entries"][0].update(weight=0), "weight"),
    (lambda body: body["entries"][0].update(map="Map Nothing"), "catalog"),
    (lambda body: body["entries"][0].update(preset="no_such"), "does not compile"),
    (lambda body: body["entries"][0].update(extra=1), "extra"),
    (lambda body: body.update(order="random"), "order"),
    (lambda body: body.update(entries=[]), "entries"),
    (lambda body: body.update(entries=[3]), "expected"),
    (lambda body: body.pop("entries"), "expected"),
])
def test_lists_put_that_is_not_a_list_is_400_and_changes_nothing(web, change, text):
    before = list_bytes(web)
    body = list_body(web)
    change(body)
    status, answer, _ = web.call("PUT", f"/api/lists/{LIST}", body)
    assert status == 400 and text in answer["error"] and web.context.training_dir not in answer["error"]
    assert list_bytes(web) == before
    assert not os.path.exists(list_path(web, "new_one"))


def test_lists_put_of_a_new_name_that_fails_creates_no_file(web):
    body = list_body(web)
    body["entries"][0]["rounds"] = 0
    assert web.call("PUT", "/api/lists/new_one", body)[0] == 400
    assert not os.path.exists(list_path(web, "new_one"))


@pytest.mark.parametrize("name", BAD_NAMES + ["maps", "Maps"])
def test_list_names_that_are_not_names_are_400(web, name):
    before = {file: open(os.path.join(web.context.training_dir, file), "rb").read() for file in os.listdir(web.context.training_dir) if file.endswith(".json")}
    body = list_body(web)
    for method, suffix, payload in (("GET", "", None), ("PUT", "", body), ("POST", "/delete", {})):
        status, answer, _ = web.call(method, f"/api/lists/{quote(name, safe='/')}{suffix}", payload)
        assert status == 400 and "error" in answer, (method, name)
    after = {file: open(os.path.join(web.context.training_dir, file), "rb").read() for file in os.listdir(web.context.training_dir) if file.endswith(".json")}
    assert after == before


def test_a_stale_list_is_read_and_put_regenerates_it(web):
    body = web.call("GET", "/api/presets/artemis_ctf_rounds")[1]
    body["own"]["general.round_limit"] = "6"
    assert web.call("PUT", "/api/presets/artemis_ctf_rounds", {"base": body["base"], "doc": body["doc"], "own": body["own"]})[0] == 200
    data = web.call("GET", f"/api/lists/{LIST}")[1]
    assert data["stale"] and "artemis_ctf_rounds" in data["stale"] and data["error"] is None and len(data["entries"]) == 3
    assert web.call("GET", "/api/lists")[1]["lists"][0]["stale"] == data["stale"]
    status, saved, _ = web.call("PUT", f"/api/lists/{LIST}", {"order": data["order"], "entries": data["entries"]})
    assert status == 200 and saved["stale"] is None
    assert web.call("GET", f"/api/lists/{LIST}")[1]["stale"] is None
    assert json.loads(list_bytes(web))["presets"]["artemis_ctf_rounds"]["lines"] == compiled(web, "artemis_ctf_rounds").splitlines()


def test_a_list_with_a_map_that_left_the_catalog_is_shown_with_its_error(web):
    maps = [row for row in web.call("GET", "/api/maps")[1]["maps"] if row["config"] != "Map CTF 2v2 Final"]
    assert web.call("PUT", "/api/maps", {"maps": maps})[0] == 200
    data = web.call("GET", f"/api/lists/{LIST}")[1]
    assert data["error"] and "Map CTF 2v2 Final" in data["error"] and len(data["entries"]) == 3
    assert web.call("GET", "/api/lists")[1]["lists"][0]["error"]


def test_list_delete_moves_it_to_deleted(web):
    original = list_bytes(web)
    status, body, _ = web.call("POST", f"/api/lists/{LIST}/delete", {})
    assert status == 200 and body["moved_to"] == f"_deleted/{LIST}.json"
    assert not os.path.exists(list_path(web))
    assert open(os.path.join(web.context.training_dir, "_deleted", LIST + ".json"), "rb").read() == original
    assert web.call("GET", "/api/lists")[1]["lists"] == []
    assert web.call("POST", f"/api/lists/{LIST}/delete", {})[0] == 404
    assert "Maps.json" in os.listdir(web.context.training_dir)
    assert web.call("GET", "/api/maps")[1]["maps"][0]["used_by"] == []


def test_list_changes_need_the_token(web):
    before = list_bytes(web)
    assert web.call("PUT", f"/api/lists/{LIST}", list_body(web), token=False)[0] == 403
    assert web.call("POST", f"/api/lists/{LIST}/delete", {}, token="bad")[0] == 403
    assert web.call("POST", "/api/test/entry", {"list": LIST, "index": 0}, token=False)[0] == 403
    assert web.call("POST", "/api/test/list", {"list": LIST, "episodes": 1}, token=False)[0] == 403
    assert list_bytes(web) == before and not web.queued and not web.launched


# Tests of an entry and of a list

def test_test_entry_queues_the_scenario_of_the_entry(web):
    status, body, _ = web.call("POST", "/api/test/entry", {"list": LIST, "index": 1})
    assert status == 200 and body["state"] in ("queued", "running")
    job = web.wait(body["id"], ("running",))
    assert job["kind"] == "entry" and job["list"] == LIST and job["index"] == 1 and job["state"] == "running"
    (job_id, text), = web.queued
    assert job_id == body["id"] and re.fullmatch(r"\d{8}-\d{6}-[0-9a-f]{4}", job_id)
    entry = web.call("GET", f"/api/lists/{LIST}")[1]["entries"][1]
    lines = text.splitlines()
    assert lines[0].startswith("name ls3_") and "config " + entry["map"] in lines and "preset artemis_ctf_rounds" in lines and "map " + entry["map"] in lines
    assert entry["roster"] == {"auto": 1, "rival": 1} and "roster 0 1 0 0 0 0 0 0 auto 1" in lines
    assert f"round_setup preset=artemis_ctf_rounds; map={entry['map']}; roster=0 1 0 0 0 0 0 0 auto 1" in lines
    assert "check no_errors" in lines and "check setup_ok" in lines and "max_seconds 150.0" in lines
    assert not web.launched


def test_entry_scenario_is_accepted_by_game_compile_for_every_kind_of_entry():
    game = api.game_tool()
    base = {"map": "Map CTF 1v1 First", "rounds": 1, "weight": 1.0}
    cases = [dict(base, preset=None, roster=None), dict(base, preset="", roster={"rival": 1}), dict(base, preset="artemis_ctf_rounds", roster={"auto": 3, "rival": 4})]
    for entry in cases:
        scenario = api.entry_scenario("l", 0, entry)
        for configs in (None, ["Map CTF 1v1 First", "Other"]):
            text, count = game.Compile(scenario, configs)
            assert count == 1 and "config Map CTF 1v1 First" in text and "check setup_ok" in text
        with pytest.raises(ValueError, match="is not recorded"):
            game.Compile(scenario, ["Other"])
    none, base_values, full = (game.Compile(api.entry_scenario("l", 0, entry), None)[0] for entry in cases)
    assert "preset" not in none and "roster" not in none and "map Map CTF 1v1 First" in none
    assert "preset none" in base_values and "round_setup preset=none; map=Map CTF 1v1 First; roster=0 0 0 0 0 0 0 0 auto 0" not in base_values
    assert "round_setup preset=none; map=Map CTF 1v1 First; roster=0 1 0 0 0 0 0 0 auto 0" in base_values
    assert "roster 0 4 0 0 0 0 0 0 auto 3" in full


def test_test_entry_result_comes_from_the_report(web):
    job_id = web.call("POST", "/api/test/entry", {"list": LIST, "index": 0})[1]["id"]
    assert web.wait(job_id, ("running",))["passed"] is None
    web.reports[job_id] = {"id": job_id, "done": False}
    assert web.call("GET", f"/api/test/{job_id}")[1]["state"] == "running"
    web.reports[job_id] = {"id": job_id, "done": True, "result": "ok", "message": "", "matches": [
        {"name": "m", "result": "ok", "end": "limit", "ticks": 10, "checks": [{"name": "setup_ok", "ok": True, "detail": "fine"}], "log": []}]}
    done = web.call("GET", f"/api/test/{job_id}")[1]
    assert done["state"] == "done" and done["passed"] is True and done["matches"][0]["failed"] == []
    second = web.call("POST", "/api/test/entry", {"list": LIST, "index": 0})[1]["id"]
    web.wait(second, ("running",))
    web.reports[second] = {"id": second, "done": True, "result": "ok", "matches": [
        {"name": "m", "result": "ok", "checks": [{"name": "setup_ok", "ok": False, "detail": "preset did not apply"}, {"name": "no_errors", "ok": True, "detail": ""}]}]}
    failed = web.call("GET", f"/api/test/{second}")[1]
    assert failed["state"] == "done" and failed["passed"] is False and failed["matches"][0]["failed"] == ["setup_ok: preset did not apply"]
    third = web.call("POST", "/api/test/entry", {"list": LIST, "index": 0})[1]["id"]
    web.wait(third, ("running",))
    web.reports[third] = {"id": third, "done": True, "result": "fail", "message": "launch-timeout", "matches": []}
    assert web.call("GET", f"/api/test/{third}")[1]["passed"] is False


def test_only_one_test_runs_at_a_time(web):
    first = web.call("POST", "/api/test/entry", {"list": LIST, "index": 0})[1]["id"]
    web.wait(first, ("running",))
    status, body, _ = web.call("POST", "/api/test/entry", {"list": LIST, "index": 1})
    assert status == 409 and first in body["error"]
    assert web.call("POST", "/api/test/list", {"list": LIST, "episodes": 1})[0] == 409
    web.reports[first] = {"id": first, "done": True, "result": "ok", "matches": []}
    assert web.call("POST", "/api/test/entry", {"list": LIST, "index": 1})[0] == 200


def test_test_without_a_live_bench_is_409_and_queues_nothing(web):
    for state in ("watcher_off", "no_mcc"):
        web.status = {**BENCH, "state": state}
        for path, body in (("/api/test/entry", {"list": LIST, "index": 0}), ("/api/test/list", {"list": LIST, "episodes": 1})):
            status, answer, _ = web.call("POST", path, body)
            assert status == 409 and "error" in answer
    assert not web.queued and not web.launched and web.context.jobs == {}


def test_a_failure_to_queue_is_an_error_state(web):
    web.fail_queue = "the game watcher is not running"
    job_id = web.call("POST", "/api/test/entry", {"list": LIST, "index": 0})[1]["id"]
    job = web.wait(job_id, ("error",))
    assert job["passed"] is False and "watcher" in job["message"]
    web.fail_queue = None
    assert web.call("POST", "/api/test/entry", {"list": LIST, "index": 0})[0] == 200


@pytest.mark.parametrize("body", [{"list": LIST, "index": 3}, {"list": LIST, "index": -1}, {"list": LIST, "index": "0"}, {"list": LIST, "index": True},
                                  {"list": LIST}, {"list": "nothing", "index": 0}, {"list": "../x", "index": 0}, {"list": "A", "index": 0}, {"index": 0}, [], "x"])
def test_test_entry_refuses_bad_requests(web, body):
    status, answer, _ = web.call("POST", "/api/test/entry", body)
    assert status in (400, 404) and "error" in answer and not web.queued


def test_test_list_builds_the_command_and_the_scenario(web):
    status, body, _ = web.call("POST", "/api/test/list", {"list": LIST, "episodes": 3})
    assert status == 200
    expected = [sys.executable, os.path.join(Repo(), "Agent", "scripts", "env_check.py"), "--list", list_path(web), "--policy", "seek", "--episodes", "3"]
    assert body["command"] == expected and body["id"].startswith("list-")
    job = web.wait(body["id"], ("running",))
    assert web.launched == [expected] and job["output"] == ["episode 0 agent 0x1: 5 steps"] and job["passed"] is None
    (job_id, text), = web.queued
    lines = text.splitlines()
    assert job_id == body["id"] and "config Map CTF 1v1 First" in lines and "roster 0 1 0 0 0 0 0 0 auto 0" in lines and "events" in lines
    assert "max_seconds 570.0" in lines and "check no_errors" in lines and "preset artemis_ctf_rounds" in lines
    web.process.code = 0
    done = web.call("GET", f"/api/test/{body['id']}")[1]
    assert done["state"] == "done" and done["passed"] is True and "0" in done["message"]


def test_test_list_that_fails_is_reported(web):
    job_id = web.call("POST", "/api/test/list", {"list": LIST, "episodes": 1})[1]["id"]
    web.wait(job_id, ("running",))
    web.process.code = 1
    done = web.call("GET", f"/api/test/{job_id}")[1]
    assert done["state"] == "done" and done["passed"] is False


def test_test_list_waits_for_the_match_and_gives_up(web):
    web.phase = "Loading"
    job_id = web.call("POST", "/api/test/list", {"list": LIST, "episodes": 1})[1]["id"]
    job = web.wait(job_id, ("error",))
    assert "did not start" in job["message"] and not web.launched


@pytest.mark.parametrize("episodes", [0, 51, -1, "3", 2.5, True, None])
def test_test_list_episodes_are_an_integer_from_1_to_50(web, episodes):
    status, answer, _ = web.call("POST", "/api/test/list", {"list": LIST, "episodes": episodes})
    assert status == 400 and "episodes" in answer["error"] and not web.queued and not web.launched


def test_test_list_refuses_a_stale_list(web):
    body = web.call("GET", "/api/presets/artemis_ctf_rounds")[1]
    body["own"]["general.round_limit"] = "6"
    web.call("PUT", "/api/presets/artemis_ctf_rounds", {"base": body["base"], "doc": body["doc"], "own": body["own"]})
    status, answer, _ = web.call("POST", "/api/test/list", {"list": LIST, "episodes": 2})
    assert status == 409 and "save it first" in answer["error"] and not web.launched


def test_test_status_of_an_unknown_id_is_404(web):
    assert web.call("GET", "/api/test/nothing")[0] == 404


def test_the_real_launchers_are_the_fixed_ones():
    """The default Context runs only the two commands of D291 S6: the bench queue (files, no subprocess) and env_check as a list of arguments."""
    import inspect
    context = api.default_context()
    assert context.queue_fn is api.bench_queue and context.launch_fn is api.env_launch
    source = inspect.getsource(api.env_launch)
    assert "shell=False" in source and "shell=True" not in inspect.getsource(api)
    assert "subprocess." not in inspect.getsource(api.bench_queue) and inspect.getsource(api).count("subprocess.Popen") == 1

