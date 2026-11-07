"""
test_tools.py
Tests of the verification tools themselves: Tools/test.py Evaluate (no false green), the --update-baseline guard of check_rules.py
and the -n handling of deps.py.
"""
import hashlib
import json
import importlib.util
import os
import re
import subprocess
import sys

TOOLS = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ROOT = os.path.dirname(TOOLS)
BASELINE = os.path.join(TOOLS, "check_rules_baseline.txt")


def LoadTool(name):
    spec = importlib.util.spec_from_file_location("artemis_" + name, os.path.join(TOOLS, name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


TestTool = LoadTool("test")


def Case(name, failed=False, skipped=False):
    case = {"name": name}
    if failed:
        case["failures"] = [{"failure": "a.cpp:1\nExpected equality"}]
    if skipped:
        case["result"] = "SKIPPED"
    return case


def Data(*cases):
    return {"time": "1ms", "testsuites": [{"name": "S", "testsuite": list(cases)}]}


def Run(script, *arguments):
    return subprocess.run([sys.executable, os.path.join(TOOLS, script)] + list(arguments), cwd=ROOT, capture_output=True, text=True)


def Digest(path):
    with open(path, "rb") as handle:
        return hashlib.sha256(handle.read()).hexdigest()


def Rows(output):
    return [line for line in output.splitlines() if re.match(r"^\s+\d+\s+\S", line)]


def test_evaluate_all_passed_is_0():
    code, lines = TestTool.Evaluate(Data(Case("A"), Case("B")), 0)
    assert code == 0
    assert lines[0].startswith("Tests: 2 passed, 0 failed, 0 skipped")


def test_evaluate_failure_is_1():
    code, lines = TestTool.Evaluate(Data(Case("A"), Case("B", failed=True)), 1)
    assert code == 1
    assert any("FAILED S.B" in line for line in lines)


def test_evaluate_no_testsuites_is_2():
    for data in ({}, {"testsuites": None}, {"testsuites": {}}, []):
        code, lines = TestTool.Evaluate(data, 0)
        assert code == 2
        assert "malformed" in lines[0] and "no testsuites" in lines[0]


def test_evaluate_zero_cases_is_2():
    code, lines = TestTool.Evaluate({"testsuites": []}, 0)
    assert code == 2 and "no test ran" in lines[0] and "filter" not in lines[0]
    code, lines = TestTool.Evaluate(Data(), 0, filter="NoSuchSuite.*")
    assert code == 2 and "the filter matched no test" in lines[0]


def test_evaluate_only_skipped_is_0():
    code, lines = TestTool.Evaluate(Data(Case("A", skipped=True)), 0, {"S.A": "no game"}, "S.*")
    assert code == 0
    assert any("SKIPPED S.A: no game" in line for line in lines)


def test_evaluate_nonzero_exit_without_failures_is_2():
    code, lines = TestTool.Evaluate(Data(Case("A")), 3)
    assert code == 2
    assert any("the exe exited with 3 but the JSON lists no failure" in line for line in lines)


def test_evaluate_nonzero_exit_with_failures_is_1():
    code, _ = TestTool.Evaluate(Data(Case("A", failed=True)), 1)
    assert code == 1
    code, _ = TestTool.Evaluate(Data(Case("A", failed=True)), 0)
    assert code == 1


def test_update_baseline_with_files_is_rejected():
    before = Digest(BASELINE)
    done = Run("check_rules.py", "--update-baseline", "--files", "Tools/deps.py")
    assert done.returncode == 2
    assert "--update-baseline needs the full run" in done.stderr
    assert Digest(BASELINE) == before


def test_update_baseline_with_only_is_rejected():
    before = Digest(BASELINE)
    done = Run("check_rules.py", "--update-baseline", "--only", "D2")
    assert done.returncode == 2
    assert "--update-baseline needs the full run" in done.stderr
    assert Digest(BASELINE) == before


def test_top_n_caps():
    done = Run("deps.py", "top", "-n", "3")
    assert done.returncode == 0
    assert len(Rows(done.stdout)) == 3


def test_top_n_zero_is_uncapped():
    default = Run("deps.py", "top")
    uncapped = Run("deps.py", "top", "-n", "0")
    assert uncapped.returncode == 0
    assert "more" not in uncapped.stdout
    assert len(Rows(uncapped.stdout)) >= len(Rows(default.stdout))
    assert len(Rows(default.stdout)) <= 20

def test_game_compiles_the_tickdump_key_and_rejects_other_values():
    sys.path.insert(0, TOOLS)
    try:
        game = LoadTool("game")
    finally:
        sys.path.remove(TOOLS)

    def Lines(value):
        scenario = {"name": "s", "matches": [{"name": "m", "config": "c", "tickdump": value}]}
        return game.Compile(scenario, None)[0].splitlines()

    assert "tickdump" in Lines(True)
    assert "tickdump full" in Lines("full")
    assert not any(line.startswith("tickdump") for line in Lines(False))
    for bad in ("yes", "tdump", 2):
        try:
            Lines(bad)
        except ValueError as error:
            assert "tickdump" in str(error)
        else:
            raise AssertionError(bad)


def test_game_compiles_every_tick_false_and_rejects_it_with_the_recorders():
    sys.path.insert(0, TOOLS)
    try:
        game = LoadTool("game")
    finally:
        sys.path.remove(TOOLS)

    def Lines(**keys):
        scenario = {"name": "s", "matches": [dict({"name": "m", "config": "c"}, **keys)]}
        return game.Compile(scenario, None)[0].splitlines()

    assert "every_tick off" in Lines(every_tick=False)
    assert not any(line.startswith("every_tick") for line in Lines(every_tick=True))
    assert not any(line.startswith("every_tick") for line in Lines())
    for keys in ({"every_tick": False, "tickdump": True}, {"every_tick": False, "events": True}, {"every_tick": "no"}):
        try:
            Lines(**keys)
        except ValueError as error:
            assert "every_tick" in str(error)
        else:
            raise AssertionError(keys)


def LoadGame():
    sys.path.insert(0, TOOLS)
    try:
        return LoadTool("game")
    finally:
        sys.path.remove(TOOLS)


WINDOWS_PATH = "C:\\Users\\x\\AppData\\Local\\Artemis\\TickDumps\\20261105_101500_ctf_c.tdump"


def test_game_asserts_the_tdump_by_its_short_name_through_the_python_request():
    game = LoadGame()
    script, args = game.AssertArguments(WINDOWS_PATH)
    assert script == "Tools\\tickdump.py"
    assert args == ["assert", "20261105_101500_ctf_c.tdump", "--json"]
    assert game.AssertArguments(WINDOWS_PATH.replace("\\", "/"))[1] == args


def test_game_interprets_the_exit_of_the_assert():
    game = LoadGame()
    failing = json.dumps({"result": "FAIL", "records": 9, "rules": [{"name": "header_flags", "count": 2, "cases": [
        {"tick": 7, "handle": "0x00000000", "detail": "0x1"}]}]})
    assert game.InterpretAssert(0, '{"result": "PASS"}') == ("pass", [])
    status, lines = game.InterpretAssert(1, failing + "\n")
    assert status == "fail" and lines == ["header_flags: 2 case(s)", "  tick 7 handle 0x00000000 0x1"]
    status, lines = game.InterpretAssert(1, "garbage\nmore")
    assert status == "fail" and lines[0].startswith("no rule could be read") and lines[-2:] == ["garbage", "more"]
    assert game.InterpretAssert(2, "ERROR: cannot read x\n") == ("error", ["ERROR: cannot read x"])
    assert game.InterpretAssert(7, "") == ("error", ["exit 7 without output"])
    # An error, or a fail that cannot be read, shows the end of stderr (the traceback), not just the status.
    traceback = "\n".join(["Traceback (most recent call last):", "  File x, line 1", "OverflowError: Python integer -1 out of bounds for uint16"])
    status, lines = game.InterpretAssert(1, "", traceback)
    assert status == "fail" and lines[-1] == "OverflowError: Python integer -1 out of bounds for uint16"
    status, lines = game.InterpretAssert(2, "", "a\n" * 20 + traceback)
    assert status == "error" and lines[-1].startswith("OverflowError") and len(lines) == 8


def test_game_assert_tickdump_uses_the_runner_and_reports_pending_and_errors():
    game = LoadGame()
    calls = []

    def Runner(code, out, err=""):
        def Run(script, args, timeout):
            calls.append((script, args, timeout))
            return code, out, err
        return Run
    assert game.AssertTickDump(WINDOWS_PATH, 30, Runner(0, ""))[0] == "pass"
    assert calls == [("Tools\\tickdump.py", ["assert", "20261105_101500_ctf_c.tdump", "--json"], 30)]
    assert game.AssertTickDump(WINDOWS_PATH, 30, Runner(1, '{"rules": []}'))[0] == "fail"
    assert game.AssertTickDump(WINDOWS_PATH, 30, Runner(2, "", "ERROR: boom")) == ("error", ["ERROR: boom"])
    status, lines = game.AssertTickDump(WINDOWS_PATH, 30, Runner(None, "20261105-101500-ab12"))
    assert status == "pending" and "py --id 20261105-101500-ab12 --wait" in lines[0]
    assert game.AssertTickDump(WINDOWS_PATH, 30, Runner(None, "the watcher refused the run (x)"))[0] == "error"


def test_game_report_fails_the_scenario_when_a_tickdump_assert_fails(capsys):
    game = LoadGame()

    def Report(result="ok"):
        return {"id": "r1", "scenario": "s", "result": result, "seconds": 1, "message": "done", "matches": [
            {"index": 1, "name": "a", "config": "c", "result": "ok", "files": {"tickdump": WINDOWS_PATH}},
            {"index": 2, "name": "b", "config": "c", "result": "ok", "files": {}}]}

    def Asserter(status):
        return lambda path: (status, ["rule: 1 case(s)"] if status == "fail" else [])
    assert game.PrintReport(Report(), Asserter("pass")) == 0
    assert "tickdump assert: PASS" in capsys.readouterr().out
    assert game.PrintReport(Report(), Asserter("fail")) == 1
    out = capsys.readouterr().out
    assert "r1 (s): FAIL" in out and "tickdump-assert" in out and "tickdump assert: FAIL" in out and "rule: 1 case(s)" in out
    assert game.PrintReport(Report(), Asserter("error")) == 1
    assert "tickdump-assert" in capsys.readouterr().out
    assert game.PrintReport(Report(), Asserter("pending")) == 2
    assert game.PrintReport(Report("fail"), Asserter("pass")) == 1
    # A match without tickdump never runs an assert.
    report = Report()
    report["matches"][0]["files"] = {}
    assert game.PrintReport(report, lambda path: 1 / 0) == 0


def test_game_report_prints_the_perf_lines_of_a_match_that_has_them(capsys):
    game = LoadGame()
    perf = {"ticks": 600, "wall_ms_per_tick": 9.5, "engine_ms": 3.25, "read_wait_ms": 1.5, "read_timeouts": 0,
            "sweep_ms": {"avg": 2.0, "p50": 1.75, "p99": 6.5, "max": 12.0},
            "stages": {"Capture": 0.5, "Raycast": 4.0, "Bridge": 0.25, "Tick": 1.0, "A": 0.1, "B": 0.2, "C": 0.3, "Other": 0.05},
            "lockstep": {"waits": 7, "avg_ms": 1.25, "max_ms": 3.5, "timeouts": 0}}
    report = {"id": "r1", "scenario": "s", "result": "ok", "seconds": 1, "message": "", "matches": [
        {"index": 1, "name": "a", "config": "c", "result": "ok", "files": {}, "perf": perf},
        {"index": 2, "name": "b", "config": "c", "result": "ok", "files": {}}]}
    assert game.PrintReport(report, lambda path: 1 / 0) == 0
    lines = capsys.readouterr().out.splitlines()
    assert ("    perf: wall 9.50 ms/tick | engine 3.25 | read wait 1.50 | sweep avg/p50/p99/max 2.00/1.75/6.50/12.00 ms"
            " | lockstep 7 avg 1.25 max 3.50 ms") in lines
    assert "    stages: Raycast 4.00 ms, Tick 1.00 ms, Capture 0.50 ms, A 0.10 ms, B 0.20 ms, C 0.30 ms" not in lines
    assert "    stages: Raycast 4.00 ms, Tick 1.00 ms, Capture 0.50 ms, C 0.30 ms, Bridge 0.25 ms, B 0.20 ms" in lines
    assert sum(1 for line in lines if "perf:" in line or "stages:" in line) == 2


def test_game_report_perf_line_ends_with_the_object_table_maxima_when_the_block_has_them(capsys):
    game = LoadGame()
    perf = {"ticks": 600, "wall_ms_per_tick": 9.5, "engine_ms": 3.25, "read_wait_ms": 1.5, "read_timeouts": 0,
            "sweep_ms": {"avg": 2.0, "p50": 1.75, "p99": 6.5, "max": 12.0}, "stages": {},
            "lockstep": {"waits": 7, "avg_ms": 1.25, "max_ms": 3.5, "timeouts": 0},
            "object_slots_max": 1701, "objects_max": 655}
    report = {"id": "r1", "scenario": "s", "result": "ok", "seconds": 1, "message": "", "matches": [
        {"index": 1, "name": "a", "config": "c", "result": "ok", "files": {}, "perf": perf}]}
    assert game.PrintReport(report, lambda path: 1 / 0) == 0
    lines = capsys.readouterr().out.splitlines()
    assert ("    perf: wall 9.50 ms/tick | engine 3.25 | read wait 1.50 | sweep avg/p50/p99/max 2.00/1.75/6.50/12.00 ms"
            " | lockstep 7 avg 1.25 max 3.50 ms | object slots max 1701 objects max 655") in lines


def test_game_report_without_perf_prints_nothing_new(capsys):
    game = LoadGame()
    report = {"id": "r1", "scenario": "s", "result": "ok", "seconds": 1, "message": "", "matches": [
        {"index": 1, "name": "a", "config": "c", "result": "ok", "files": {}}]}
    assert game.PrintReport(report, lambda path: 1 / 0) == 0
    out = capsys.readouterr().out
    assert "perf:" not in out and "stages:" not in out


def test_game_run_python_reads_the_exit_and_the_output_of_the_run(tmp_path, monkeypatch):
    game = LoadGame()
    (tmp_path / "python").mkdir()
    sent = []
    monkeypatch.setattr(game, "GAME", str(tmp_path))
    monkeypatch.setattr(game, "Send", lambda kind, id=None, **extra: (sent.append((kind, id, extra)) or (True, "queued")))
    monkeypatch.setattr(game.time, "sleep", lambda seconds: None)

    def Answer(name):
        if name.endswith(".json"):
            id = sent[0][1]
            (tmp_path / "python" / (id + ".out")).write_text('{"result": "PASS"}\n', encoding="utf-8")
            (tmp_path / "python" / (id + ".err")).write_text("warning\n", encoding="utf-8")
            return {"done": True, "exit": 0}
        return None
    monkeypatch.setattr(game, "ReadJson", Answer)
    assert game.RunPython("Tools\\tickdump.py", ["assert", "x.tdump", "--json"], 10) == (0, '{"result": "PASS"}\n', "warning\n")
    assert sent[0][0] == "python" and sent[0][2] == {"script": "Tools\\tickdump.py", "args": ["assert", "x.tdump", "--json"]}
    monkeypatch.setattr(game, "ReadJson", lambda name: None)
    code, out, err = game.RunPython("Tools\\tickdump.py", [], 0)
    assert code is None and out == sent[1][1] and err == ""
    monkeypatch.setattr(game, "Send", lambda kind, id=None, **extra: (False, "no"))
    assert game.RunPython("Tools\\tickdump.py", [], 5) == (None, "the watcher refused the run (no)", "")
