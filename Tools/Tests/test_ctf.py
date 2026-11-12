"""
test_ctf.py
The environment of stage 0 (Agent/artemis/env/ctf.py) over synthetic frames, with no game: the bridge is a fake that hands out the
ticks of a list. The real Bridge needs Windows shared memory, so a stand-in module takes its place for the import. B4h H.5: the side
of an agent comes from the OwnerTeam of the stands, and the flag state from the rows of the tick. P.6 (D255): the clock and the score
of the round come from the Match.
"""
import importlib
import math
import os
import sys
import types
from dataclasses import dataclass
from typing import NamedTuple

import numpy as np
import pytest

from tdump_writer import Repo

AGENT = os.path.join(Repo(), "Agent")
P0, B0 = 0xEC700000, 0xE5000001
FLAG_RED, FLAG_BLUE, STAND_RED, STAND_BLUE = 0xE5A00001, 0xE5A00002, 0xE5A00003, 0xE5A00004
RED, BLUE = 0, 1
NONE = 0xFFFFFFFF
HOME, CARRIED, DROPPED = 0, 1, 2


@dataclass
class FakeAction:
    handle: int
    buttons: int = 0
    yaw: float = 0.0
    pitch: float = 0.0
    forward: float = 0.0
    left: float = 0.0


class FakeTick:
    def __init__(self, layout, players, entities, objectives, stands, events=(), game_tick=100, match_id=1, action_tick=1, match=None):
        def Rows(name, rows):
            out = np.zeros(len(rows), getattr(layout, name))
            for i, row in enumerate(rows):
                for field, value in row.items():
                    out[field][i] = value
            return out
        self.players = Rows("BridgePlayer", players)
        self.entities = Rows("BridgeEntity", sorted(entities, key=lambda e: e["Handle"]))
        self.objectives = Rows("BridgeObjective", objectives)
        self.stands = Rows("BridgeStand", stands)
        self.events = Rows("BridgeEvent", list(events))
        self.match = Rows("BridgeMatch", [{"GameTick": game_tick, "RoundTicksRemaining": -1, **(match or {})}])[0]
        self.match_id, self.action_tick = match_id, action_tick


class SetupAnswer(NamedTuple):
    """What the bridge answers to a setup request: the result, the round and the reason."""
    result: object
    round: int
    reason: str


# BridgeSetupResult by number, filled by the ctf fixture once the layout is imported.
RESULTS = {}


def Status(result, round=-1, reason=""):
    """An answer with the result 1 Pending, 2 Applied, 3 AtLoad or 4 Rejected."""
    return SetupAnswer(RESULTS[result], round, reason)


class FakeBridge:
    def __init__(self, ticks):
        self.ticks, self.is_alive, self.written = list(ticks), True, []
        # Setup requests: what Artemis answers at once (Pending) and at the round break (Applied), the requests seen and the calls in order.
        self.setup_requests, self.round_ends, self.calls = [], 0, []
        self.setup_answer, self.setup_final = Status(1), Status(2)

    def set_control(self, **kwargs):
        self.controls = getattr(self, "controls", []) + [kwargs]

    def wait_control(self, *args, **kwargs):
        return True

    def wait_tick(self, timeout):
        return self.ticks.pop(0) if self.ticks else None

    def new_events(self, published):
        return published.events

    def write_actions(self, tick, actions):
        self.written.append((tick, actions))

    def end_round(self, wait=True):
        self.round_ends += 1
        self.calls.append("end_round")

    def request_setup(self, preset=None, map=None, roster=None, wait=True, timeout=2.0):
        self.setup_requests.append({"preset": preset, "map": map, "roster": roster})
        self.calls.append("request_setup")
        return self.setup_answer

    def wait_setup(self, timeout=10.0):
        return self.setup_final


