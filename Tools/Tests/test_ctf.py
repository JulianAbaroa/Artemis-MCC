"""
test_ctf.py
The environment of stage 0 (Agent/artemis/env/ctf.py) over synthetic frames, with no game: the bridge is a fake that hands out the
ticks of a list. The real Bridge needs Windows shared memory, so a stand-in module takes its place for the import. B4h H.5: the side
of an agent is Head.Side of its View, and the flags come from the View too (F4.9: the observation reads only the View and the Match, and
the frames write Self, Flags and Head of the Views, no Entities, Objectives or Stands, for the observation). P.6 (D255): the clock and
the score of the round come from the Match.
"""
import ctypes
import importlib
import inspect
import math
import os
import sys
import time
from typing import NamedTuple
from unittest import mock

import numpy as np
import pytest

from tdump_writer import Repo

AGENT = os.path.join(Repo(), "Agent")
P0, B0 = 0xEC700000, 0xE5000001
FLAG_RED, FLAG_BLUE, STAND_RED, STAND_BLUE = 0xE5A00001, 0xE5A00002, 0xE5A00003, 0xE5A00004
RED, BLUE = 0, 1
NONE = 0xFFFFFFFF
HOME, CARRIED, DROPPED = 0, 1, 2
MAP_SHIFT = (1.0, 1.0, 0.0)


class FakeTick:
    def __init__(self, layout, players, entities, objectives, stands, events=(), game_tick=100, match_id=1, action_tick=1, match=None,
                 views=(), publish_qpc=0):
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
        self.match_id, self.action_tick, self.publish_qpc = match_id, action_tick, publish_qpc
        # One View per row of Players; a row left out is all zero (not computed).
        self.views = np.zeros(len(players), layout.BridgeView)
        for i, row in enumerate(views):
            self.views[i] = row


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

    def wait_tick(self, timeout, copy=True):
        self.copies = getattr(self, "copies", []) + [copy]
        return self.ticks.pop(0) if self.ticks else None

    def is_unchanged(self, published):
        """P.3 B: the validation of the Views read in place. changed = how many of the next checks say the buffer changed."""
        self.checks = getattr(self, "checks", 0) + 1
        if getattr(self, "changed", 0) > 0:
            self.changed -= 1
            return False
        return True

    def reread_tick(self):
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
    # The real module (its Action, CONTROLS, PackControls) with kernel32 stood in, and a fake Bridge in place of the one on shared memory.
    monkeypatch.setattr(ctypes, "WinDLL", lambda *args, **kwargs: mock.MagicMock(), raising=False)
    bridge_module = importlib.import_module("artemis.bridge")
    bridge_module.Bridge = FakeBridge
    module = importlib.import_module("artemis.env.ctf")
    module.layout = layout
    RESULTS.update({value: layout.BridgeSetupResult(value) for value in (1, 2, 3, 4)})
    yield module
    for name in [n for n in sys.modules if n == "artemis" or n.startswith("artemis.")]:
        sys.modules.pop(name, None)


def ViewRow(layout, team=RED, side=RED, feet=(5.0, 0.0, 0.0), yaw=0.0, pitch=0.0, velocity=(0.0, 0.0, 0.0), vitality=1.0, shield=1.0,
            carries=False, own=None, enemy=None, computed=True, has_body=True, handle=P0, interaction=(0, 0xFF)):
    """One View. own / enemy: (State, Position, StandPosition) of Flags[0] and Flags[1] (HasFlag 1), (None, None, StandPosition) for a
    side whose flag the engine is recreating (HasFlag 0, only the stand), None for a row of zeros. interaction: (Kind, Detail) of
    View.Interaction, by default no selection (Kind 0, Detail 0xFF before the first action, TargetHandle 0xFFFFFFFF)."""
    row = np.zeros(1, layout.BridgeView)[0]
    row["Interaction"]["Kind"], row["Interaction"]["Detail"], row["Interaction"]["TargetHandle"] = *interaction, NONE
    head, me = row["Head"], row["Self"]
    head["PlayerHandle"], head["IsComputed"], head["HasBody"], head["Team"], head["Side"] = handle, int(computed), int(has_body), team, side
    for field, value in (("Feet", feet), ("LinearVelocity", velocity)):
        for axis, v in zip("XYZ", value):
            me[field][axis] = v
    me["FrameYaw"], me["AimPitch"], me["Vitality"], me["Shield"], me["CarriesFlag"] = yaw, pitch, vitality, shield, int(carries)
    for k, flag in enumerate((own, enemy)):
        if flag is not None:
            if flag[0] is not None:
                row["Flags"]["State"][k] = flag[0]
                row["Flags"]["HasFlag"][k] = 1
            for field, value in (("Position", flag[1]), ("StandPosition", flag[2])):
                if value is None:
                    continue
                for axis, v in zip("XYZ", value):
                    row["Flags"][field][axis][k] = v
    return row


def Frame(ctf, owners=(RED, BLUE), flags=(HOME, HOME), flag_at=None, carrying=False, events=(), game_tick=100, agent_at=(5.0, 0.0, 0.0),
          action_tick=1, match_id=1, match=None, vitality=1.0, stand_count=2, local_team=None, shield=1.0, computed=True, publish_qpc=0, recreating=False, interaction=(0, 0xFF)):
    """One tick of a round on a map with a red base at x = 0 and a blue base at x = 100. owners: OwnerTeam of the stands of BaseTeam 0
    and 1 in this round. flags: state of the flag of BaseTeam 0 and 1. flag_at: position of the flag of BaseTeam 1 when it is not
    home. match: fields of the Match (by default a round without time limit). recreating: the own flag does not exist in this tick (the engine
    recreates it): the View keeps the own stand, HasFlag 0. The agent is the player P0 of team RED, aiming at +x."""
    L = ctf.layout
    stand_at = {RED: (0.0, 0.0, 0.0), BLUE: (100.0, 0.0, 0.0)}
    flag_pos = [stand_at[RED], flag_at or stand_at[BLUE]]
    entities = [{"Handle": B0, "WorldOrigin": agent_at, "Team": RED, "Vitality": vitality},
                {"Handle": STAND_RED, "WorldOrigin": stand_at[RED]}, {"Handle": STAND_BLUE, "WorldOrigin": stand_at[BLUE]},
                {"Handle": FLAG_RED, "WorldOrigin": flag_pos[0]}, {"Handle": FLAG_BLUE, "WorldOrigin": flag_pos[1]}]
    players = [{"Handle": P0, "BipedHandle": B0, "Team": RED, "LocalIndex": 0xFF, "IsAlive": 1, "HasBody": 1, "Aim": (1.0, 0.0, 0.0),
                "Shield": 1.0, "Objective": FLAG_BLUE if carrying else NONE}]
    if local_team is not None:
        # The local player 0, the passive rival of the lists (D287), on the team the engine gave it.
        players.append({"Handle": 0xEC700001, "BipedHandle": NONE, "Team": local_team, "LocalIndex": 0, "IsAlive": 1, "HasBody": 1})
    objectives = [{"Handle": FLAG_RED, "BaseTeam": RED, "OwnerTeam": owners[0], "State": flags[0], "CarrierPlayer": NONE, "CarrierBiped": NONE},
                  {"Handle": FLAG_BLUE, "BaseTeam": BLUE, "OwnerTeam": owners[1], "State": flags[1],
                   "CarrierPlayer": P0 if flags[1] == CARRIED else NONE, "CarrierBiped": B0 if flags[1] == CARRIED else NONE}]
    stands = [{"Handle": STAND_RED, "BaseTeam": RED, "OwnerTeam": owners[0]}, {"Handle": STAND_BLUE, "BaseTeam": BLUE, "OwnerTeam": owners[1]}][:stand_count]
    # The View of the agent: Side is the BaseTeam of the stand its team owns, Flags[0] the flag of that side and Flags[1] the other one.
    # note: a View row that is all zero is a side without flag, and the red base of the omniscient rows is the origin, so the positions of
    # the View carry a shift of the whole map (the distances and the directions do not change).
    def Shifted(p):
        return tuple(a + b for a, b in zip(p, MAP_SHIFT))
    side = owners.index(RED) if RED in owners else None
    own = enemy = None
    if side is not None:
        own = (None if recreating else flags[side], None if recreating else Shifted(flag_pos[side]), Shifted(stand_at[side]))
        enemy = (flags[1 - side], Shifted(flag_pos[1 - side]), Shifted(stand_at[1 - side]))
    views = [ViewRow(L, side=side if side is not None else ctf.NEUTRAL_TEAM, feet=Shifted(agent_at), vitality=vitality, shield=shield,
                     carries=carrying, own=own, enemy=enemy, computed=computed, interaction=interaction)]
    return FakeTick(L, players, entities, objectives, stands, events, game_tick, match_id=match_id, action_tick=action_tick, match=match,
                    views=views, publish_qpc=publish_qpc)


