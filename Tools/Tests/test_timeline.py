"""
test_timeline.py
P.3 C (D373): the timeline of each decision. A read tick carries the QPC marks and the Need block of the header, and artemis.timeline joins the
marks of a decision with the Need of a later tick into rows and a table, without MCC.
"""
import importlib

import numpy as np
import pytest

from test_bridge_setup import modules  # noqa: F401  (fixture: artemis.bridge and artemis.bridge_layout with a stand-in for kernel32)
from test_ctf import ctf, Frame, FakeBridge, Nothing, P0  # noqa: F401  (ctf: fixture, artemis.env.ctf over a fake bridge)


@pytest.fixture
def timeline(modules):  # noqa: F811
    return importlib.import_module("artemis.timeline")


def test_a_read_tick_carries_the_marks_and_the_need(modules, timeline):  # noqa: F811
    bridge_module, layout = modules
    names = layout.BridgeTickHeader.names
    assert names[-2:] == ("PossessedPlayer", "PossessedInput")
    assert names.index("PublishQpc") < names.index("CaptureQpc") < names.index("NeedFlags")

    data = np.zeros((), dtype=layout.BridgeTick)
    empty = bridge_module.PublishedTick(data, 0)
    assert (empty.capture_qpc, empty.ai_start_qpc, empty.need_decision, empty.need_flags) == (0, 0, 0, 0)

    header = data["Header"]
    header["CaptureQpc"], header["AiStartQpc"], header["AiEndQpc"], header["FlattenStartQpc"] = 10, 20, 30, 40
    header["NeedDecision"], header["NeedQpc"], header["NeedEndQpc"], header["NeedFlags"] = 8, 50, 60, 1
    tick = bridge_module.PublishedTick(data, 99)
    assert (tick.capture_qpc, tick.ai_start_qpc, tick.ai_end_qpc, tick.flatten_start_qpc) == (10, 20, 30, 40)
    assert (tick.need_decision, tick.need_qpc, tick.need_end_qpc, tick.need_flags) == (8, 50, 60, 1)
    assert timeline.tick_marks(tick)["capture"] == 10 and timeline.tick_marks(tick)["received"] == 99
    assert timeline.need_marks(tick) == (8, 50, 60, 1)
    assert timeline.need_marks(empty) is None


def Marks(base, answered=None):
    """A decision whose marks go 1 ms (1 count = 1 ms at the frequency 1000 of the tests) apart from base."""
    marks = {"capture": base, "ai_start": base + 1, "ai_end": base + 3, "flatten_start": base + 4, "publish": base + 5,
             "received": base + 6}
    marks["answered"] = base + 8 if answered is None else answered
    return marks


def test_join_gives_a_row_per_decision_with_and_without_need(timeline):
    decisions = {(1, 4): Marks(100), (1, 8): Marks(200), (1, 12): Marks(300)}
    needs = {(1, 4): (112, 112, 0),      # no wait: need and end are the same instant
             (1, 8): (205, 230, 1)}      # waited: need before the answer, end after
    rows = timeline.join_decisions(decisions, needs)
    assert [row["key"] for row in rows] == [(1, 4), (1, 8), (1, 12)]
    assert [row["has_need"] for row in rows] == [True, True, False]
    assert (rows[2]["need_qpc"], rows[2]["need_end_qpc"], rows[2]["need_flags"]) == (0, 0, 0)

    ms = timeline.segment_ms(rows, 1000.0)  # 1000 counts per second: one count is one ms
    assert ms["ai_queue"] == [1.0, 1.0, 1.0] and ms["ai"] == [2.0, 2.0, 2.0] and ms["bridge_queue"] == [1.0] * 3
    assert ms["flatten"] == [1.0] * 3 and ms["wake"] == [1.0] * 3 and ms["python"] == [2.0] * 3
    # Only the decisions with a Need have the stretches that end at the need.
    assert ms["game_to_need"] == [12.0, 5.0]
    assert ms["slack"] == [112 - 108, 205 - 208]  # positive: the answer was in time, negative: the game waited
    assert ms["wait"] == [0.0, 25.0]
    assert timeline.count_waits(rows) == (3, 1, 1)


def test_a_wait_that_was_cut_or_timed_out_counts_as_waited_and_a_mark_that_is_zero_is_left_out(timeline):
    rows = timeline.join_decisions({(1, 4): {**Marks(100), "ai_start": 0, "ai_end": 0}},
                                   {(1, 4): (110, 140, timeline.NEED_WAITED | timeline.NEED_CUT)})
    assert timeline.count_waits(rows) == (1, 1, 0)
    ms = timeline.segment_ms(rows, 1000.0)
    assert ms["ai_queue"] == [] and ms["ai"] == [] and ms["bridge_queue"] == []
    assert ms["wait"] == [30.0]


def test_the_table_has_a_line_per_stretch_and_the_counts(timeline):
    rows = timeline.join_decisions({(1, 4): Marks(100), (1, 8): Marks(200)}, {(1, 4): (112, 112, 0)})
    lines = timeline.timeline_lines(rows, 1000.0)
    assert lines[0].startswith("timeline over 2 decisions")
    assert [line.split()[0] for line in lines[1:-1]] == [name for name, *_ in timeline.SEGMENTS]
    assert "waited in 0 of 2" in lines[-1] and "1 have no Need" in lines[-1]
    assert timeline.timeline_lines([], 1000.0) == ["timeline: no decision measured"]
    assert timeline.timeline_lines(rows, 0) == ["timeline: no decision measured"]


def test_ctf_env_with_timing_keeps_the_marks_and_the_need_of_a_later_tick(ctf, monkeypatch):  # noqa: F811
    timeline = importlib.import_module("artemis.timeline")
    frames = [Frame(ctf, action_tick=4, publish_qpc=1000), Frame(ctf, action_tick=8, publish_qpc=1200), Frame(ctf, action_tick=12)]
    frames[0].capture_qpc, frames[0].ai_start_qpc = 990, 995
    frames[1].need_decision, frames[1].need_qpc, frames[1].need_end_qpc, frames[1].need_flags = 4, 1300, 1400, 1
    bridge = FakeBridge(frames)
    bridge.qpc_frequency, bridge.last_copy_ns = 1000, 0
    monkeypatch.setattr(ctf, "qpc", lambda: 1500)
    env = ctf.CtfEnv(bridge, ctf.CtfConfig(timing=True))
    env.reset()
    env.step({P0: Nothing(ctf)})
    env.step({P0: Nothing(ctf)})
    match = frames[0].match_id
    assert env.decisions[(match, 4)]["capture"] == 990 and env.decisions[(match, 4)]["answered"] == 1500
    assert env.needs == {(match, 4): (1300, 1400, 1)}
    rows = timeline.join_decisions(env.decisions, env.needs)
    assert [row["has_need"] for row in rows] == [True, False]