@pytest.fixture
def ctf(monkeypatch):
    """artemis.env.ctf imported with a stand-in for artemis.bridge (the real one opens Windows shared memory)."""
    monkeypatch.syspath_prepend(AGENT)
    for name in [n for n in sys.modules if n == "artemis" or n.startswith("artemis.")]:
        monkeypatch.delitem(sys.modules, name)
    layout = importlib.import_module("artemis.bridge_layout")
    fake = types.ModuleType("artemis.bridge")
    fake.Action, fake.Bridge = FakeAction, FakeBridge
    fake.BridgeMode, fake.BridgeOwner = layout.BridgeMode, layout.BridgeOwner
    monkeypatch.setitem(sys.modules, "artemis.bridge", fake)
    module = importlib.import_module("artemis.env.ctf")
    module.layout = layout
    RESULTS.update({value: layout.BridgeSetupResult(value) for value in (1, 2, 3, 4)})
    yield module
    for name in [n for n in sys.modules if n == "artemis" or n.startswith("artemis.")]:
        sys.modules.pop(name, None)


def Frame(ctf, owners=(RED, BLUE), flags=(HOME, HOME), flag_at=None, carrying=False, events=(), game_tick=100, agent_at=(5.0, 0.0, 0.0),
          action_tick=1, match_id=1, match=None, vitality=1.0, stand_count=2):
    """One tick of a round on a map with a red base at x = 0 and a blue base at x = 100. owners: OwnerTeam of the stands of BaseTeam 0
    and 1 in this round. flags: state of the flag of BaseTeam 0 and 1. flag_at: position of the flag of BaseTeam 1 when it is not
    home. match: fields of the Match (by default a round without time limit). The agent is the player P0 of team RED, aiming at +x."""
    L = ctf.layout
    stand_at = {RED: (0.0, 0.0, 0.0), BLUE: (100.0, 0.0, 0.0)}
    flag_pos = [stand_at[RED], flag_at or stand_at[BLUE]]
    entities = [{"Handle": B0, "WorldOrigin": agent_at, "Team": RED, "Vitality": vitality},
                {"Handle": STAND_RED, "WorldOrigin": stand_at[RED]}, {"Handle": STAND_BLUE, "WorldOrigin": stand_at[BLUE]},
                {"Handle": FLAG_RED, "WorldOrigin": flag_pos[0]}, {"Handle": FLAG_BLUE, "WorldOrigin": flag_pos[1]}]
    players = [{"Handle": P0, "BipedHandle": B0, "Team": RED, "LocalIndex": 0xFF, "IsAlive": 1, "HasBody": 1, "Aim": (1.0, 0.0, 0.0),
                "Shield": 1.0, "Objective": FLAG_BLUE if carrying else NONE}]
    objectives = [{"Handle": FLAG_RED, "BaseTeam": RED, "OwnerTeam": owners[0], "State": flags[0], "CarrierPlayer": NONE, "CarrierBiped": NONE},
                  {"Handle": FLAG_BLUE, "BaseTeam": BLUE, "OwnerTeam": owners[1], "State": flags[1],
                   "CarrierPlayer": P0 if flags[1] == CARRIED else NONE, "CarrierBiped": B0 if flags[1] == CARRIED else NONE}]
    stands = [{"Handle": STAND_RED, "BaseTeam": RED, "OwnerTeam": owners[0]}, {"Handle": STAND_BLUE, "BaseTeam": BLUE, "OwnerTeam": owners[1]}][:stand_count]
    return FakeTick(L, players, entities, objectives, stands, events, game_tick, match_id=match_id, action_tick=action_tick, match=match)


def Reset(ctf, *frames):
    env = ctf.CtfEnv(FakeBridge(frames))
    observations, _ = env.reset()
    return env, observations[P0]


def test_enemy_flag_at_its_stand_is_home(ctf):
    env, obs = Reset(ctf, Frame(ctf))
    assert env.states[P0].side == RED
    assert obs.shape == (ctf.OBS_SIZE,)
    assert obs[11] == 1.0 and obs[12] == 0.0 and obs[6] == 0.0
    assert obs[18] == 1.0
    assert obs[10] == pytest.approx(95.0 / ctf.METERS)
    # Facing +x: the enemy flag is straight ahead and the own stand behind.
    assert obs[7] > 0 and obs[14] < 0


def test_obs_4_follows_the_vitality_of_the_biped_and_nan_is_zero(ctf):
    _, obs = Reset(ctf, Frame(ctf, vitality=0.25))
    assert obs[4] == pytest.approx(0.25)
    _, obs = Reset(ctf, Frame(ctf, vitality=float("nan")))
    assert obs[4] == 0.0