def Reset(ctf, *frames):
    env = ctf.CtfEnv(FakeBridge(frames))
    observations, _ = env.reset()
    return env, observations[P0]


def test_enemy_flag_at_its_stand_is_home(ctf):
    env, obs = Reset(ctf, Frame(ctf))
    assert env.states[P0].side == RED
    assert obs.shape == (ctf.ACTION_AT + sum(ctf.ActionNvec("stage0")),)
    assert obs[11] == 1.0 and obs[12] == 0.0 and obs[6] == 0.0
    assert obs[18] == 1.0
    assert obs[10] == pytest.approx(95.0 / ctf.METERS)
    # Facing +x: the enemy flag is straight ahead and the own stand behind.
    assert obs[7] > 0 and obs[14] < 0


def test_the_own_stand_is_seen_while_the_own_flag_is_recreated(ctf):
    _, whole = Reset(ctf, Frame(ctf))
    env, recreated = Reset(ctf, Frame(ctf, recreating=True))
    # The stand is still there: its vector and distance, and the enemy flag, are the same as with the flag; only obs[18] (the own flag is home) is off.
    assert env.states[P0].side == RED
    assert recreated[14] < 0 and recreated[17] > 0
    keep = np.arange(len(whole)) != 18
    assert np.array_equal(whole[keep], recreated[keep])
    assert whole[18] == 1.0 and recreated[18] == 0.0
    # The shaping (phi) does not switch off: the same potential as with the flag.
    view = lambda **kw: Frame(ctf, **kw).views[0]
    match = Frame(ctf).match
    nvec, last = ctf.ActionNvec("stage0"), np.zeros(len(ctf.ActionNvec("stage0")), dtype=int)
    _, phi_whole = ctf.Observe(view(), match, last, nvec, 120.0)
    _, phi_recreated = ctf.Observe(view(recreating=True), match, last, nvec, 120.0)
    assert phi_whole is not None and phi_recreated == pytest.approx(phi_whole)


def test_obs_4_follows_the_vitality_of_the_biped_and_nan_is_zero(ctf):
    _, obs = Reset(ctf, Frame(ctf, vitality=0.25))
    assert obs[4] == pytest.approx(0.25)
    _, obs = Reset(ctf, Frame(ctf, vitality=float("nan")))
    assert obs[4] == 0.0


def test_carried_flag_makes_the_agent_a_carrier_that_heads_for_its_stand(ctf):
    frame = Frame(ctf, flags=(HOME, CARRIED), flag_at=(5.0, 0.0, 0.5), carrying=True)
    env = ctf.CtfEnv(FakeBridge([frame]))
    env.reset()
    obs, phi = env._observe_all()[P0]
    assert obs[6] == 1.0 and obs[13] == 1.0
    assert obs[11] == 0.0 and obs[12] == 0.0
    assert phi == pytest.approx(-5.0 / ctf.METERS)


def test_dropped_flag_is_flagged_as_dropped_and_shaped_through_it(ctf):
    frame = Frame(ctf, flags=(HOME, DROPPED), flag_at=(50.0, 0.0, 0.0))
    env = ctf.CtfEnv(FakeBridge([frame]))
    env.reset()
    obs, phi = env._observe_all()[P0]
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
    assert env.states[P0].side == ctf.NEUTRAL_TEAM
    assert not obs[7:19].any()


def test_a_step_writes_the_agent_input_of_the_profile(ctf):
    bridge = FakeBridge([Frame(ctf, action_tick=1), Frame(ctf, action_tick=2)])
    env = ctf.CtfEnv(bridge, ctf.CtfConfig(turn_limit=0.1))
    env.reset()
    # forward, left nothing, yaw to the right, pitch to the middle, jump press and hold, crouch nothing
    env.step({P0: np.array([2, 1, 8, 2, 2, 0, 0])})
    tick, actions = bridge.written[-1]
    action = actions[P0 & 0xFFFF]
    assert tick == 1 and action.handle == P0
    assert (action.forward, action.left) == (1.0, 0.0)
    assert action.yaw_delta == pytest.approx(5 * 0.1) and action.pitch_delta == 0.0  # N = d = 5 ticks of the turn limit (D375)
    assert (action.level, action.edge) == (1, 1)


def test_the_use_control_of_the_profile_is_level_and_edge_by_state(ctf):
    use = importlib.import_module("artemis.bridge").CONTROLS.index("Use")
    assert ctf.PROFILES["stage0"] == ("Jump", "Crouch", "Use") and len(ctf.ActionNvec("stage0")) == 7 and ctf.ActionNvec("stage0")[-1] == 4
    # nothing, tap (edge), press and hold (level and edge), still held (level)
    for state, (level, edge) in enumerate(((0, 0), (0, 1), (1, 1), (1, 0))):
        bridge = FakeBridge([Frame(ctf, action_tick=1), Frame(ctf, action_tick=2)])
        env = ctf.CtfEnv(bridge)
        env.reset()
        env.step({P0: np.array([1, 1, 4, 2, 0, 0, state])})
        action = bridge.written[-1][1][P0 & 0xFFFF]
        assert (action.level >> use & 1, action.edge >> use & 1) == (level, edge) and action.level & ~(1 << use) == 0 and action.edge & ~(1 << use) == 0


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
    env = ctf.CtfEnv(FakeBridge([start, taken, again, captured]), ctf.CtfConfig(shaping=0.1))
    env.reset()
    nothing = np.zeros(len(ctf.ActionNvec("stage0")), dtype=np.int64)
    config = env.config
    _, rewards, _, _, infos = env.step({P0: nothing})
    assert infos[P0]["events"] == ["FlagTaken"]
    assert rewards[P0] >= config.take_reward - 0.1
    _, rewards, _, _, infos = env.step({P0: nothing})
    assert infos[P0]["events"] == ["FlagTaken"]  # reported (PL.1), not paid
    assert rewards[P0] < config.take_reward - 0.1
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
    nothing = np.zeros(len(ctf.ActionNvec("stage0")), dtype=np.int64)
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
    # GameTick is read once, to tell a new match from the same one (D372), never as a clock of the episode.
    assert source.count('["GameTick"]') == 1 and "start_tick" not in source


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


class FakeSetups:
    """Stands for a TrainingList: hands out a setup per call."""
    def __init__(self, *setups):
        self.setups = list(setups)

    def next_setup(self):
        return self.setups.pop(0)


def test_an_environment_with_setups_asks_for_the_next_one_at_every_reset(ctf):
    bridge = FakeBridge([Frame(ctf), Frame(ctf, match_id=2)])
    env = ctf.CtfEnv(bridge, setups=FakeSetups({"preset": "a", "map": "Map A", "roster": {"0": 1}}, {"map": "Map B"}))
    env.reset()
    env.reset()
    assert [request["map"] for request in bridge.setup_requests] == ["Map A", "Map B"]
    assert bridge.setup_requests[0]["roster"] == {"0": 1}


def test_a_reset_with_its_own_setup_does_not_take_one_from_the_setups(ctf):
    bridge = FakeBridge([Frame(ctf)])
    setups = FakeSetups({"map": "Map A"})
    ctf.CtfEnv(bridge, setups=setups).reset(preset="p")
    assert [request["preset"] for request in bridge.setup_requests] == ["p"]
    assert len(setups.setups) == 1


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


# --- roster relative to the local player (D287) and a match that ends -------------------------------------------------------------------


def test_a_relative_roster_is_resolved_from_the_team_of_the_local_player_before_the_request(ctf):
    for local_team, expected in ((RED, {"0": 2, "1": 3}), (BLUE, {"1": 2, "0": 3})):
        bridge = FakeBridge([Frame(ctf, local_team=local_team), Frame(ctf, local_team=local_team, action_tick=2)])
        ctf.CtfEnv(bridge).reset(roster={"auto": 2, "rival": 3})
        assert bridge.setup_requests == [{"preset": None, "map": None, "roster": expected}]


def test_a_roster_with_team_keys_is_sent_as_it_is_and_needs_no_tick_before(ctf):
    bridge = FakeBridge([Frame(ctf)])
    ctf.CtfEnv(bridge).reset(roster={"0": 1, "1": 1})
    assert bridge.setup_requests[0]["roster"] == {"0": 1, "1": 1}


