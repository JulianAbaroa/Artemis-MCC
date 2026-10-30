"""
ctf.py
Environment of stage 0 of the curriculum (D115, D146, Diseno/14): every agent alone against the enemy flag, take it and bring it to
its own flag stand. Parallel multi-agent API in the style of PettingZoo without the package: dicts keyed by agent (the full player
handle, D40). One step = one decision of the bridge (N ticks, Lockstep). The observation is computed here from the omniscient tick
(Full mode); it moves to the View of each agent with B5.

The flags and the stands come from the tick as Artemis computes them (B4h): each flag row has its BaseTeam, the OwnerTeam that plays
its side in this round, its State (Home, Carried, Dropped) and its carrier; each stand row has its BaseTeam and OwnerTeam. The side
of an agent is the BaseTeam of the stand its team owns in this round, so the swap of sides between rounds needs no deduction here.
The rewards read the FlagTaken and FlagCaptured events Artemis generates, whose cause is the carrier.
"""
import math
from dataclasses import dataclass

import numpy as np

from ..bridge import Action, Bridge, BridgeMode, BridgeOwner
from ..bridge_layout import NO_HANDLE, NOT_LOCAL
from ..semantic import Event, FlagState

TWO_PI = 2 * math.pi
METERS = 50.0
PITCH_LIMIT = 1.4
CROUCH_BIT = 1 << 0
JUMP_BIT = 1 << 1

# MultiDiscrete branches: forward, left, yaw delta, pitch delta, jump, crouch.
ACTION_NVEC = (3, 3, 9, 5, 2, 2)
THROTTLE = (-1.0, 0.0, 1.0)
YAW_STEPS = (-1.0, -0.5, -0.25, -0.1, 0.0, 0.1, 0.25, 0.5, 1.0)
PITCH_STEPS = (-0.5, -0.2, 0.0, 0.2, 0.5)
OBS_SIZE = 20 + sum(ACTION_NVEC)


@dataclass
class CtfConfig:
    period: int = 4
    latency: int = 2
    scale: float = 0.0
    turn_limit: float = 0.1
    max_seconds: float = 120.0
    shaping: float = 0.1
    take_reward: float = 0.2
    capture_reward: float = 1.0
    death_reward: float = -0.2
    tick_timeout: float = 5.0
    reset_timeout: float = 120.0


def Wrap(angle):
    return angle % TWO_PI


def AimAngles(player):
    aim = player["Aim"]
    x, y, z = float(aim["X"]), float(aim["Y"]), float(aim["Z"])
    return Wrap(math.atan2(y, x)), math.atan2(z, math.hypot(x, y))


def Vec(v):
    return np.array([float(v["X"]), float(v["Y"]), float(v["Z"])], dtype=np.float32)


class AgentState:
    """What the environment keeps of one agent across the steps of an episode."""

    def __init__(self, handle):
        self.handle = handle
        self.index = handle & 0xFFFF
        self.target = None
        self.last_action = np.zeros(len(ACTION_NVEC), dtype=np.int64)
        self.phi = None
        self.is_taken_rewarded = False
        # BaseTeam of the base the agent plays from, as of the last observation (see CtfEnv._side_of).
        self.side = None