def test_carried_flag_makes_the_agent_a_carrier_that_heads_for_its_stand(ctf):
    frame = Frame(ctf, flags=(HOME, CARRIED), flag_at=(5.0, 0.0, 0.5), carrying=True)
    env = ctf.CtfEnv(FakeBridge([frame]))
    env.reset()
    obs, phi = env._observe(P0)
    assert obs[6] == 1.0 and obs[13] == 1.0
    assert obs[11] == 0.0 and obs[12] == 0.0
    assert phi == pytest.approx(-5.0 / ctf.METERS)


def test_dropped_flag_is_flagged_as_dropped_and_shaped_through_it(ctf):
    frame = Frame(ctf, flags=(HOME, DROPPED), flag_at=(50.0, 0.0, 0.0))
    env = ctf.CtfEnv(FakeBridge([frame]))
    env.reset()
    obs, phi = env._observe(P0)
    assert obs[11] == 0.0 and obs[12] == 1.0 and obs[6] == 0.0
    assert phi == pytest.approx(-(45.0 + 50.0) / ctf.METERS)


def test_own_flag_that_is_not_home_is_seen(ctf):
    env, obs = Reset(ctf, Frame(ctf, flags=(DROPPED, HOME)))
    assert obs[18] == 0.0


def test_a_flag_lying_near_its_stand_but_dropped_is_not_at_the_stand(ctf):
    # The old test was a distance to the stand; now the state of the flag is the truth (a dropped flag can lie a metre from it).
    env, obs = Reset(ctf, Frame(ctf, flags=(HOME, DROPPED), flag_at=(99.5, 0.0, 0.0)))
    assert obs[11] == 0.0 and obs[12] == 1.0


def test_sides_swap_between_rounds_with_the_owner_team_of_the_stands(ctf):
    env = ctf.CtfEnv(FakeBridge([Frame(ctf, owners=(RED, BLUE), action_tick=1), Frame(ctf, owners=(BLUE, RED), action_tick=2, match_id=2)]))
    _, _ = env.reset()
    assert env.states[P0].side == RED
    # Round 2: the red team plays the stand of BaseTeam 1, so its enemy flag is the one of BaseTeam 0, at x = 0 behind the agent.
    observations, _ = env.reset()
    obs = observations[P0]
    assert env.states[P0].side == BLUE
    assert obs[7] < 0 and obs[10] == pytest.approx(5.0 / ctf.METERS)
    assert obs[14] > 0 and obs[17] == pytest.approx(95.0 / ctf.METERS)


def test_team_that_owns_no_stand_has_no_side_and_sees_no_flags(ctf):
    env, obs = Reset(ctf, Frame(ctf, owners=(BLUE, 2)))
    assert env.states[P0].side is None
    assert not obs[7:19].any()


def test_rewards_come_from_the_flag_events_of_artemis(ctf):
    def Event(kind, cause=P0, effect=NONE, sequence=1):
        return {"Sequence": sequence, "Kind": kind, "CausePlayer": cause, "EffectPlayer": effect}
    start = Frame(ctf, action_tick=1)
    taken = Frame(ctf, flags=(HOME, CARRIED), flag_at=(5.0, 0.0, 0.5), carrying=True, action_tick=2,
                  events=[Event(ctf.Event.FlagTaken)])
    # Another player's take and a second take of the agent give nothing more.
    again = Frame(ctf, flags=(HOME, CARRIED), flag_at=(5.0, 0.0, 0.5), carrying=True, action_tick=3,
                  events=[Event(ctf.Event.FlagTaken, cause=0xEC700001, sequence=2), Event(ctf.Event.FlagTaken, sequence=3)])
    captured = Frame(ctf, action_tick=4, events=[Event(ctf.Event.FlagCaptured, sequence=4)])
    env = ctf.CtfEnv(FakeBridge([start, taken, again, captured]))
    env.reset()
    nothing = np.zeros(len(ctf.ACTION_NVEC), dtype=np.int64)
    config = env.config
    _, rewards, _, _, infos = env.step({P0: nothing})
    assert infos[P0]["events"] == ["FlagTaken"]
    assert rewards[P0] >= config.take_reward - 0.1
    _, rewards, _, _, infos = env.step({P0: nothing})
    assert infos[P0]["events"] == []
    _, rewards, _, _, infos = env.step({P0: nothing})
    assert infos[P0]["events"] == ["FlagCaptured"]
    assert rewards[P0] >= config.capture_reward - 0.5  # the shaping of the phi jump rides on top