def test_a_relative_roster_without_a_local_player_raises(ctf):
    bridge = FakeBridge([Frame(ctf)])
    with pytest.raises(RuntimeError, match="no local player"):
        ctf.CtfEnv(bridge).reset(roster={"rival": 1})
    assert bridge.setup_requests == []


class EndedBridge(FakeBridge):
    """A bridge whose match ended: no more ticks, and the phase it publishes."""
    def __init__(self, ticks, phase=None):
        super().__init__(ticks)
        self.phase = phase
        self.waits = 0

    def wait_tick(self, timeout, copy=True):
        self.waits += 1
        return super().wait_tick(timeout, copy)


def test_reset_raises_and_does_not_hang_when_the_match_ended_with_a_team_left_empty(ctf):
    # The round ended with the agent bot and the local player on team 0 and nobody on team 1: Reach ended the match.
    bridge = EndedBridge([Frame(ctf, local_team=RED)], phase=ctf.layout.BridgePhase.Idle)
    env = ctf.CtfEnv(bridge, ctf.CtfConfig(tick_timeout=0.001, reset_timeout=1.0, match_gap_timeout=0.001))
    with pytest.raises(RuntimeError, match="match ended by the gametype: a team was left empty"):
        env.reset(roster={"0": 1})
        env.reset()
    assert bridge.waits <= 3


def test_reset_gives_up_after_reset_timeout_when_no_tick_comes_and_the_phase_says_nothing(ctf):
    bridge = EndedBridge([Frame(ctf)])
    env = ctf.CtfEnv(bridge, ctf.CtfConfig(tick_timeout=0.25, reset_timeout=1.0))
    env.reset()
    with pytest.raises(RuntimeError, match="no tick for"):
        env.step({P0: np.zeros(len(ctf.ActionNvec("stage0")), dtype=np.int64)})
    assert bridge.waits <= 6


def test_a_reset_whose_round_never_starts_raises_a_timeout(ctf):
    def dead():
        frame = Frame(ctf)
        frame.players[0]["IsAlive"] = 0
        return frame
    bridge = FakeBridge([dead() for _ in range(5000)])
    env = ctf.CtfEnv(bridge, ctf.CtfConfig(tick_timeout=0.001, reset_timeout=0.05))
    # Ticks come but the bot never lives: the wall clock bounds the wait.
    with pytest.raises(TimeoutError, match="no round with every agent alive"):
        env.reset()


# --- F4.9: the observation of the policy comes from the View and the Match -------------------------------------------------------


def Nothing(ctf):
    return np.zeros(len(ctf.ActionNvec("stage0")), dtype=np.int64)


def test_observation_reads_only_the_view_and_the_match(ctf):
    L = ctf.layout
    nvec = ctf.ActionNvec("stage0")
    last = np.array([2, 1, 8, 2, 2, 0, 0])
    # Facing +y (yaw pi / 2), at (10, 20, 1), moving 5 m/s toward +x (the agent's right), carrying nothing; the enemy flag lies 30 m ahead
    # (+y) and 40 m to the right (+x) and the own stand is 10 m behind.
    view = ViewRow(L, side=BLUE, feet=(10.0, 20.0, 1.0), yaw=math.pi / 2, pitch=0.25, velocity=(5.0, 0.0, 0.0), vitality=0.5, shield=0.75,
                   own=(HOME, (10.0, 10.0, 1.0), (10.0, 10.0, 1.0)), enemy=(DROPPED, (50.0, 50.0, 1.0), (200.0, 0.0, 0.0)))
    match = Frame(ctf, match={"RoundClockTicks": 1800, "RoundTicksRemaining": -1, "ScoreToWin": 4, "TeamRoundScores": [1, 3, 0, 0, 0, 0, 0, 0]}).match
    obs, phi = ctf.Observe(view, match, last, nvec, 120.0)
    M = ctf.METERS
    # Local frame: x forward (+y of the world), y to the left (-x of the world).
    assert obs[0:3] == pytest.approx([0.0, -0.5, 0.0], abs=1e-6)
    assert obs[3] == pytest.approx(0.25) and obs[4] == pytest.approx(0.5) and obs[5] == pytest.approx(0.75)
    assert obs[6] == 0.0 and obs[13] == 0.0
    assert obs[7:10] == pytest.approx([30.0 / M, -40.0 / M, 0.0], abs=1e-6) and obs[10] == pytest.approx(50.0 / M)
    assert obs[11] == 0.0 and obs[12] == 1.0
    assert obs[14:17] == pytest.approx([-10.0 / M, 0.0, 0.0], abs=1e-6) and obs[17] == pytest.approx(10.0 / M)
    assert obs[18] == 1.0
    assert obs[19] == pytest.approx(0.75)
    # Own team RED has 1 of 4: 3/4 left; the enemy has the best other score, 3 of 4: 1/4 left.
    assert obs[20] == pytest.approx(0.75) and obs[21] == pytest.approx(0.25)
    assert phi == pytest.approx(-(50.0 + math.dist((50, 50, 1), (10, 10, 1))) / M)
    assert obs[ctf.ACTION_AT:].sum() == len(nvec)
    assert obs[ctf.ACTION_AT + 2] == 1.0 and obs[ctf.ACTION_AT + 3 + 1] == 1.0
    # The omniscient rows of the tick say the opposite and change nothing.
    env_a, a = Reset(ctf, Frame(ctf))
    frame = Frame(ctf)
    frame.entities["WorldOrigin"]["X"] += 500.0
    frame.entities["Vitality"] = 0.1
    frame.players["Aim"]["Y"], frame.players["Aim"]["X"] = 1.0, 0.0
    frame.players["Shield"] = 0.0
    frame.players["Objective"] = FLAG_BLUE
    frame.objectives["State"] = DROPPED
    frame.objectives["BaseTeam"] = frame.objectives["BaseTeam"][::-1]
    frame.stands["OwnerTeam"] = frame.stands["OwnerTeam"][::-1]
    _, b = Reset(ctf, frame)
    assert np.array_equal(a, b)


def test_the_environment_no_longer_reads_entities_objectives_or_stands_for_the_observation(ctf):
    for function in (ctf.Observe, ctf.ObserveBatch, ctf.CtfEnv._observe_all, ctf.TimeLeft, ctf.ScoreLeft):
        source = inspect.getsource(function).lower()
        for word in ("entities", "objectives", "stands", "bipedhandle", "players"):
            assert word not in source, (function.__name__, word)


def test_reset_waits_until_the_views_of_every_agent_are_computed(ctf):
    # The tick in which the mask is set has no View yet: the environment answers with nothing and goes on until it has one.
    bridge = FakeBridge([Frame(ctf, computed=False, action_tick=1), Frame(ctf, computed=False, action_tick=2), Frame(ctf, action_tick=3)])
    env = ctf.CtfEnv(bridge)
    observations, _ = env.reset()
    assert [tick for tick, _ in bridge.written] == [1, 2] and all(actions == {} for _, actions in bridge.written)
    assert bridge.controls[-1]["agents"] == 1 << (P0 & 0xFFFF)
    assert observations[P0][11] == 1.0 and observations[P0][4] == 1.0
    # Views that never come: a clear TimeoutError, no endless wait.
    class Slow(FakeBridge):
        def wait_tick(self, timeout, copy=True):
            time.sleep(0.002)
            return super().wait_tick(timeout, copy)
    env = ctf.CtfEnv(Slow([Frame(ctf, computed=False) for _ in range(5000)]), ctf.CtfConfig(tick_timeout=0.001, reset_timeout=0.1))
    with pytest.raises(TimeoutError, match="Views of the agents were not computed"):
        env.reset()


def test_an_agent_without_a_computed_view_observes_no_body(ctf):
    env = ctf.CtfEnv(FakeBridge([Frame(ctf, action_tick=1), Frame(ctf, computed=False, action_tick=2, match=Clock(1800))]))
    env.reset()
    last = np.array([2, 1, 8, 2, 2, 0, 0])
    observations, rewards, _, _, _ = env.step({P0: last})
    obs = observations[P0]
    assert not obs[:19].any() and not obs[20:22].any()
    assert obs[19] == pytest.approx(0.75)
    assert obs[ctf.ACTION_AT:].sum() == len(last) and env.states[P0].phi is None
    # The same for an agent that left the Players (no row, no View) and for a body that is gone.
    env = ctf.CtfEnv(FakeBridge([Frame(ctf, action_tick=1)]))
    env.reset()
    env.published.players["Handle"] = 0xEC7000FF
    env._rows = None
    observed = env._observe_all()[P0]
    assert observed[1] is None and not observed[0][:19].any()
    view = ViewRow(ctf.layout, has_body=False)
    obs, phi = ctf.Observe(view, Frame(ctf).match, last, ctf.ActionNvec("stage0"), 120.0)
    assert phi is None and not obs[:19].any()