class CtfEnv:
    """Stage 0 environment over one Artemis instance. reset() waits for the agents to spawn in a round; step() answers the current
    tick with the actions and returns what the next one says. Episode = one round: it ends with RoundEnd or GameOver
    (terminated) or is cut after max_seconds of game time with Bridge.end_round (truncated)."""

    def __init__(self, bridge: Bridge, config: CtfConfig = None):
        self.bridge = bridge
        self.config = config or CtfConfig()
        self.agents = []
        self.states = {}
        self.published = None
        self.match_id = None
        self.start_tick = 0
        self.is_waiting_round = False
        self.is_cut = False
        self.events = []
        self.flags = self.stands = None

    # --- Control ----------------------------------------------------------------------------------------------------------------

    def take_control(self, agents_mask=0) -> bool:
        c = self.config
        self.bridge.set_control(owner=BridgeOwner.Python, mode=BridgeMode.Lockstep, period=c.period, latency=c.latency,
                                scale=c.scale, agents=agents_mask, turn_limit=c.turn_limit)
        return self.bridge.wait_control(Owner=BridgeOwner.Python, IsClientConnected=1)

    def release(self):
        self.bridge.set_control(owner=BridgeOwner.UI, scale=0.0, agents=0, turn_limit=0.0)

    # --- Ticks -----------------------------------------------------------------------------------------------------------------

    def _next_tick(self, actions=None):
        """Answers the current tick (Lockstep needs an answer for every tick) and waits for the next one."""
        if self.published is not None:
            self.bridge.write_actions(self.published.action_tick, actions or {})
        published = None
        while published is None:
            published = self.bridge.wait_tick(self.config.tick_timeout)
            if published is None and not self.bridge.is_alive:
                raise RuntimeError("Artemis closed the bridge")
        self.published = published
        self.events = list(self.bridge.new_events(published))
        # Flags and stands of the round, as Artemis computes them (rows of the tick, no search over the entities).
        self.flags = published.objectives
        self.stands = published.stands
        return published

    def _entity(self, handle):
        rows = self.published.entities[self.published.entities["Handle"] == handle]
        return rows[0] if len(rows) else None

    def _players(self):
        return {int(p["Handle"]): p for p in self.published.players}

    @staticmethod
    def _is_playing(player):
        return player is not None and int(player["IsAlive"]) and int(player["HasBody"])

    # --- Episode ----------------------------------------------------------------------------------------------------------------

    def reset(self):
        """Waits until every bot of the match is alive in a running round and returns {agent: observation}."""
        waited = 0.0
        while True:
            published = self._next_tick()
            waited += self.config.period / 60.0
            is_new_match = published.match_id != self.match_id
            if is_new_match:
                self.match_id = published.match_id
                self.is_waiting_round = False
            for event in self.events:
                # note: RoundStart comes ~5 s before the spawn of the round, while the bodies of the last one may still stand.
                if int(event["Kind"]) == Event.Spawn:
                    self.is_waiting_round = False
                elif int(event["Kind"]) in (Event.RoundEnd, Event.GameOver):
                    self.is_waiting_round = True
            players = [p for p in published.players if int(p["LocalIndex"]) == NOT_LOCAL]
            if players and not self.is_waiting_round and all(self._is_playing(p) for p in players):
                break
            if waited > self.config.reset_timeout * 60:
                raise TimeoutError("no round with every agent alive")
        self.agents = [int(p["Handle"]) for p in players]
        self.states = {handle: AgentState(handle) for handle in self.agents}
        for player in players:
            self.states[int(player["Handle"])].side = self._side_of(int(player["Team"]))
        mask = 0
        for handle in self.agents:
            mask |= 1 << (handle & 0xFFFF)
        self.bridge.set_control(agents=mask)
        self.start_tick = int(published.match["GameTick"])
        self.is_cut = False
        observations = {}
        for handle in self.agents:
            observations[handle], phi = self._observe(handle)
            self.states[handle].phi = phi
        return observations, {handle: {} for handle in self.agents}

    def step(self, actions):
        """actions: {agent: array of the MultiDiscrete branches}. Returns observations, rewards, terminated, truncated, infos."""
        c = self.config
        players = self._players()
        bridge_actions = {}
        for handle, branches in actions.items():
            state = self.states[handle]
            player = players.get(handle)
            branches = np.asarray(branches, dtype=np.int64)
            state.last_action = branches
            if player is None:
                continue
            if state.target is None or not self._is_playing(player):
                state.target = AimAngles(player)
            per_decision = c.period * c.turn_limit if c.turn_limit > 0 else math.pi / 4
            yaw = Wrap(state.target[0] + YAW_STEPS[branches[2]] * per_decision)
            pitch = float(np.clip(state.target[1] + PITCH_STEPS[branches[3]] * per_decision, -PITCH_LIMIT, PITCH_LIMIT))
            state.target = (yaw, pitch)
            buttons = (JUMP_BIT if branches[4] else 0) | (CROUCH_BIT if branches[5] else 0)
            bridge_actions[state.index] = Action(handle=handle, buttons=buttons, yaw=yaw, pitch=pitch,
                                                 forward=THROTTLE[branches[0]], left=THROTTLE[branches[1]])
        self._next_tick(bridge_actions)

        rewards = {handle: 0.0 for handle in self.agents}
        infos = {handle: {"events": []} for handle in self.agents}
        is_round_over = False
        for event in self.events:
            kind = int(event["Kind"])
            cause, effect = int(event["CausePlayer"]), int(event["EffectPlayer"])
            if kind in (Event.RoundEnd, Event.GameOver):
                is_round_over = True
            for handle in self.agents:
                state = self.states[handle]
                if kind == Event.FlagCaptured and cause == handle:
                    rewards[handle] += c.capture_reward
                elif kind == Event.FlagTaken and cause == handle and not state.is_taken_rewarded:
                    rewards[handle] += c.take_reward
                    state.is_taken_rewarded = True
                elif kind == Event.Death and effect == handle:
                    rewards[handle] += c.death_reward
                else:
                    continue
                infos[handle]["events"].append(Event(kind).name)
        if is_round_over:
            self.is_waiting_round = True

        elapsed = (int(self.published.match["GameTick"]) - self.start_tick) / 60.0
        is_timeout = not is_round_over and not self.is_cut and elapsed >= c.max_seconds
        if is_timeout:
            # The RoundEnd of the cut comes a few ticks later: reset() waits for the next round past it.
            self.bridge.end_round(wait=False)
            self.is_cut = True
            self.is_waiting_round = True

        observations = {}
        for handle in self.agents:
            observations[handle], phi = self._observe(handle)
            state = self.states[handle]
            # note: phi' - phi without the discount telescopes exactly over the episode. With gamma it would add (1 - gamma) |phi|
            # per step, a reward for staying alive as large as the capture on a long map.
            if phi is not None and state.phi is not None:
                rewards[handle] += c.shaping * (phi - state.phi)
            state.phi = phi
        terminated = {handle: is_round_over for handle in self.agents}
        truncated = {handle: is_timeout for handle in self.agents}
        return observations, rewards, terminated, truncated, infos

    def _side_of(self, team):
        """BaseTeam of the stand that the team owns in this round: the base its agents play from, or None if the team owns no stand.
        note: The flags and the stands keep the BaseTeam of their placement in the map variant, and the rounds of Reach CTF switch the
        sides: the blue bot plays from the stand of BaseTeam 1 in one round and of BaseTeam 0 in the next. Artemis gives the OwnerTeam
        of every stand for the round (the team mapping of the match), so the side is read, not deduced. A flag belongs to the agent
        when its BaseTeam is the side."""
        owned = self.stands[self.stands["OwnerTeam"] == team]
        return int(owned["BaseTeam"][0]) if len(owned) else None

    # --- Observation ------------------------------------------------------------------------------------------------------------

    def _observe(self, handle):
        """Observation vector of one agent and the shaping potential of its state (None while it has no body)."""
        c = self.config
        obs = np.zeros(OBS_SIZE, dtype=np.float32)
        player = self._players().get(handle)
        state = self.states[handle]
        offset = 20
        for branch, n in zip(state.last_action, ACTION_NVEC):
            obs[offset + int(branch)] = 1.0
            offset += n
        elapsed = (int(self.published.match["GameTick"]) - self.start_tick) / 60.0
        obs[19] = max(0.0, 1.0 - elapsed / c.max_seconds)
        if not self._is_playing(player):
            return obs, None
        body = self._entity(int(player["BipedHandle"]))
        if body is None:
            return obs, None
        side = self._side_of(int(player["Team"]))
        state.side = side
        position = Vec(body["WorldOrigin"])
        yaw, pitch = AimAngles(player)
        cos, sin = math.cos(-yaw), math.sin(-yaw)

        def Local(v):
            return np.array([cos * v[0] - sin * v[1], sin * v[0] + cos * v[1], v[2]], dtype=np.float32)

        def PositionOf(handle):
            entity = self._entity(int(handle))
            return Vec(entity["WorldOrigin"]) if entity is not None else None

        enemy_flag = own_flag = own_stand = None
        if side is not None:
            for flag in self.flags:
                if int(flag["BaseTeam"]) == side:
                    own_flag = flag
                else:
                    enemy_flag = flag
            for stand in self.stands:
                if int(stand["BaseTeam"]) == side:
                    own_stand = stand

        is_carrying = int(player["Objective"]) != NO_HANDLE
        obs[0:3] = Local(Vec(body["LinearVelocity"])) / 10.0
        obs[3] = pitch
        obs[4] = 1.0
        obs[5] = float(player["Shield"])
        obs[6] = float(is_carrying)
        phi = None
        stand_position = PositionOf(own_stand["Handle"]) if own_stand is not None else None
        flag_position = PositionOf(enemy_flag["Handle"]) if enemy_flag is not None else None
        if flag_position is not None:
            to_flag = flag_position - position
            obs[7:10] = Local(to_flag) / METERS
            obs[10] = np.linalg.norm(to_flag) / METERS
            flag_state = int(enemy_flag["State"])
            obs[11] = float(flag_state == FlagState.Home)
            obs[12] = float(flag_state == FlagState.Dropped)
            obs[13] = float(is_carrying)
            if stand_position is not None:
                if is_carrying:
                    phi = -np.linalg.norm(stand_position - position) / METERS
                else:
                    phi = -(np.linalg.norm(to_flag) + np.linalg.norm(stand_position - flag_position)) / METERS
        if stand_position is not None:
            to_stand = stand_position - position
            obs[14:17] = Local(to_stand) / METERS
            obs[17] = np.linalg.norm(to_stand) / METERS
        if own_flag is not None:
            obs[18] = float(int(own_flag["State"]) == FlagState.Home)
        return obs, (float(phi) if phi is not None else None)