def test_the_environment_no_longer_deduces_the_side_or_searches_the_entities(ctf):
    source = open(ctf.__file__, encoding="utf-8").read()
    assert "def _side(" not in source and "np.linalg.norm(Vec(e[\"WorldOrigin\"])" not in source
    assert "FLAG_SUBTYPE" not in source and "STAND_SUBTYPE" not in source and "AT_STAND" not in source


# --- Clock and score of the round (P.6, D255) ------------------------------------------------------------------------------------

def Clock(clock, remaining=-1, limit=0):
    return {"RoundClockTicks": clock, "RoundTicksRemaining": remaining, "RoundTimeLimitTicks": limit}


def Observe(ctf, match, max_seconds=120.0):
    env = ctf.CtfEnv(FakeBridge([Frame(ctf, match=match)]), ctf.CtfConfig(max_seconds=max_seconds))
    observations, _ = env.reset()
    return observations[P0]


def test_time_left_is_the_one_of_the_round_when_it_has_a_limit(ctf):
    # 2 min limit, 1 min played: half left (the cap of the environment is the same 120 s).
    assert Observe(ctf, Clock(3600, remaining=3600, limit=7200))[19] == pytest.approx(0.5)


def test_time_left_without_a_limit_is_the_cap_of_the_environment_over_the_round_clock(ctf):
    assert Observe(ctf, Clock(1800))[19] == pytest.approx(0.75)


def test_a_cap_shorter_than_the_limit_of_the_round_is_the_horizon(ctf):
    # Cap 60 s, limit 120 s, 20 s played: 40 of 60 s left.
    assert Observe(ctf, Clock(1200, remaining=6000, limit=7200), max_seconds=60.0)[19] == pytest.approx(2.0 / 3.0)


def test_a_round_that_ran_out_has_no_time_left(ctf):
    assert Observe(ctf, Clock(7200, remaining=0, limit=7200))[19] == 0.0


def test_score_left_of_both_teams_comes_from_the_match(ctf):
    scores = [0] * 8
    scores[RED], scores[BLUE] = 1, 2
    obs = Observe(ctf, {"ScoreToWin": 3, "TeamRoundScores": scores})
    assert obs[20] == pytest.approx(2.0 / 3.0) and obs[21] == pytest.approx(1.0 / 3.0)


def test_no_score_to_win_gives_no_score_left(ctf):
    obs = Observe(ctf, {"ScoreToWin": 0})
    assert obs[20] == 0.0 and obs[21] == 0.0


def test_the_cap_of_the_environment_truncates_and_the_end_of_the_round_terminates(ctf):
    nothing = np.zeros(len(ctf.ACTION_NVEC), dtype=np.int64)
    # Cap of 60 s reached by the round clock: cut with end_round, truncated.
    frames = [Frame(ctf, match=Clock(0)), Frame(ctf, action_tick=2, match=Clock(3600))]
    env = ctf.CtfEnv(FakeBridge(frames), ctf.CtfConfig(max_seconds=60.0))
    env.reset()
    _, _, terminated, truncated, _ = env.step({P0: nothing})
    assert truncated[P0] and not terminated[P0]
    # The time limit of the round runs out first: the engine ends the round, terminated.
    end = {"Sequence": 1, "Kind": ctf.Event.RoundEnd, "CausePlayer": NONE, "EffectPlayer": NONE}
    env = ctf.CtfEnv(FakeBridge([Frame(ctf, match=Clock(0, remaining=1800, limit=1800)),
                                 Frame(ctf, action_tick=2, events=[end], match=Clock(1800, remaining=0, limit=1800))]))
    env.reset()
    _, _, terminated, truncated, _ = env.step({P0: nothing})
    assert terminated[P0] and not truncated[P0]


def test_the_environment_measures_no_time_of_its_own(ctf):
    source = open(ctf.__file__, encoding="utf-8").read()
    assert "GameTick" not in source and "start_tick" not in source


def test_reset_with_a_preset_requests_it_and_ends_the_round(ctf):
    bridge = FakeBridge([Frame(ctf)])
    env = ctf.CtfEnv(bridge)
    env.reset(preset="artemis_ctf_fast")
    assert bridge.setup_requests == [{"preset": "artemis_ctf_fast", "map": None, "roster": None}]
    assert bridge.round_ends == 1