def test_enemy_score_is_the_best_other_team(ctf):
    L = ctf.layout
    nvec = ctf.ActionNvec("stage0")
    last = np.zeros(len(nvec), dtype=np.int64)

    def Scores(team, scores, to_win=5):
        match = Frame(ctf, match={"ScoreToWin": to_win, "TeamRoundScores": scores + [0] * (8 - len(scores))}).match
        obs, _ = ctf.Observe(ViewRow(L, team=team), match, last, nvec, 120.0)
        return float(obs[20]), float(obs[21])
    assert Scores(RED, [1, 4]) == pytest.approx((0.8, 0.2))
    assert Scores(BLUE, [1, 4]) == pytest.approx((0.2, 0.8))
    # More than two teams: the enemy is the other team that is nearest to winning, not the first one.
    assert Scores(0, [2, 0, 3, 1]) == pytest.approx((0.6, 0.4))
    assert Scores(2, [2, 0, 3, 1]) == pytest.approx((0.4, 0.6))


def test_shield_nan_is_zero(ctf):
    _, obs = Reset(ctf, Frame(ctf, shield=0.5))
    assert obs[5] == pytest.approx(0.5)
    _, obs = Reset(ctf, Frame(ctf, shield=float("nan")))
    assert obs[5] == 0.0 and np.all(np.isfinite(obs))


def test_timing_is_off_by_default_and_counts_each_phase_when_on(ctf, monkeypatch):
    assert ctf.CtfConfig().timing is False
    frames = lambda: [Frame(ctf, action_tick=1, publish_qpc=1000), Frame(ctf, action_tick=2, publish_qpc=1200), Frame(ctf, action_tick=3)]
    env = ctf.CtfEnv(FakeBridge(frames()))
    env.reset()
    env.step({P0: Nothing(ctf)})
    env.step({P0: Nothing(ctf)})
    assert all(not values for values in env.timings.values()) and env.slack_ms == []

    bridge = FakeBridge(frames())
    bridge.qpc_frequency, bridge.last_copy_ns = 1000, 777
    monkeypatch.setattr(ctf, "qpc", lambda: 1500)
    env = ctf.CtfEnv(bridge, ctf.CtfConfig(timing=True))
    env.reset()
    assert all(not values for values in env.timings.values())  # the reset is not a decision
    env.step({P0: Nothing(ctf)})
    env.step({P0: Nothing(ctf)})
    assert tuple(env.timings) == ctf.PHASES
    assert all(len(values) == 2 for values in env.timings.values())
    assert env.timings["copy"] == [777, 777] and all(v >= 0 for values in env.timings.values() for v in values)
    # publish -> answer: (1500 - publish_qpc) counts of 1 ms each; the first step answers the tick of publish 1000, the second the one of 1200.
    assert env.slack_ms == pytest.approx([500.0, 300.0])


# --- F5.1 B: interaction in the observation, new match, Seek -------------------------------------------------------------------


def InteractionEntries(ctf, kind, detail=0, **kw):
    L = ctf.layout
    view = ViewRow(L, side=RED, own=(HOME, (0.0, 0.0, 0.0), (0.0, 0.0, 0.0)), enemy=(HOME, (100.0, 0.0, 0.0), (100.0, 0.0, 0.0)),
                   interaction=(kind, detail), **kw)
    obs, _ = ctf.Observe(view, Frame(ctf).match, Nothing(ctf), ctf.ActionNvec("stage0"), 120.0)
    return obs[ctf.INTERACTION_AT:ctf.ACTION_AT]


def test_view_row_and_frame_accept_the_interaction_with_no_selection_by_default(ctf):
    view = Frame(ctf).views[0]
    assert (int(view["Interaction"]["Kind"]), int(view["Interaction"]["Detail"]), int(view["Interaction"]["TargetHandle"])) == (0, 0xFF, NONE)
    view = Frame(ctf, interaction=(10, 0)).views[0]
    assert (int(view["Interaction"]["Kind"]), int(view["Interaction"]["Detail"])) == (10, 0)
    assert (ctf.STATE_SIZE, ctf.INTERACTION_SIZE, ctf.ACTION_AT) == (22, 7, 29)


def test_interaction_kind_is_one_hot_in_the_observation(ctf):
    kinds = list(ctf.BridgeInteractionKind)
    assert len(kinds) == 7 == ctf.INTERACTION_SIZE
    for i, kind in enumerate(kinds):
        entries = InteractionEntries(ctf, int(kind))
        assert entries.sum() == 1.0 and entries[i] == 1.0, kind
    assert ctf.GRAB_OBJECTIVE_AT == ctf.INTERACTION_AT + 6


def test_no_selection_is_the_none_entry_whatever_the_detail(ctf):
    for detail in (0xFF, 0x00):
        entries = InteractionEntries(ctf, 0, detail)
        assert entries[0] == 1.0 and entries.sum() == 1.0
    assert np.array_equal(InteractionEntries(ctf, 10, 0xFF), InteractionEntries(ctf, 10, 0x00))


def test_no_body_or_no_view_has_no_interaction_entries(ctf):
    assert not InteractionEntries(ctf, 10, has_body=False).any()
    assert not InteractionEntries(ctf, 10, computed=False).any()
    obs, phi = ctf.Observe(None, Frame(ctf).match, Nothing(ctf), ctf.ActionNvec("stage0"), 120.0)
    assert not obs[ctf.INTERACTION_AT:ctf.ACTION_AT].any() and phi is None


def test_an_unknown_kind_sets_no_entry(ctf):
    for kind in (1, 5, 7, 11, 255):
        assert not InteractionEntries(ctf, kind).any(), kind


def test_the_interaction_reaches_the_observation_of_a_step(ctf):
    env = ctf.CtfEnv(FakeBridge([Frame(ctf, action_tick=1), Frame(ctf, action_tick=2, interaction=(10, 0))]))
    env.reset()
    observations, *_ = env.step({P0: Nothing(ctf)})
    assert observations[P0][ctf.GRAB_OBJECTIVE_AT] == 1.0


def Crossing(ctf, first, second):
    env = ctf.CtfEnv(FakeBridge([first, second]))
    env.reset()
    return env, env.step({P0: Nothing(ctf)})


def test_a_new_match_truncates_the_episode(ctf):
    env, (_, _, terminated, truncated, _) = Crossing(ctf, Frame(ctf, match_id=1, game_tick=100), Frame(ctf, action_tick=2, match_id=2, game_tick=104))
    assert truncated[P0] and not terminated[P0] and env.is_waiting_round


def test_a_lower_game_tick_truncates_the_episode(ctf):
    env, (_, _, terminated, truncated, _) = Crossing(ctf, Frame(ctf, game_tick=100), Frame(ctf, action_tick=2, game_tick=40))
    assert truncated[P0] and not terminated[P0] and env.is_waiting_round
    # The same match and the clock going on: nothing happens.
    env, (_, _, terminated, truncated, _) = Crossing(ctf, Frame(ctf, game_tick=100), Frame(ctf, action_tick=2, game_tick=104))
    assert not truncated[P0] and not terminated[P0] and not env.is_waiting_round


def test_round_end_and_new_match_in_one_step_terminates(ctf):
    end = {"Sequence": 1, "Kind": ctf.Event.GameOver, "CausePlayer": NONE, "EffectPlayer": NONE}
    env, (_, _, terminated, truncated, _) = Crossing(ctf, Frame(ctf, match_id=1, game_tick=100),
                                                      Frame(ctf, action_tick=2, match_id=2, game_tick=4, events=[end]))
    assert terminated[P0] and not truncated[P0] and env.is_waiting_round


