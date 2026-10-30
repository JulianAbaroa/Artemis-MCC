"""
test_ctf.py
The environment of stage 0 (Agent/artemis/env/ctf.py) over synthetic frames, with no game: the bridge is a fake that hands out the
ticks of a list. The real Bridge needs Windows shared memory, so a stand-in module takes its place for the import. B4h H.5: the side
of an agent comes from the OwnerTeam of the stands, and the flag state from the rows of the tick.
"""
import importlib
import math
import os
import sys
import types
from dataclasses import dataclass

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
    def __init__(self, layout, players, entities, objectives, stands, events=(), game_tick=100, match_id=1, action_tick=1):
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
        self.match = Rows("BridgeMatch", [{"GameTick": game_tick}])[0]
        self.match_id, self.action_tick = match_id, action_tick


class FakeBridge:
    def __init__(self, ticks):
        self.ticks, self.is_alive, self.written = list(ticks), True, []

    def set_control(self, **kwargs):
        pass

    def wait_control(self, *args, **kwargs):
        return True

    def wait_tick(self, timeout):
        return self.ticks.pop(0) if self.ticks else None

    def new_events(self, published):
        return published.events

    def write_actions(self, tick, actions):
        self.written.append((tick, actions))

    def end_round(self, wait=True):
        pass


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
    yield module
    for name in [n for n in sys.modules if n == "artemis" or n.startswith("artemis.")]:
        sys.modules.pop(name, None)


def Frame(ctf, owners=(RED, BLUE), flags=(HOME, HOME), flag_at=None, carrying=False, events=(), game_tick=100, agent_at=(5.0, 0.0, 0.0),
          action_tick=1, match_id=1):
    """One tick of a round on a map with a red base at x = 0 and a blue base at x = 100. owners: OwnerTeam of the stands of BaseTeam 0
    and 1 in this round. flags: state of the flag of BaseTeam 0 and 1. flag_at: position of the flag of BaseTeam 1 when it is not
    home. The agent is the player P0 of team RED, aiming at +x."""
    L = ctf.layout
    stand_at = {RED: (0.0, 0.0, 0.0), BLUE: (100.0, 0.0, 0.0)}
    flag_pos = [stand_at[RED], flag_at or stand_at[BLUE]]
    entities = [{"Handle": B0, "WorldOrigin": agent_at, "Team": RED},
                {"Handle": STAND_RED, "WorldOrigin": stand_at[RED]}, {"Handle": STAND_BLUE, "WorldOrigin": stand_at[BLUE]},
                {"Handle": FLAG_RED, "WorldOrigin": flag_pos[0]}, {"Handle": FLAG_BLUE, "WorldOrigin": flag_pos[1]}]
    players = [{"Handle": P0, "BipedHandle": B0, "Team": RED, "LocalIndex": 0xFF, "IsAlive": 1, "HasBody": 1, "Aim": (1.0, 0.0, 0.0),
                "Shield": 1.0, "Objective": FLAG_BLUE if carrying else NONE}]
    objectives = [{"Handle": FLAG_RED, "BaseTeam": RED, "OwnerTeam": owners[0], "State": flags[0], "CarrierPlayer": NONE, "CarrierBiped": NONE},
                  {"Handle": FLAG_BLUE, "BaseTeam": BLUE, "OwnerTeam": owners[1], "State": flags[1],
                   "CarrierPlayer": P0 if flags[1] == CARRIED else NONE, "CarrierBiped": B0 if flags[1] == CARRIED else NONE}]
    stands = [{"Handle": STAND_RED, "BaseTeam": RED, "OwnerTeam": owners[0]}, {"Handle": STAND_BLUE, "BaseTeam": BLUE, "OwnerTeam": owners[1]}]
    return FakeTick(L, players, entities, objectives, stands, events, game_tick, match_id=match_id, action_tick=action_tick)


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