def test_reset_reports_the_preset_and_its_result_in_the_info(ctf):
    bridge = FakeBridge([Frame(ctf)])
    _, infos = ctf.CtfEnv(bridge).reset(preset="artemis_ctf_fast")
    assert infos[P0]["preset"] == {"name": "artemis_ctf_fast", "result": "Applied"}
    _, infos = ctf.CtfEnv(FakeBridge([Frame(ctf)])).reset()
    assert "preset" not in infos[P0] and "setup" not in infos[P0]


def test_a_preset_still_pending_after_the_wait_is_logged_and_reported(ctf, caplog):
    bridge = FakeBridge([Frame(ctf)])
    bridge.setup_final = Status(1)
    with caplog.at_level("WARNING"):
        _, infos = ctf.CtfEnv(bridge).reset(preset="artemis_ctf_fast")
    assert infos[P0]["preset"]["result"] == "Pending"
    assert "still pending" in caplog.text


def test_reset_without_a_setup_asks_for_nothing(ctf):
    bridge = FakeBridge([Frame(ctf)])
    ctf.CtfEnv(bridge).reset()
    assert bridge.setup_requests == [] and bridge.round_ends == 0


def test_an_empty_preset_asks_for_the_base_values(ctf):
    bridge = FakeBridge([Frame(ctf)])
    ctf.CtfEnv(bridge).reset(preset="")
    assert bridge.setup_requests[0]["preset"] == ""


def test_a_preset_taken_in_the_lobby_does_not_end_a_round(ctf):
    bridge = FakeBridge([Frame(ctf)])
    bridge.setup_answer = Status(3)
    ctf.CtfEnv(bridge).reset(preset="artemis_ctf_fast")
    assert bridge.round_ends == 0


def test_a_rejected_setup_raises_with_its_reason_at_once_or_at_the_break(ctf):
    bridge = FakeBridge([Frame(ctf)])
    bridge.setup_answer = Status(4, reason="preset: changes load fields")
    with pytest.raises(RuntimeError, match="changes load fields"):
        ctf.CtfEnv(bridge).reset(preset="artemis_ctf_colors")
    assert bridge.round_ends == 0

    bridge = FakeBridge([Frame(ctf)])
    bridge.setup_final = Status(4, reason="roster: 70 bots, at most 64")
    with pytest.raises(RuntimeError, match="at most 64"):
        ctf.CtfEnv(bridge).reset(preset="artemis_ctf_colors")
    assert bridge.round_ends == 1


def test_a_setup_nobody_answered_raises(ctf):
    bridge = FakeBridge([Frame(ctf)])
    bridge.setup_answer = None
    with pytest.raises(RuntimeError, match="did not take"):
        ctf.CtfEnv(bridge).reset(preset="artemis_ctf_fast")


def test_sparse_is_asked_for_only_when_the_config_says_so(ctf):
    off, on = FakeBridge([Frame(ctf)]), FakeBridge([Frame(ctf)])
    assert ctf.CtfConfig().sparse is False
    ctf.CtfEnv(off).take_control()
    ctf.CtfEnv(on, ctf.CtfConfig(sparse=True)).take_control()
    assert off.controls[-1]["sparse"] is False
    assert on.controls[-1]["sparse"] is True


def test_release_goes_back_to_every_tick(ctf):
    bridge = FakeBridge([Frame(ctf)])
    ctf.CtfEnv(bridge, ctf.CtfConfig(sparse=True)).release()
    assert bridge.controls[-1]["sparse"] is False


def test_the_layout_has_the_sparse_fields(ctf):
    layout = ctf.layout
    assert "RequestSparse" in layout.BridgeClientControl.names
    assert "IsSparse" in layout.BridgeArtemisControl.names
    assert layout.OFFSETS["Control.Artemis.IsSparse"] == 46
    assert layout.OFFSETS["Control.Client.RequestSparse"] == layout.OFFSETS["Control.Client"] + 13


# --- map variants (Q98, D266) -----------------------------------------------------------------------------------------------------