def test_reset_after_a_new_match_waits_for_its_round(ctf):
    for second in (Frame(ctf, action_tick=2, match_id=2, game_tick=4), Frame(ctf, action_tick=2, game_tick=4)):
        bridge = FakeBridge([Frame(ctf, game_tick=100), second])
        env = ctf.CtfEnv(bridge)
        env.reset()
        _, _, _, truncated, _ = env.step({P0: Nothing(ctf)})
        assert truncated[P0]
        # The new match has no body yet, then the round starts with a Spawn: reset() goes on past the ticks without a round.
        dead = Frame(ctf, action_tick=3, match_id=second.match_id, game_tick=8)
        dead.players["IsAlive"] = 0
        spawn = {"Sequence": 1, "Kind": ctf.Event.Spawn, "CausePlayer": P0, "EffectPlayer": NONE}
        bridge.ticks += [dead, Frame(ctf, action_tick=4, match_id=second.match_id, game_tick=12, events=[spawn])]
        observations, _ = env.reset()
        assert P0 in observations and not bridge.ticks and not env.is_waiting_round
        # The episode of the new round runs: the reference is the one of the new tick.
        bridge.ticks.append(Frame(ctf, action_tick=5, match_id=second.match_id, game_tick=16))
        _, _, terminated, truncated, _ = env.step({P0: Nothing(ctf)})
        assert not truncated[P0] and not terminated[P0]


