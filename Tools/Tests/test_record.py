"""
test_record.py
Tests of Tools/record.py (REC.1): the plan (policies, enabled / --only, config and max_seconds per take, errors), the scenario of the run
(compiled by game.Compile), the EDL (60 fps, golden text), the marks (file and offset across a RecordFileChanged, the pause clock, the
attempts of a manual take) over a fake OBS client, and the hotkey handling over a fake key source. The real OBS and the Win32 hotkeys are
not tested here.
Also (REC.2): the skip after every take (Execute over doubles) and the cap of a take with a scale.
"""
import importlib.util
import json
import os
import types

import pytest

from tdump_writer import Repo

TOOLS = os.path.join(Repo(), "Tools")


def Load(name):
    spec = importlib.util.spec_from_file_location("artemis_" + name, os.path.join(TOOLS, name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


record = Load("record")
game = Load("game")


def Raw(**kw):
    raw = {"name": "E01", "config": "Map M.1", "snapshots": "snaps", "output": "out", "max_seconds": 100,
           "takes": [{"id": "R1", "red": "u02300", "blue": "u00000", "scale": 1, "rounds": 2, "sequence": "Seq 1", "note": "n1"},
                     {"id": "R2", "red": "seek", "blue": "random", "scale": 9, "rounds": 3, "sequence": "Seq 2", "max_seconds": 40,
                      "config": "Other", "enabled": False},
                     {"id": "R3", "red": "u02300", "blue": "u02300", "scale": 1, "rounds": 1, "sequence": "Seq 3", "manual": True}]}
    raw.update(kw)
    return raw


def Takes(raw, **kw):
    raw = dict(raw)
    raw["takes"] = [dict(t, **kw) for t in raw["takes"]]
    return raw


def test_plan_resolves_policies_enabled_only_and_per_take_values(tmp_path):
    plan = record.ParsePlan(Raw())
    assert [t.id for t in record.Select(plan, None)] == ["R1", "R3"]
    assert [t.id for t in record.Select(plan, ["R2", "R1"])] == ["R1", "R2"]
    r1, r2, r3 = plan.takes
    assert (r1.config, r2.config) == ("Map M.1", "Other")
    assert (r1.max_seconds, r2.max_seconds) == (100, 40)
    assert r1.match_seconds == 2 * 100 + 60 and r2.match_seconds == 3 * 40 / 9 + 60  # the scale divides the cap of the rounds
    assert r3.is_manual and r3.match_seconds == record.MANUAL_CAP_SECONDS
    root = str(tmp_path)
    assert record.ResolvePolicy("u02300", plan, root) == os.path.join(root, "snaps", "policy_u02300.pt")
    assert record.ResolvePolicy("seek", plan, root) == "seek"
    assert record.ResolvePolicy("Agent/x/p.pt", plan, root) == os.path.join(root, "Agent", "x", "p.pt")
    with pytest.raises(ValueError, match="names takes"):
        record.Select(plan, ["R9"])
    manual = record.ParsePlan(Raw(takes=[dict(Raw()["takes"][2], max_seconds=600)]))
    assert manual.takes[0].match_seconds == 600


@pytest.mark.parametrize("change, message", [
    (lambda r: r["takes"].append(dict(r["takes"][0])), "id repeated"),
    (lambda r: r["takes"][0].update(sequence=" "), "sequence"),
    (lambda r: r["takes"][0].update(rounds=0), "rounds"),
    (lambda r: r["takes"][0].update(rounds=1.5), "rounds"),
    (lambda r: r["takes"][0].update(manual="yes"), "manual"),
    (lambda r: r["takes"][0].update(color=1), "unknown keys"),
    (lambda r: r["takes"][0].update(scale=-1), "scale"),
    (lambda r: r.update(takes=[]), "takes"),
    (lambda r: r.update(name=""), "name"),
])
def test_plan_errors_are_clear(change, message):
    raw = Raw()
    change(raw)
    with pytest.raises(ValueError, match=message):
        record.ParsePlan(raw)


def test_validate_checks_snapshots_and_sequences(tmp_path):
    plan = record.ParsePlan(Raw(snapshots=str(tmp_path / "snaps")))
    (tmp_path / "snaps").mkdir()
    (tmp_path / "snaps" / "policy_u02300.pt").write_bytes(b"x")
    director = tmp_path / "Director"
    director.mkdir()
    (director / "Map_M.1.txt").write_text("version 2\r\nshot|1|pov|a|on|hold\r\nsequence|Seq 1|1\r\nsequence|Seq 3|1\r\n", encoding="utf-8")
    takes = record.Select(plan, None)
    errors, warnings = record.Validate(plan, takes, str(tmp_path), str(director))
    assert errors == [f"take R1: blue policy u00000: {tmp_path / 'snaps' / 'policy_u00000.pt'} does not exist"] and not warnings
    errors, _ = record.Validate(plan, record.Select(plan, ["R2"]), str(tmp_path), str(director))
    assert any("Seq 2" in e for e in errors)
    _, warnings = record.Validate(plan, takes, str(tmp_path), str(tmp_path / "missing"))
    assert warnings and "not checked" in warnings[0]


def test_scenario_has_one_match_per_active_take_and_compiles():
    plan = record.ParsePlan(Raw())
    takes = record.Select(plan, None)
    scenario = record.BuildScenario(plan, takes)
    assert scenario["name"] == "rec_E01" and len(scenario["matches"]) == 2
    first, manual = scenario["matches"]
    assert first == {"name": "E01 R1", "config": "Map M.1", "bots": [0, 1], "every_tick": False, "director": "Seq 1", "max_seconds": 260}
    assert manual["max_seconds"] == 1800 and manual["director"] == "Seq 3"
    text, count = game.Compile(scenario, ["Map M.1", "Other"])
    assert count == 2 and "director Seq 1" in text and "director Seq 3" in text and "max_seconds 260" in text
    with pytest.raises(ValueError):
        game.Compile({"matches": [dict(first, director="")]}, None)


def test_env_check_command_manual_runs_until_stopped(tmp_path):
    plan = record.ParsePlan(Raw())
    auto, manual = plan.takes[0], plan.takes[2]
    command = record.EnvCheckCommand("py", auto, plan, "s.json", str(tmp_path))
    assert command[command.index("--episodes") + 1] == "2" and "--greedy" not in command and "--sparse" not in command
    assert command[command.index("--scale") + 1] == "1" and command[command.index("--summary-json") + 1] == "s.json"
    assert record.EnvCheckCommand("py", manual, plan, "s.json", str(tmp_path))[command.index("--episodes") + 1] == "0"


def test_timecode_rounds_frames_at_60_fps():
    assert record.Timecode(0) == "00:00:00:00" and record.Timecode(61) == "00:00:01:01"
    assert record.Timecode(60 * 3600 + 60 * 61 + 5) == "01:01:01:05"
    assert record.ParseTimecode("01:00:00:00") == 216000
    edl = record.Edl("T", [("a", 0.0083, 0.0)])  # 0.0083 s = 0.5 frames -> frame 0 (banker's) ; zero length -> 1 frame
    assert "01:00:00:00 01:00:00:01" in edl and "|D:1" in edl


def test_edl_golden_text_for_two_takes():
    text = record.Edl("E01", [("R1 bloques 2, 3", 1.5, 30.25), ("R3 Cámara | x", 40.0, 12.0)])
    assert text == ("TITLE: E01\nFCM: NON-DROP FRAME\n\n"
                    "001  001      V     C        01:00:01:30 01:00:01:31 01:00:01:30 01:00:01:31\n"
                    " |C:ResolveColorBlue |M:R1 bloques 2, 3 |D:1815\n\n"
                    "002  001      V     C        01:00:40:00 01:00:40:01 01:00:40:00 01:00:40:01\n"
                    " |C:ResolveColorBlue |M:R3 Camara   x |D:720\n")


class FakeReq:
    """The ReqClient methods record.Obs uses. duration_ms follows t (set by the test); pause freezes or not by counts_pause."""

    def __init__(self, counts_pause=False):
        self.t, self.active, self.paused, self.counts_pause = 0.0, False, False, counts_pause
        self.frozen, self.calls, self.chapters_fail, self.skipped = 0.0, [], False, 0.0

    def get_record_status(self):
        shown = self.frozen if (self.paused and not self.counts_pause) else self.t - self.skipped
        return types.SimpleNamespace(output_active=self.active, output_paused=self.paused, output_duration=int(shown * 1000))

    def start_record(self):
        self.active = True

    def pause_record(self):
        self.paused, self.frozen = True, self.t

    def resume_record(self):
        self.paused = False
        if not self.counts_pause:
            self.skipped = self.t - self.frozen

    def stop_record(self):
        self.active = False
        return types.SimpleNamespace(output_path="a.mp4")

    def create_record_chapter(self, name):
        self.calls.append(name)
        if self.chapters_fail:
            raise RuntimeError("unsupported")


def test_marks_follow_the_file_across_a_record_file_changed(tmp_path):
    req = FakeReq()
    obs = record.Obs(req)
    obs.Start()
    obs.OnState("OBS_WEBSOCKET_OUTPUT_STARTED", "a.mkv")
    req.t = 3500.0
    first = obs.Position()
    assert (first["video_s"], first["file"], first["file_offset_s"]) == (3500.0, "a.mkv", 3500.0)
    req.t = 3600.0
    obs.OnFileChanged("b.mkv")
    req.t = 3700.0
    second = obs.Position()
    assert (second["video_s"], second["file"], second["file_offset_s"]) == (3700.0, "b.mkv", 100.0)


def test_pause_clock_subtracts_paused_time_only_if_obs_counts_it():
    for counts in (False, True):
        req, now = FakeReq(counts), [0.0]
        obs = record.Obs(req, clock=lambda: now[0])
        obs.Start()
        req.t, now[0] = 10.0, 10.0
        obs.Pause()
        req.t, now[0] = 70.0, 70.0
        obs.Resume()
        req.t, now[0] = 75.0, 75.0
        position = obs.Position()["video_s"]
        assert position == 15.0  # 10 s before the pause + 5 s after, whatever OBS counts
        assert obs.pause_counted is counts


def test_chapters_fall_back_once():
    req = FakeReq()
    req.chapters_fail = True
    obs = record.Obs(req)
    obs.Chapter("a")
    obs.Chapter("b")
    assert req.calls == ["a"] and obs.chapters is False


def MakeMarks(tmp_path, takes_ids=("R1", "R3")):
    plan = record.ParsePlan(Raw())
    takes = record.Select(plan, list(takes_ids))
    return plan, record.Marks(plan, "plan.json", takes, str(tmp_path), "2026-11-26_1200", "t0")


def Pos(video, file="a.mkv", offset=None):
    return {"video_s": video, "file": file, "file_offset_s": video if offset is None else offset}


def test_marks_json_and_edl_with_attempts_of_a_manual_take(tmp_path):
    req = FakeReq()
    obs = record.Obs(req)
    obs.Start()
    obs.OnState("STARTED", "a.mkv")
    plan, marks = MakeMarks(tmp_path)
    marks.Begin("R1", Pos(0.0))
    marks.Finish("R1", Pos(30.0), "ok", None, {"summary": {"rounds": 2}})
    marks.Begin("R3", Pos(30.0))
    lines = []
    req.t = 50.0
    assert record.HandleKey(marks, "R3", "attempt", obs, lines.append) is False
    req.t = 80.0
    assert record.HandleKey(marks, "R3", "attempt", obs, lines.append) is False
    req.t = 100.0
    assert record.HandleKey(marks, "R3", "end", obs, lines.append) is True
    marks.Finish("R3", obs.Position(), "ok")
    marks.Write(obs)
    assert "attempt 2" in lines[0] and "attempt 3" in lines[1] and "end of take" in lines[2]
    data = json.load(open(marks.base + "_marcas.json"))
    r3 = next(t for t in data["takes"] if t["id"] == "R3")
    assert [(a["n"], a["start_s"], a["end_s"]) for a in r3["attempts"]] == [(1, 30.0, 50.0), (2, 50.0, 80.0), (3, 80.0, 100.0)]
    assert next(t for t in data["takes"] if t["id"] == "R2")["status"] == "skipped"
    assert data["takes"][0]["file"] == "a.mkv" and data["files"] == ["a.mkv"]
    edl = open(marks.base + ".edl").read()
    assert edl.count("|M:") == 4 and "|M:R1 n1 |D:1800" in edl
    assert "|M:R3 attempt 1 |D:1200" in edl and "|M:R3 attempt 2 |D:1800" in edl and "|M:R3 attempt 3 |D:1200" in edl


def test_error_take_marker_and_keys_parse():
    assert record.ParseKey("f9") == 0x78 and record.ParseKey("F10") == 0x79 and record.ParseKey("a") == 65
    with pytest.raises(ValueError):
        record.ParseKey("F99")


def test_fake_key_source_and_console_fallback(monkeypatch):
    class Fake:
        def __init__(self, keys):
            self.keys = keys
            self.queue = ["attempt", "end"]

        def Poll(self):
            out, self.queue = self.queue, []
            return out

        def Close(self):
            pass
    source = record.OpenKeys("F9", "F10", Fake)
    assert source.Poll() == ["attempt", "end"] and source.keys == {"attempt": 0x78, "end": 0x79}

    def Refuse(keys):
        raise record.HotkeyError("F9 owned by OBS")
    assert isinstance(record.OpenKeys("F9", "F10", Refuse), record.ConsoleKeys)


def test_obs_settings(tmp_path):
    path = tmp_path / "obs.json"
    path.write_text('{"port": 4460, "password": "pw"}', encoding="utf-8")
    assert record.ObsSettings({}, str(path)) == ("localhost", 4460, "pw")
    assert record.ObsSettings({"ARTEMIS_OBS_PASSWORD": "x"}, str(tmp_path / "no.json")) == ("localhost", 4455, "x")
    with pytest.raises(ValueError, match="no OBS settings"):
        record.ObsSettings({}, str(tmp_path / "no.json"))


def test_wait_running_uses_the_one_based_match_index():
    states = [{"_alive": True, "scenario": "s1", "matchIndex": 1, "phase": "Loading"},
              {"_alive": True, "scenario": "s1", "matchIndex": 1, "phase": "Running"},
              {"_alive": True, "scenario": "s1", "matchIndex": 2, "phase": "Running"}]
    fake = types.SimpleNamespace(Bench=lambda: states.pop(0) if len(states) > 1 else states[0])
    assert record.WaitRunning(fake, "s1", 1, 100, sleep=lambda s: None)[0] == "ok"
    assert record.WaitRunning(fake, "s1", 1, 100, sleep=lambda s: None)[0] == "missed"
    assert record.WaitRunning(fake, "other", 1, 100, sleep=lambda s: None, now=iter(range(0, 1000, 60)).__next__)[0] in ("timeout", "gone")


class FakeObs:
    """The Obs methods Execute uses; every call is logged in `events` (shared with the fake game) to check the order."""

    def __init__(self, events):
        self.events, self.files, self.pause_counted, self.chapters, self.t = events, [], None, True, 0.0

    def Position(self):
        self.t += 1.0
        return {"video_s": self.t, "file": "a.mkv", "file_offset_s": self.t}

    def Status(self):
        return types.SimpleNamespace(output_active=False)

    def Chapter(self, name):
        pass

    def Start(self):
        self.events.append("obs start")

    def Resume(self):
        self.events.append("obs resume")

    def Pause(self):
        self.events.append("obs pause")

    def Stop(self):
        self.events.append("obs stop")
        return "a.mp4"


class FakeProc:
    def __init__(self, code):
        self.returncode = code

    def poll(self):
        return self.returncode

    def wait(self, seconds=None):
        return self.returncode


def RunExecute(tmp_path, monkeypatch, skip_results, codes=(0, 0), takes_ids=("R1", "R2")):
    raw = Raw()
    raw["output"] = str(tmp_path / "out")
    raw["takes"][1]["enabled"] = True
    plan = record.ParsePlan(raw)
    takes = record.Select(plan, list(takes_ids))
    events, calls = [], []
    states = {}

    def Bench():
        return dict(states, _alive=True)

    def Skip(match=None, timeout=30.0, scenario=None):
        calls.append((match, scenario))
        events.append(f"skip {match}")
        return skip_results.pop(0) if skip_results else 0

    fake_game = types.SimpleNamespace(Bench=Bench, Skip=Skip)
    next_state = iter([(m, "Running") for m in range(1, len(takes) + 1)])
    original = record.WaitRunning

    def Wait(game, scenario_id, index, timeout, **kw):
        number, phase = next(next_state)
        states.update(scenario="sc1", matchIndex=number, phase=phase, message="")
        events.append(f"running {number} timeout {timeout:g}")
        return "ok", dict(states)

    monkeypatch.setattr(record, "WaitRunning", Wait)
    monkeypatch.setattr(record.subprocess, "run", lambda *a, **k: types.SimpleNamespace(stdout="queued sc1 (x, 2 matches)", stderr="", returncode=0))
    procs = iter(codes)
    monkeypatch.setattr(record.subprocess, "Popen", lambda *a, **k: FakeProc(next(procs)))
    monkeypatch.setattr(record, "AbsPath", lambda path, root=None: path if os.path.isabs(path) else os.path.join(str(tmp_path), path))
    args = types.SimpleNamespace(start_timeout=300.0, key_attempt="F9", key_end="F10")
    code = record.Execute(args, plan, takes, {"matches": []}, fake_game, FakeObs(events), "plan.json")
    monkeypatch.setattr(record, "WaitRunning", original)
    return code, events, calls


def test_execute_skips_every_take_when_it_ends_and_the_last_one_after_stopping_obs(tmp_path, monkeypatch):
    code, events, calls = RunExecute(tmp_path, monkeypatch, [])
    assert code == 0
    assert calls == [(1, "sc1"), (2, "sc1")]
    # take 1: the video pauses first, then the match is skipped and the next one is waited for with no cap added
    assert events == ["running 1 timeout 300", "obs start", "obs pause", "skip 1", "running 2 timeout 300", "obs resume", "obs stop", "skip 2"]


def test_execute_skips_a_failed_take_and_waits_for_the_cap_when_the_skip_does_not_work(tmp_path, monkeypatch):
    code, events, calls = RunExecute(tmp_path, monkeypatch, [1, 0], codes=(3, 0))
    assert code == 1  # env_check exit code 3 is an error of the take, the plan goes on
    assert [c[0] for c in calls] == [1, 2]
    plan = record.ParsePlan(Raw())
    cap = plan.takes[0].match_seconds
    assert f"running 2 timeout {300 + cap:g}" in events


def test_skip_take_survives_an_os_error_of_the_game_module():
    def Boom(*a, **k):
        raise OSError("disk")
    assert record.SkipTake(types.SimpleNamespace(Skip=Boom), "s", 1) is False
    assert record.SkipTake(types.SimpleNamespace(Skip=lambda *a, **k: 0), "s", 1) is True
    assert record.SkipTake(types.SimpleNamespace(Skip=lambda *a, **k: 1), "s", 1) is False


def test_match_cap_of_a_take_with_scale_divides_by_the_scale():
    raw = Raw()
    raw["takes"][1].update(scale=9, rounds=3, max_seconds=90, enabled=True)
    take = record.ParsePlan(raw).takes[1]
    assert take.match_seconds == 3 * 90 / 9 + 60
    raw["takes"][1].update(scale=0.5)
    assert record.ParsePlan(raw).takes[1].match_seconds == 3 * 90 + 60  # a scale under 1 never lengthens it