def test_reset_with_a_map_requests_it_and_ends_the_round(ctf):
    bridge = FakeBridge([Frame(ctf)])
    ctf.CtfEnv(bridge).reset(map="Reach Bus Mountain Pass Reach Bus CTF")
    assert bridge.setup_requests == [{"preset": None, "map": "Reach Bus Mountain Pass Reach Bus CTF", "roster": None}]
    assert bridge.round_ends == 1


def test_reset_with_a_preset_a_map_and_a_roster_is_one_request_and_one_end_of_round(ctf):
    bridge = FakeBridge([Frame(ctf)])
    ctf.CtfEnv(bridge).reset(preset="artemis_ctf_fast", map="reach_bus", roster={"0": 3, "1": 3})
    assert bridge.calls == ["request_setup", "end_round"]
    assert bridge.setup_requests == [{"preset": "artemis_ctf_fast", "map": "reach_bus", "roster": {"0": 3, "1": 3}}]


def test_the_setup_is_a_dict_or_a_match_setup_and_the_keywords_replace_its_fields(ctf):
    bridge = FakeBridge([Frame(ctf)])
    ctf.CtfEnv(bridge).reset(setup={"preset": "a", "map": "m"}, preset="b")
    assert bridge.setup_requests == [{"preset": "b", "map": "m", "roster": None}]

    bridge = FakeBridge([Frame(ctf)])
    ctf.CtfEnv(bridge).reset(setup=ctf.MatchSetup(roster=[2, 2]))
    assert bridge.setup_requests == [{"preset": None, "map": None, "roster": [2, 2]}]

    with pytest.raises(ValueError):
        ctf.CtfEnv(FakeBridge([Frame(ctf)])).reset(setup={"presets": "a"})


def test_reset_reports_the_map_the_roster_and_the_setup_in_the_info(ctf):
    bridge = FakeBridge([Frame(ctf)])
    bridge.setup_final = Status(2, 4, "partial: 2 of 4 bots")
    _, infos = ctf.CtfEnv(bridge).reset(map="reach_bus", roster={"0": 4})
    assert infos[P0]["map"] == {"name": "reach_bus", "result": "Applied"}
    assert infos[P0]["roster"]["result"] == "Applied"
    assert infos[P0]["setup"] == {"result": "Applied", "round": 4, "reason": "partial: 2 of 4 bots"}
    assert "preset" not in infos[P0]
    _, infos = ctf.CtfEnv(FakeBridge([Frame(ctf)])).reset()
    assert "map" not in infos[P0]


def test_an_empty_map_asks_for_the_loaded_one(ctf):
    bridge = FakeBridge([Frame(ctf)])
    ctf.CtfEnv(bridge).reset(map="")
    assert bridge.setup_requests[0]["map"] == ""


def test_a_map_left_for_the_next_load_while_a_match_is_up_raises(ctf):
    bridge = FakeBridge([Frame(ctf)])
    bridge.setup_answer = Status(3)
    with pytest.raises(RuntimeError, match="next match load"):
        ctf.CtfEnv(bridge).reset(map="reach_bus")


def test_a_rejected_map_raises_at_once_or_at_the_break(ctf):
    bridge = FakeBridge([Frame(ctf)])
    bridge.setup_answer = Status(4, reason="map: does not fit the loaded map")
    with pytest.raises(RuntimeError, match="does not fit the loaded map"):
        ctf.CtfEnv(bridge).reset(map="other_map")

    bridge = FakeBridge([Frame(ctf)])
    bridge.setup_final = Status(4, reason="map: does not fit the loaded map")
    with pytest.raises(RuntimeError, match="rejected between rounds"):
        ctf.CtfEnv(bridge).reset(map="other_map")
    assert bridge.round_ends == 1


def test_a_map_whose_round_has_no_flag_stands_raises(ctf):
    with pytest.raises(RuntimeError, match="no CTF"):
        ctf.CtfEnv(FakeBridge([Frame(ctf, stand_count=1)])).reset(map="no_flags")
    with pytest.raises(RuntimeError, match="no CTF"):
        ctf.CtfEnv(FakeBridge([Frame(ctf, stand_count=0)])).reset(map="no_flags")


def test_the_stands_are_only_checked_when_a_map_is_asked(ctf):
    ctf.CtfEnv(FakeBridge([Frame(ctf, stand_count=0)])).reset()