def EnvCheck(ctf):
    """Agent/scripts/env_check.py imported over the stand-in bridge of the ctf fixture."""
    import importlib.util
    spec = importlib.util.spec_from_file_location("env_check_under_test", os.path.join(AGENT, "scripts", "env_check.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class Rng:
    def __init__(self, value):
        self.value = value

    def random(self):
        return self.value


def SeekWith(ctf, last_use=0, interaction=(0, 0xFF), carrying=False, flags=(HOME, HOME), jump=1.0, **kw):
    """The Seek action over the observation of a frame; last_use: state of Use in the last action; jump: what rng.random() gives."""
    config = ctf.CtfConfig()
    nvec = ctf.ActionNvec("stage0")
    last = np.array([2, 1, 4, 2, 0, 0, last_use])
    frame = Frame(ctf, interaction=interaction, carrying=carrying, flags=flags, **kw)
    obs, _ = ctf.Observe(frame.views[0], frame.match, last, nvec, 120.0)
    return EnvCheck(ctf).Seek(obs, config, Rng(jump))


def test_seek_holds_use_on_grab_objective_2_and_then_3(ctf):
    first = SeekWith(ctf, last_use=0, interaction=(10, 0))
    assert first[4:].tolist() == [0, 0, 2]
    for last_use, state in ((1, 2), (2, 3), (3, 3)):
        assert SeekWith(ctf, last_use=last_use, interaction=(10, 0))[6] == state, last_use


def test_seek_stops_while_grabbing(ctf):
    throttle = ctf.THROTTLE
    assert throttle[SeekWith(ctf, interaction=(10, 0))[0]] == 0.0
    assert throttle[SeekWith(ctf, interaction=(10, 0), jump=0.0)[0]] == 0.0
    # The yaw still points at the flag (straight ahead here: the middle bin).
    assert SeekWith(ctf, interaction=(10, 0))[2] == 4


def test_seek_releases_use_when_carrying_or_without_selection(ctf):
    carried = dict(carrying=True, flags=(HOME, CARRIED), flag_at=(5.0, 0.0, 0.5))
    assert SeekWith(ctf, last_use=3, interaction=(10, 0), **carried)[6] == 0
    assert SeekWith(ctf, last_use=3)[6] == 0
    assert SeekWith(ctf, last_use=2, interaction=(2, 0))[6] == 0
    assert SeekWith(ctf, last_use=2, interaction=(0, 0))[6] == 0


def test_seek_is_unchanged_without_interaction(ctf):
    throttle = ctf.THROTTLE
    walk = SeekWith(ctf)
    assert walk.tolist() == [2, 1, 4, 2, 0, 0, 0] and throttle[walk[0]] == 1.0
    assert SeekWith(ctf, jump=0.0).tolist() == [2, 1, 4, 2, 1, 0, 0]
    # Carrying: the stand (behind, the agent faces +x) decides the yaw, as before.
    behind = SeekWith(ctf, carrying=True, flags=(HOME, CARRIED), flag_at=(5.0, 0.0, 0.5))
    assert behind[0] == 2 and behind[2] in (0, len(ctf.YAW_STEPS) - 1) and behind[6] == 0


# --- ObserveBatch (P.3): the vectorized observation against the one of a row at a time -------------------------------------------------

def ReferenceScoreLeft(match, team):
    """Per-team ScoreLeft as it was before P.3 (the oracle of ObserveBatch): (own, enemy) of one team."""
    to_win = int(match["ScoreToWin"])
    if to_win <= 0:
        return 0.0, 0.0
    scores = [int(score) for score in match["TeamRoundScores"]]

    def Left(score):
        return max(0, to_win - score) / to_win
    others = [score for t, score in enumerate(scores) if t != team]
    return Left(scores[team]) if 0 <= team < len(scores) else 0.0, Left(max(others)) if others else 0.0


def ReferenceHasStand(flag):
    return any(float(flag["StandPosition"][axis]) != 0.0 for axis in "XYZ")


def ReferenceObservePerRow(ctf, view, match, last_action, nvec, max_seconds):
    """The Observe of one View as it was before P.3 (a Python call per agent, field by field): the oracle of ObserveBatch. Not used by
    the package (rules 1.2: one implementation there)."""
    obs = np.zeros(ctf.ACTION_AT + sum(nvec), dtype=np.float32)
    offset = ctf.ACTION_AT
    for branch, n in zip(last_action, nvec):
        obs[offset + int(branch)] = 1.0
        offset += n
    obs[19] = ctf.TimeLeft(match, max_seconds)
    if view is None or not int(view["Head"]["IsComputed"]):
        return obs, None
    head, me = view["Head"], view["Self"]
    obs[20:22] = ReferenceScoreLeft(match, int(head["Team"]))
    if not int(head["HasBody"]):
        return obs, None
    kind = int(view["Interaction"]["Kind"])
    if kind in ctf.INTERACTION_KINDS:
        obs[ctf.INTERACTION_AT + ctf.INTERACTION_KINDS.index(kind)] = 1.0
    position = ctf.Vec(me["Feet"])
    yaw, pitch = float(me["FrameYaw"]), float(me["AimPitch"])
    cos, sin = math.cos(-yaw), math.sin(-yaw)

    def Local(v):
        return np.array([cos * v[0] - sin * v[1], sin * v[0] + cos * v[1], v[2]], dtype=np.float32)

    is_carrying = int(me["CarriesFlag"]) != 0
    obs[0:3] = Local(ctf.Vec(me["LinearVelocity"])) / 10.0
    obs[3] = pitch
    vitality, shield = float(me["Vitality"]), float(me["Shield"])
    obs[4] = 0.0 if math.isnan(vitality) else vitality
    obs[5] = 0.0 if math.isnan(shield) else shield
    obs[6] = float(is_carrying)
    flags = view["Flags"]
    if int(head["Side"]) == ctf.NEUTRAL_TEAM:
        return obs, None
    own, enemy = flags[0], flags[1]
    has_stand = ReferenceHasStand(own)
    stand_position = ctf.Vec(own["StandPosition"])
    phi = None
    if int(enemy["HasFlag"]):
        flag_position = ctf.Vec(enemy["Position"])
        to_flag = flag_position - position
        obs[7:10] = Local(to_flag) / ctf.METERS
        obs[10] = np.linalg.norm(to_flag) / ctf.METERS
        flag_state = int(enemy["State"])
        obs[11] = float(flag_state == ctf.FlagState.Home)
        obs[12] = float(flag_state == ctf.FlagState.Dropped)
        obs[13] = float(is_carrying)
        if has_stand:
            if is_carrying:
                phi = -np.linalg.norm(stand_position - position) / ctf.METERS
            else:
                phi = -(np.linalg.norm(to_flag) + np.linalg.norm(stand_position - flag_position)) / ctf.METERS
    if has_stand:
        to_stand = stand_position - position
        obs[14:17] = Local(to_stand) / ctf.METERS
        obs[17] = np.linalg.norm(to_stand) / ctf.METERS
        obs[18] = float(int(own["HasFlag"]) != 0 and int(own["State"]) == ctf.FlagState.Home)
    return obs, (float(phi) if phi is not None else None)


def BatchDifferences(ctf, views, match, last_actions, nvec, max_seconds=120.0):
    """The rows where ObserveBatch differs from the oracle row by row (obs 1e-6, potential equal within 1e-6 or both none), as a list of
    (row, entry or -1 for the potential)."""
    obs, phi = ctf.ObserveBatch(views, match, last_actions, nvec, max_seconds)
    assert obs.dtype == np.float32 and phi.dtype == np.float64
    assert obs.shape == (len(views), ctf.ACTION_AT + sum(nvec)) and phi.shape == (len(views),)
    bad = []
    for i, view in enumerate(views):
        expected, expected_phi = ReferenceObservePerRow(ctf, view, match, last_actions[i], nvec, max_seconds)
        bad += [(i, int(entry)) for entry in np.flatnonzero(np.abs(obs[i].astype(np.float64) - expected) > 1e-6)]
        if np.isnan(phi[i]) != (expected_phi is None) or (expected_phi is not None and abs(phi[i] - expected_phi) > 1e-6):
            bad.append((i, -1))
    return bad


def CoveringViews(ctf):
    """Views for every branch of Observe: no View, not computed, no body, neutral side, enemy flag without HasFlag, no own stand, own flag
    being recreated, carrying with and without the enemy flag Carried, Dropped, Home, NaN vitality and shield, every Kind of the enum and
    some outside it, varied yaws and positions and teams 0, 1 and others. Returns (views, match, last_actions)."""
    L = ctf.layout
    stand_a, stand_b = (1.0, 2.0, 0.0), (101.0, 2.0, 0.0)
    own = (HOME, stand_a, stand_a)
    enemy = (HOME, stand_b, stand_b)
    cases = [
        {"zero": True},
        {"computed": False},
        {"has_body": False, "team": 1},
        {"side": ctf.NEUTRAL_TEAM, "team": 5},
        {"enemy": (None, None, stand_b)},
        {"own": (HOME, stand_a, None)},
        {"own": (None, None, stand_a)},
        {"carries": True, "enemy": (CARRIED, (3.0, 4.0, 1.0), stand_b)},
        {"enemy": (CARRIED, (30.0, -4.0, 1.0), stand_b)},
        {"enemy": (DROPPED, (50.0, 20.0, 2.0), stand_b), "carries": True},
        {"enemy": (DROPPED, (-50.0, 20.0, 2.0), stand_b)},
        {"vitality": float("nan"), "shield": float("nan")},
        {"own": (DROPPED, (9.0, 9.0, 0.0), stand_a), "team": 9},
        {"enemy": (HOME, stand_b, None), "carries": True},
    ]
    cases += [{"interaction": (int(kind), 0x11)} for kind in ctf.BridgeInteractionKind]
    cases += [{"interaction": (kind, 0)} for kind in (3, 255)]
    rows = []
    for i, case in enumerate(cases):
        if case.get("zero"):
            rows.append(np.zeros(1, L.BridgeView)[0])
            continue
        kw = {"side": case.get("side", [RED, BLUE][i % 2]), "team": case.get("team", i % 2),
              "feet": (5.0 + 3.1 * i, -7.0 + 1.3 * i, 0.5 * (i % 3)), "yaw": -3.0 + 0.7 * i, "pitch": 0.05 * i - 0.4,
              "velocity": (2.0 - 0.3 * i, 0.4 * i, 0.1 * i), "vitality": 0.5 + 0.02 * i, "shield": 1.0 - 0.03 * i,
              "own": own, "enemy": enemy}
        kw.update({k: v for k, v in case.items() if k not in ("zero",)})
        rows.append(ViewRow(L, **kw))
    views = np.zeros(len(rows), L.BridgeView)
    for i, row in enumerate(rows):
        views[i] = row
    match = Frame(ctf, match={"RoundClockTicks": 1800, "RoundTicksRemaining": 5000, "RoundTimeLimitTicks": 7200, "ScoreToWin": 4,
                              "TeamRoundScores": [1, 3, 0, 2, 0, 0, 0, 0]}).match
    nvec = ctf.ActionNvec("stage0")
    rng = np.random.default_rng(7)
    last = np.stack([rng.integers(0, n, size=len(views)) for n in nvec], axis=1).astype(np.int64)
    return views, match, last


def test_observe_batch_equals_the_per_row_observe(ctf):
    views, match, last = CoveringViews(ctf)
    assert len(views) >= 16
    nvec = ctf.ActionNvec("stage0")
    assert BatchDifferences(ctf, views, match, last, nvec) == []
    # Other clocks and scores: no score to win, a cap shorter than the round.
    other = Frame(ctf, match={"RoundClockTicks": 300, "RoundTicksRemaining": -1, "ScoreToWin": 0}).match
    assert BatchDifferences(ctf, views, other, last, nvec, 60.0) == []
    # The covered branches are really different from each other (the test would pass on all zeros too).
    obs, phi = ctf.ObserveBatch(views, match, last, nvec, 120.0)
    assert np.isnan(phi).sum() > 5 and (~np.isnan(phi)).sum() > 5
    assert obs[:, ctf.INTERACTION_AT:ctf.ACTION_AT].sum() >= 7 and obs[:, 20].any() and obs[:, 18].any() and obs[:, 12].any() and obs[:, 6].any()
    assert (obs[:, ctf.ACTION_AT:].sum(axis=1) == len(nvec)).all()


def test_observe_batch_with_no_agents(ctf):
    nvec = ctf.ActionNvec("stage0")
    obs, phi = ctf.ObserveBatch(np.zeros(0, ctf.layout.BridgeView), Frame(ctf).match, np.zeros((0, len(nvec)), dtype=np.int64), nvec, 120.0)
    assert obs.shape == (0, ctf.ACTION_AT + sum(nvec)) and obs.dtype == np.float32 and phi.shape == (0,)


def test_observe_batch_of_one_is_observe(ctf):
    views, match, last = CoveringViews(ctf)
    nvec = ctf.ActionNvec("stage0")
    for i in range(len(views)):
        obs, phi = ctf.ObserveBatch(views[i:i + 1], match, last[i:i + 1], nvec, 120.0)
        one, one_phi = ctf.Observe(views[i], match, last[i], nvec, 120.0)
        assert np.array_equal(obs[0], one) and (one_phi is None) == bool(np.isnan(phi[0])) and (one_phi is None or one_phi == phi[0])
    obs, phi = ctf.Observe(None, match, last[0], nvec, 120.0)
    assert phi is None and obs[19] == pytest.approx(ctf.TimeLeft(match, 120.0)) and not obs[:19].any() and not obs[20:ctf.ACTION_AT].any()


def test_the_environment_observes_all_agents_in_one_batch(ctf):
    L = ctf.layout
    handles = (P0, P0 + 1, P0 + 2)
    flag = {"own": (HOME, (0.0, 0.0, 0.0), (0.0, 0.0, 0.0)), "enemy": (HOME, (100.0, 0.0, 0.0), (100.0, 0.0, 0.0))}

    def Tick(count, action_tick, **match):
        players = [{"Handle": h, "BipedHandle": B0 + i, "Team": RED, "LocalIndex": 0xFF, "IsAlive": 1, "HasBody": 1}
                   for i, h in enumerate(handles[:count])]
        views = [ViewRow(L, handle=h, feet=(5.0 + 10.0 * i, 3.0 * i, 0.0), yaw=0.4 * i, side=RED, **flag) for i, h in enumerate(handles[:count])]
        return FakeTick(L, players, [], [], [], action_tick=action_tick, views=views, match=match or None)

    env = ctf.CtfEnv(FakeBridge([Tick(3, 1), Tick(2, 2, RoundClockTicks=1800)]))
    nvec = env.nvec
    with mock.patch.object(ctf, "ObserveBatch", wraps=ctf.ObserveBatch) as batch:
        observations, _ = env.reset()
        after_reset = batch.call_count
        assert set(observations) == set(handles)
        for i, h in enumerate(handles):
            expected, _ = ctf.Observe(env.published.views[i], env.published.match, np.zeros(len(nvec), dtype=np.int64), nvec, env.config.max_seconds)
            assert np.array_equal(observations[h], expected)
        # P0 + 2 leaves the Players: no row, no View, an observation of no body.
        actions = {h: np.array([2, 1, 4 + i, 2, i % 4, 0, 1], dtype=np.int64) for i, h in enumerate(handles)}
        before_step = batch.call_count
        observations, rewards, _, _, _ = env.step(actions)
        after_step = batch.call_count
    assert (after_reset, after_step - before_step) == (1, 1)
    assert set(observations) == set(handles) and set(rewards) == set(handles)
    for i, h in enumerate(handles):
        view = env.published.views[i] if i < 2 else None
        expected, phi = ctf.Observe(view, env.published.match, actions[h], nvec, env.config.max_seconds)
        assert np.array_equal(observations[h], expected)
        assert (env.states[h].phi is None) == (phi is None)
    assert not observations[handles[2]][:19].any() and observations[handles[0]][19] == pytest.approx(0.75)
    assert env.states[handles[0]].side == RED and env.states[handles[2]].side == RED


# --- P.3 B (D374): the Views are read in place and validated; the guard of the sparse mode ---------------------------------------------


def test_the_environment_reads_without_copy_and_validates_every_observation(ctf):
    bridge = FakeBridge([Frame(ctf, action_tick=1), Frame(ctf, action_tick=2), Frame(ctf, action_tick=3)])
    env = ctf.CtfEnv(bridge)
    env.reset()
    assert bridge.checks == 1 and env.reread == 0
    env.step({P0: Nothing(ctf)})
    assert bridge.checks == 2 and env.reread == 0
    assert set(bridge.copies) == {False}


def test_a_tick_that_changes_under_the_observation_is_read_again_and_the_observation_is_the_new_tick(ctf):
    def Event(kind, sequence):
        return {"Sequence": sequence, "Kind": kind, "CausePlayer": P0, "EffectPlayer": NONE}
    first = Frame(ctf, action_tick=1)
    torn = Frame(ctf, action_tick=2, agent_at=(5.0, 0.0, 0.0), events=[Event(ctf.Event.FlagTaken, 1)])
    new = Frame(ctf, action_tick=3, agent_at=(40.0, 0.0, 0.0), game_tick=101, events=[Event(ctf.Event.FlagCaptured, 2)])
    last = Frame(ctf, action_tick=4, game_tick=102)
    env = ctf.CtfEnv(FakeBridge([first, torn, new, last]))
    env.reset()
    env.bridge.changed = 1
    observations, _, _, _, infos = env.step({P0: Nothing(ctf)})
    assert env.reread == 1 and env.published is new and env.bridge.checks == 3
    # The observation is the one of the new tick, byte for byte what a step on it gives.
    reference = ctf.CtfEnv(FakeBridge([first, new]))
    reference.reset()
    expected, _, _, _, _ = reference.step({P0: Nothing(ctf)})
    assert observations[P0].tobytes() == expected[P0].tobytes()
    # The rewards of this step come from the tick it was read as; the events of the new tick are not lost: they go to the next step.
    assert infos[P0]["events"] == ["FlagTaken"]
    _, _, _, _, infos = env.step({P0: Nothing(ctf)})
    assert infos[P0]["events"] == ["FlagCaptured"]
    assert env.reread == 1


def test_a_tick_that_keeps_changing_raises_instead_of_looping(ctf):
    env = ctf.CtfEnv(FakeBridge([Frame(ctf, action_tick=1), Frame(ctf, action_tick=2)] + [Frame(ctf, action_tick=3 + i) for i in range(100)]))
    env.reset()
    env.bridge.changed = 10 ** 6
    with pytest.raises(RuntimeError, match="changed under the observation"):
        env.step({P0: Nothing(ctf)})
    assert env.reread == ctf.MAX_REREADS


class RefusedSparse(FakeBridge):
    """Artemis keeps every tick: IsSparse never goes to 1."""
    def wait_control(self, *args, **kwargs):
        return "IsSparse" not in kwargs


def test_a_sparse_request_that_artemis_does_not_grant_raises_with_the_reason(ctf):
    bridge = RefusedSparse([Frame(ctf)])
    env = ctf.CtfEnv(bridge, ctf.CtfConfig(sparse=True))
    with pytest.raises(RuntimeError, match=r"IsSparse 0.*events.*tickdump.*every_tick"):
        env.take_control()
    assert env.is_sparse is False
    assert bridge.controls[-1]["sparse"] is False  # the instance goes back to the UI
    # Without the request there is nothing to check.
    off = ctf.CtfEnv(RefusedSparse([Frame(ctf)]))
    assert off.take_control() is True and off.is_sparse is False


def test_a_sparse_request_that_artemis_grants_goes_on(ctf):
    env = ctf.CtfEnv(FakeBridge([Frame(ctf)]), ctf.CtfConfig(sparse=True))
    assert env.take_control() is True and env.is_sparse is True


# --- P.3 E: the training crosses the change of match of a scenario ----------------------------------------------------------------

P1 = 0xEC700010


def OtherMatch(ctf, handle=P1, match_id=2, **kwargs):
    """A tick of the next match of the scenario: another match_id and the agent bot re-added with another handle."""
    frame = Frame(ctf, match_id=match_id, **kwargs)
    frame.players["Handle"][0] = handle
    return frame


class IdleBridge(FakeBridge):
    """A bridge with the phase Idle (the bench is in the lobby): a None in the list of ticks is a wait_tick that timed out."""
    def __init__(self, ticks, phase=None):
        super().__init__(ticks)
        self.phase = phase


def GameEvent(ctf, kind, cause=NONE, effect=NONE, sequence=1):
    return {"Sequence": sequence, "Kind": kind, "CausePlayer": cause, "EffectPlayer": effect}


def test_idle_shorter_than_the_gap_and_then_another_match_truncates_every_agent_and_reset_starts_the_new_one(ctf):
    taken = GameEvent(ctf, ctf.Event.FlagTaken, cause=P0)
    spawn = GameEvent(ctf, ctf.Event.Spawn, cause=P1, sequence=2)
    bridge = IdleBridge([Frame(ctf, action_tick=1), Frame(ctf, action_tick=2)] + [None] * 5
                        + [OtherMatch(ctf, action_tick=1, events=[taken, spawn]), OtherMatch(ctf, action_tick=2)],
                        phase=ctf.layout.BridgePhase.Idle)
    env = ctf.CtfEnv(bridge, ctf.CtfConfig(tick_timeout=0.001, reset_timeout=1.0, match_gap_timeout=1.0))
    env.reset()
    old_observation = env._observations[P0].copy()
    observations, rewards, terminated, truncated, infos = env.step({P0: Nothing(ctf)})
    assert not truncated[P0] and env.agents == [P0]
    observations, rewards, terminated, truncated, infos = env.step({P0: Nothing(ctf)})
    # Nothing of the new match enters the old step (its FlagTaken of another handle's world does not reward the old agent) and the
    # observation is the last one of the old agent.
    assert truncated == {P0: True} and terminated == {P0: False} and rewards == {P0: 0.0}
    assert infos[P0]["new_match"] is True and infos[P0]["events"] == []
    assert env.is_waiting_round and env.match_id == 1
    assert observations[P0].shape == old_observation.shape
    observations, infos = env.reset()
    assert list(observations) == [P1] and env.agents == [P1] and list(env.states) == [P1] and env.match_id == 2
    assert not env.is_waiting_round
    assert bridge.controls[-1]["agents"] == 1 << (P1 & 0xFFFF)


def test_idle_longer_than_the_gap_raises_with_the_time_waited(ctf):
    bridge = IdleBridge([Frame(ctf)] + [None] * 100, phase=ctf.layout.BridgePhase.Idle)
    env = ctf.CtfEnv(bridge, ctf.CtfConfig(tick_timeout=0.01, reset_timeout=0.05, match_gap_timeout=0.05))
    env.reset()
    with pytest.raises(RuntimeError, match=r"the match ended: Artemis has no running match after 0 s"):
        env.step({P0: Nothing(ctf)})
    # A gap longer than reset_timeout is not cut by it: Idle is counted by the gap alone.
    bridge = IdleBridge([Frame(ctf)] + [None] * 8 + [OtherMatch(ctf)], phase=ctf.layout.BridgePhase.Idle)
    env = ctf.CtfEnv(bridge, ctf.CtfConfig(tick_timeout=0.01, reset_timeout=0.05, match_gap_timeout=1.0))
    env.reset()
    _, _, _, truncated, _ = env.step({P0: Nothing(ctf)})
    assert truncated[P0]


def test_the_gap_error_names_the_seconds_of_the_gap(ctf):
    bridge = IdleBridge([Frame(ctf)] + [None] * 1000, phase=ctf.layout.BridgePhase.Idle)
    env = ctf.CtfEnv(bridge, ctf.CtfConfig(tick_timeout=0.5, reset_timeout=1.0, match_gap_timeout=2.0))
    env.reset()
    with pytest.raises(RuntimeError, match=r"after 2 s"):
        env.step({P0: Nothing(ctf)})
    assert bridge.ticks.count(None) == 1000 - 4


def test_a_dead_bridge_during_idle_keeps_the_closed_bridge_error(ctf):
    bridge = IdleBridge([Frame(ctf)] + [None] * 10, phase=ctf.layout.BridgePhase.Idle)
    env = ctf.CtfEnv(bridge, ctf.CtfConfig(tick_timeout=0.001, reset_timeout=1.0, match_gap_timeout=1.0))
    env.reset()
    bridge.is_alive = False
    with pytest.raises(RuntimeError, match="Artemis closed the bridge"):
        env.step({P0: Nothing(ctf)})


def test_control_is_not_asked_again_by_the_change_of_match_and_the_sparse_guard_stays_with_take_control(ctf):
    # Artemis keeps owner, mode, N, d and sparse of the client between matches (BridgeService::BeginMatch leaves the control alone), so
    # the change of match asks for the AgentMask of the new handles only: no owner, mode or sparse in that request.
    bridge = FakeBridge([Frame(ctf, action_tick=1), OtherMatch(ctf, action_tick=1), OtherMatch(ctf, action_tick=2)])
    env = ctf.CtfEnv(bridge, ctf.CtfConfig(sparse=True))
    assert env.take_control() is True
    env.reset()
    env.step({P0: Nothing(ctf)})
    env.reset()
    controls = bridge.controls
    assert sum(1 for c in controls if "owner" in c) == 1 and sum(1 for c in controls if c.get("sparse")) == 1
    assert [c["agents"] for c in controls if set(c) == {"agents"}] == [1 << (P0 & 0xFFFF), 1 << (P1 & 0xFFFF)]


def test_ppo_closes_the_rows_of_the_old_handles_at_a_change_of_match_and_gae_does_not_cross(ctf):
    pytest.importorskip("torch")
    import torch
    ppo_module = importlib.import_module("artemis.train.ppo")
    frames = [Frame(ctf, action_tick=1), Frame(ctf, action_tick=2), Frame(ctf, action_tick=3),
              OtherMatch(ctf, action_tick=1), OtherMatch(ctf, action_tick=2), OtherMatch(ctf, action_tick=3), OtherMatch(ctf, action_tick=4)]
    bridge = FakeBridge(frames)
    env = ctf.CtfEnv(bridge)
    config = ppo_module.PpoConfig(steps=5, minibatch=4)
    ppo = ppo_module.Ppo(env, bridge, env.obs_size, env.nvec, config, log=lambda line: None)
    batch = ppo.collect()
    # Rows in order: P0, P0, P0 (the third is the cut), then P1, P1 of the new match.
    assert list(batch["dones"]) == [0.0, 0.0, 1.0, 0.0, 0.0]
    assert list(ppo.observations) == [P1]
    # The advantage of the cut row is its own delta (nothing after it is mixed in) and the first rows of the new match do not see the old ones.
    values, rewards = batch["values"], batch["rewards"]
    assert batch["advantages"][2] == pytest.approx(rewards[2] - values[2])
    last = float(ppo.policy.value(torch.as_tensor(ppo.observations[P1])[None])[0])
    delta = rewards[4] + config.gamma * last - values[4]
    assert batch["advantages"][4] == pytest.approx(delta, abs=1e-5)


def test_every_take_of_the_agent_is_in_the_events_but_only_the_first_is_paid(ctf):
    def Taken(sequence):
        return {"Sequence": sequence, "Kind": ctf.Event.FlagTaken, "CausePlayer": P0, "EffectPlayer": NONE}
    start = Frame(ctf, action_tick=1)
    twice = Frame(ctf, flags=(HOME, CARRIED), flag_at=(5.0, 0.0, 0.5), carrying=True, action_tick=2, events=[Taken(1), Taken(2)])
    env = ctf.CtfEnv(FakeBridge([start, twice]), ctf.CtfConfig(shaping=0.0))
    env.reset()
    nothing = np.zeros(len(ctf.ActionNvec("stage0")), dtype=np.int64)
    _, rewards, _, _, infos = env.step({P0: nothing})
    assert infos[P0]["events"] == ["FlagTaken", "FlagTaken"]
    assert rewards[P0] == pytest.approx(env.config.take_reward)


def test_policy_greedy_is_the_argmax_of_each_branch_and_act_is_unchanged(ctf):
    torch = pytest.importorskip("torch")
    ppo_module = importlib.import_module("artemis.train.ppo")
    policy = ppo_module.Policy(4, [3, 2], 8)
    with torch.no_grad():
        policy.body = torch.nn.Identity()
        policy.actor = torch.nn.Identity()
    logits = torch.tensor([[0.1, 2.0, -1.0, 5.0, 1.0], [9.0, 0.0, 0.0, 0.0, 0.5]])
    assert policy.greedy(logits).tolist() == [[1, 0], [0, 1]]
    actions, log_prob, entropy, _ = ppo_module.Policy(4, [3, 2], 8).act(torch.zeros(5, 4))
    assert actions.shape == (5, 2) and log_prob.shape == (5,) and entropy.shape == (5,)


def test_env_check_loads_a_saved_policy_and_a_size_mismatch_says_both_sizes(ctf, tmp_path):
    torch = pytest.importorskip("torch")
    ppo_module = importlib.import_module("artemis.train.ppo")
    check = EnvCheck(ctf)
    nvec = [3, 2, 4]
    saved = ppo_module.Policy(12, nvec, 256)
    path = str(tmp_path / "policy.pt")
    torch.save(saved.state_dict(), path)
    loaded = check.LoadPolicy(path, 12, nvec, 256)
    assert all(torch.equal(a, b) for a, b in zip(saved.state_dict().values(), loaded.state_dict().values()))
    with pytest.raises(ValueError) as error:
        check.LoadPolicy(path, 15, nvec, 256)
    assert "observation 12" in str(error.value) and "observation 15" in str(error.value)
    with pytest.raises(ValueError) as error:
        check.LoadPolicy(path, 12, [3, 2, 4, 2], 256)
    assert "actions 9" in str(error.value) and "actions 11" in str(error.value)


def test_env_check_assigns_policy_and_policy2_by_team_not_by_order(ctf):
    check = EnvCheck(ctf)
    teams = {30: 1, 10: 0, 20: 1}
    assert check.AssignPolicies(teams, "a", "b") == {30: "b", 10: "a", 20: "b"}
    assert check.AssignPolicies(teams, "a", None) == {30: "a", 10: "a", 20: "a"}
    assert check.AssignPolicies({5: 1}, "a", "b") == {5: "a"}


def test_env_check_summary_aggregates_the_rows_of_the_rounds(ctf):
    check = EnvCheck(ctf)
    teams, policies = {1: 0, 2: 1}, {1: "u0.pt", 2: "u2300.pt"}

    def Row(ending, game, wall, captures, winner):
        return {"ending": ending, "game_seconds": game, "wall_seconds": wall, "captures": captures, "winner": winner,
                "teams": teams, "policies": policies}
    rows = [Row("terminated", 30.0, 31.0, {1: 0, 2: 1}, 1), Row("terminated", 50.0, 52.0, {1: 0, 2: 1}, 1),
            Row("truncated", 90.0, 91.0, {1: 0, 2: 0}, None), Row("terminated", 40.0, 41.0, {1: 1, 2: 0}, 0)]
    summary = check.Summarize(rows)
    assert (summary["rounds"], summary["terminated"], summary["truncated"], summary["draws"]) == (4, 3, 1, 1)
    assert summary["game_mean"] == pytest.approx(52.5) and summary["game_p50"] == pytest.approx(45.0)
    assert summary["wall_p50"] == pytest.approx(46.5)
    assert summary["captures"] == {1: 1, 2: 2}
    assert summary["wins_by_team"] == {1: 2, 0: 1}
    assert summary["wins_by_policy"] == {"u2300.pt": 2, "u0.pt": 1}
    assert summary["policies"] == policies
    assert check.Summarize([])["rounds"] == 0
    assert any("u2300.pt" in line for line in check.SummaryLines(summary))
    finals = {1: np.array([0.0] * 20 + [0.5, 1.0]), 2: np.array([0.0] * 20 + [1.0, 0.5])}
    assert check.RoundWinner(finals, teams) == 0
    assert check.RoundWinner({1: np.array([0.0] * 20 + [1.0, 1.0])}, teams) is None


def test_env_check_defaults_match_train_stage0(ctf):
    config = importlib.import_module("artemis.env").CtfConfig()
    assert (config.period, config.latency, config.turn_limit, config.profile) == (5, 5, 0.1, "stage0")
    source = open(os.path.join(AGENT, "scripts", "env_check.py"), encoding="utf-8").read()
    assert 'default=5, help="ticks between decisions (N)"' in source and 'default=5, help="ticks from a decision' in source
