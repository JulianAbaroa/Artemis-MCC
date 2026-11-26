"""
test_game_skip.py
Tests of "game.py skip" (REC.2): the file it writes for the bench (skip.txt, "<scenario id> <match number>") from a fake state.json, the
default match (the Running one), the FAIL cases, and the wait for the bench to move on.
"""
import importlib.util
import os

import pytest

from tdump_writer import Repo


def Load():
    spec = importlib.util.spec_from_file_location("artemis_game_skip", os.path.join(Repo(), "Tools", "game.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


game = Load()


class Bench:
    """A fake bench: state.json answers come from `states` (the last one repeats)."""

    def __init__(self, states):
        self.states = list(states)
        self.clock = 0.0

    def __call__(self):
        state = self.states.pop(0) if len(self.states) > 1 else self.states[0]
        return dict(state, _alive=state.get("_alive", True))

    def Sleep(self, seconds):
        self.clock += seconds

    def Now(self):
        return self.clock


def State(index=2, phase="Running", scenario="20261126-1-ab12"):
    return {"scenario": scenario, "matchIndex": index, "phase": phase}


def Call(monkeypatch, tmp_path, states, **kw):
    monkeypatch.setattr(game, "GAME", str(tmp_path))
    bench = Bench(states)
    monkeypatch.setattr(game, "Bench", bench)
    return game.Skip(sleep=bench.Sleep, now=bench.Now, **kw)


def Written(tmp_path):
    path = tmp_path / "skip.txt"
    return path.read_bytes().decode() if path.exists() else None


def test_skip_without_match_uses_the_running_match_and_waits_for_the_next_one(monkeypatch, tmp_path, capsys):
    code = Call(monkeypatch, tmp_path, [State(2), State(2), State(3, "Loading")])
    assert code == 0 and Written(tmp_path) == "20261126-1-ab12 2\n"
    assert not (tmp_path / "skip.txt.tmp").exists()
    assert capsys.readouterr().out.count("\n") == 1


def test_skip_ok_when_the_match_leaves_running(monkeypatch, tmp_path):
    assert Call(monkeypatch, tmp_path, [State(1), State(1, "Leaving")]) == 0
    assert Written(tmp_path) == "20261126-1-ab12 1\n"


def test_skip_fails_without_a_running_match_or_a_bench(monkeypatch, tmp_path, capsys):
    assert Call(monkeypatch, tmp_path, [State(1, "Loading")]) == 1 and Written(tmp_path) is None
    assert Call(monkeypatch, tmp_path, [{"_alive": False}]) == 1 and Written(tmp_path) is None
    assert Call(monkeypatch, tmp_path, [{"scenario": "", "matchIndex": 0, "phase": "Idle"}]) == 1
    assert "FAIL" in capsys.readouterr().out


def test_skip_match_during_loading_waits_for_running_then_for_the_leave(monkeypatch, tmp_path):
    states = [State(1, "Loading"), State(1, "Loading"), State(1, "Running"), State(1, "Running"), State(1, "Leaving")]
    assert Call(monkeypatch, tmp_path, states, match=1) == 0
    assert Written(tmp_path) == "20261126-1-ab12 1\n"


def test_skip_times_out_when_the_bench_does_not_move(monkeypatch, tmp_path, capsys):
    assert Call(monkeypatch, tmp_path, [State(1)], timeout=3.0) == 1
    assert "still Running after 3 s" in capsys.readouterr().out


def test_skip_of_a_match_already_over_writes_nothing(monkeypatch, tmp_path):
    assert Call(monkeypatch, tmp_path, [State(3)], match=2, scenario="20261126-1-ab12") == 0
    assert Written(tmp_path) is None


def test_skip_for_the_scenario_of_the_caller(monkeypatch, tmp_path):
    assert Call(monkeypatch, tmp_path, [State(1), State(2)], match=1, scenario="20261126-1-ab12") == 0
    assert Written(tmp_path) == "20261126-1-ab12 1\n"


def test_print_report_shows_a_skipped_end_without_failing():
    report = {"id": "s", "scenario": "x", "result": "ok", "seconds": 5, "message": "",
              "matches": [{"index": 1, "name": "m", "config": "c", "result": "ok", "end": "skipped", "ticks": 10}]}
    assert game.PrintReport(report) == 0
