"""
ctf.py
Environment of stage 0 of the curriculum (D115, D146, Diseno/14): every agent alone against the enemy flag, take it and bring it to
its own flag stand. Parallel multi-agent API in the style of PettingZoo without the package: dicts keyed by agent (the full player
handle, D40). One step = one decision of the bridge (N ticks, Lockstep). The policy sees only its View and the Match (F4.9, D317):
ObserveBatch is a pure function of the Views of all the agents (the rows of published.views, in world coordinates), the Match and the last
actions, one numpy pass for the N agents of a tick (P.3); Observe is its one-row wrapper; the critic (Q68), when it is separated, reads Players, Objectives and Entities.

The View has Head.Side, the BaseTeam of the stand the team of the agent owns in this round (so the swap of sides between rounds needs
no deduction here), Flags[0], the flag of the own team (State and the origin of its stand), and Flags[1], a rival flag (State and
Position, the carrier's when carried). A flag row that is all zero is a side without flag. What is not observation (who the agents are,
whether the round started, the end of the match, the events and the rewards) is read from the omniscient tick: it is the management of
the environment, not information of the agent. The rewards read the FlagTaken and FlagCaptured events Artemis generates, whose cause is
the carrier. The clock and the score of the round come from the Match of the tick (P.6, D255): the environment measures no time of its own.
The observation is [22 of state | 7 of View.Interaction.Kind (one-hot, D372) | one-hot of the last action] (the entries are listed in ObserveBatch). An episode that crosses to
another match (a new match_id or a lower GameTick with no RoundEnd / GameOver) ends truncated (D372, P.3).
"""
import logging
import math
import time
from dataclasses import dataclass

import numpy as np

from ..bridge import CONTROL_STATES, CONTROLS, Action, Bridge, BridgeMode, BridgeOwner, PackControls, qpc
from ..bridge_layout import NOT_LOCAL, BridgeInteractionKind, BridgePhase, BridgeSetupResult, BridgeView
from ..match_setup import MatchSetup, is_relative_roster, resolve_roster
from ..semantic import Event, FlagState

METERS = 50.0

# Input profiles (D365, Diseno/22 5.3): the controls the policy may use. The others stay on "nothing" and are not in the action space.
PROFILES = {"keyboard": CONTROLS, "stage0": ("Jump", "Crouch", "Use")}
# MultiDiscrete branches: forward, left, yaw delta, pitch delta, then one of the four states of CONTROL_STATES per control of the profile.
MOVE_NVEC = (3, 3, 9, 5)
THROTTLE = (-1.0, 0.0, 1.0)
YAW_STEPS = (-1.0, -0.5, -0.25, -0.1, 0.0, 0.1, 0.25, 0.5, 1.0)
PITCH_STEPS = (-0.5, -0.2, 0.0, 0.2, 0.5)
# Entries of the state before the interaction and the one-hot of the last action: 19 = time left, 20 and 21 = score left of the own and the enemy team.
STATE_SIZE = 22
# Entries of the state: 6 carries the flag, 7-9 the vector to the enemy flag, 14-16 the vector to the own stand (the ones Seek of env_check reads).
CARRIES_AT = 6
ENEMY_FLAG_AT = slice(7, 9)
OWN_STAND_AT = slice(14, 16)
# D372: after the state, one entry per BridgeInteractionKind (in the order of the generated enum: None, GrabWeapon, GrabArmorAbility,
# TakeHealthStation, EnterVehicle, Hijack, GrabObjective) = the one-hot of View.Interaction.Kind. Detail is not read.
INTERACTION_KINDS = tuple(int(kind) for kind in BridgeInteractionKind)
INTERACTION_SIZE = len(INTERACTION_KINDS)
INTERACTION_AT = STATE_SIZE
GRAB_OBJECTIVE_AT = INTERACTION_AT + INTERACTION_KINDS.index(int(BridgeInteractionKind.GrabObjective))
# Entries before the one-hot of the last action (state + interaction): where the last action starts.
ACTION_AT = STATE_SIZE + INTERACTION_SIZE
# Common.Team.Type: Head.Side of a View whose team owns no stand in this round.
NEUTRAL_TEAM = 8
# Phases of a decision that CtfConfig.timing measures (ns): blocked in wait_tick, copy of read_tick, new_events, the observations, write_actions.
PHASES = ("wait", "copy", "events", "observe", "answer")


def ActionNvec(profile):
    """Sizes of the MultiDiscrete branches of a profile."""
    return MOVE_NVEC + (len(CONTROL_STATES),) * len(PROFILES[profile])


def MakeAction(handle, branches, profile, per_decision):
    """The bridge Action of one decision from the branches of the MultiDiscrete. per_decision: radians of a full yaw / pitch step."""
    states = dict(zip(PROFILES[profile], (int(b) for b in branches[len(MOVE_NVEC):])))
    level, edge = PackControls(states)
    return Action(handle=handle, forward=THROTTLE[branches[0]], left=THROTTLE[branches[1]],
                  yaw_delta=YAW_STEPS[branches[2]] * per_decision, pitch_delta=PITCH_STEPS[branches[3]] * per_decision,
                  level=level, edge=edge)


@dataclass
class CtfConfig:
    period: int = 4
    latency: int = 4
    scale: float = 0.0
    turn_limit: float = 0.1
    profile: str = "stage0"
    # Asks Artemis for its pipeline only on decision ticks (Q101). Off by default: the checks of env_check read every tick.
    sparse: bool = False
    max_seconds: float = 120.0
    shaping: float = 0.1
    take_reward: float = 0.2
    capture_reward: float = 1.0
    death_reward: float = -0.2
    tick_timeout: float = 5.0
    reset_timeout: float = 120.0
    # Measures the phases of each decision and the slack of the Lockstep (CtfEnv.timings). Off by default: no cost outside the measurement.
    timing: bool = False


def Vec(v):
    return np.array([float(v["X"]), float(v["Y"]), float(v["Z"])], dtype=np.float32)


class AgentState:
    """What the environment keeps of one agent across the steps of an episode."""

    def __init__(self, handle, nvec):
        self.handle = handle
        self.index = handle & 0xFFFF
        self.last_action = np.zeros(len(nvec), dtype=np.int64)
        self.phi = None
        self.is_taken_rewarded = False
        # Head.Side (BaseTeam of the base the agent plays from) of the View of the last observation, None before the first.
        self.side = None


class CtfEnv:
    """Stage 0 environment over one Artemis instance. reset() waits for the agents to spawn in a round; step() answers the current
    tick with the actions and returns what the next one says. Episode = one round: it ends with RoundEnd or GameOver
    (terminated, also when the time limit of the round runs out: the time left is in the observation) or is cut with
    Bridge.end_round when the round clock of the Match reaches max_seconds (truncated), the cap of the environment. It is also truncated when
    the match changes under it (match_id or a lower GameTick with no RoundEnd / GameOver in the step)."""

    def __init__(self, bridge: Bridge, config: CtfConfig = None, setups=None):
        """setups: an object with next_setup() (a TrainingList). A reset() that is not given a setup of its own asks for the next one."""
        self.bridge = bridge
        self.config = config or CtfConfig()
        if self.config.profile not in PROFILES:
            raise ValueError(f"unknown input profile {self.config.profile!r} (have: {', '.join(PROFILES)})")
        self.nvec = ActionNvec(self.config.profile)
        self.obs_size = ACTION_AT + sum(self.nvec)
        self.setups = setups
        self.agents = []
        self.states = {}
        self.published = None
        self.match_id = None
        self.is_waiting_round = False
        self.is_cut = False
        # (match_id, GameTick) of the last tick the episode saw: a step that finds another match or an earlier tick crossed a match end.
        self._reference = None
        self.events = []
        self._rows = None
        # With config.timing: the ns of each phase of every decision (PHASES) and the ms from the publication of a tick to its answer.
        self.timings = {phase: [] for phase in PHASES}
        self.slack_ms = []

    # --- Control ----------------------------------------------------------------------------------------------------------------

    def take_control(self, agents_mask=0) -> bool:
        c = self.config
        self.bridge.set_control(owner=BridgeOwner.Python, mode=BridgeMode.Lockstep, period=c.period, latency=c.latency,
                                scale=c.scale, agents=agents_mask, turn_limit=c.turn_limit,
                                sparse=c.sparse)
        return self.bridge.wait_control(Owner=BridgeOwner.Python, IsClientConnected=1)

    def release(self):
        self.bridge.set_control(owner=BridgeOwner.UI, scale=0.0, agents=0, turn_limit=0.0, sparse=False)

    # --- Ticks -----------------------------------------------------------------------------------------------------------------

    def _next_tick(self, actions=None, measured=False):
        """Answers the current tick (Lockstep needs an answer for every tick) and waits for the next one.
        measured: records the phases of this decision (only with config.timing)."""
        measured = measured and self.config.timing
        if self.published is not None:
            started = time.perf_counter_ns()
            self.bridge.write_actions(self.published.action_tick, actions or {})
            if measured:
                answered = qpc()
                self.timings["answer"].append(time.perf_counter_ns() - started)
                frequency = getattr(self.bridge, "qpc_frequency", 1)
                self.slack_ms.append((answered - getattr(self.published, "publish_qpc", answered)) * 1000.0 / frequency)
        published = None
        waited = 0.0
        started = time.perf_counter_ns()
        while published is None:
            published = self.bridge.wait_tick(self.config.tick_timeout)
            if published is not None:
                break
            if not self.bridge.is_alive:
                raise RuntimeError("Artemis closed the bridge")
            waited += self.config.tick_timeout
            if getattr(self.bridge, "phase", None) == BridgePhase.Idle:
                raise RuntimeError(self._ended("the match ended: Artemis has no running match"))
            if waited >= self.config.reset_timeout:
                raise RuntimeError(self._ended(f"no tick for {waited:.0f} s"))
        self.published = published
        self._rows = None
        if measured:
            copy = int(getattr(self.bridge, "last_copy_ns", 0))
            self.timings["copy"].append(copy)
            self.timings["wait"].append(max(0, time.perf_counter_ns() - started - copy))
            started = time.perf_counter_ns()
        self.events = list(self.bridge.new_events(published))
        if measured:
            self.timings["events"].append(time.perf_counter_ns() - started)
        return published

    def _ended(self, what):
        """The message of a match that stopped giving ticks: what happened and, when the last tick has a CTF team with no players, why the
        gametype most likely ended it (Reach ends a match whose team is left empty)."""
        empty = []
        if self.published is not None:
            teams = {int(p["Team"]) for p in self.published.players}
            empty = [team for team in (0, 1) if team not in teams]
        if empty:
            return f"{what}: match ended by the gametype: a team was left empty? (no players on team {empty[0]})"
        return what

    def _local_team(self):
        """Team of the local player 0 in the next tick (the passive rival, D287), to resolve the roster of a training list with."""
        published = self._next_tick()
        for player in published.players:
            if int(player["LocalIndex"]) == 0:
                return int(player["Team"])
        raise RuntimeError("no local player 0 in the tick: a roster relative to it cannot be resolved")

    def _row_of(self, handle):
        """Row of Players (and of Views) of a player handle in the current tick, None if it is not there."""
        if self._rows is None:
            self._rows = {int(h): i for i, h in enumerate(self.published.players["Handle"].tolist())}
        return self._rows.get(handle)

    def _view_of(self, handle):
        """The View of the agent in the current tick, or None when it has no row or no View (a row past ViewCount)."""
        row = self._row_of(handle)
        views = self.published.views
        return views[row] if row is not None and row < len(views) else None

    def _players(self):
        return {int(p["Handle"]): p for p in self.published.players}

    @staticmethod
    def _is_playing(player):
        return player is not None and int(player["IsAlive"]) and int(player["HasBody"])

    # --- Episode ----------------------------------------------------------------------------------------------------------------

    def reset(self, setup=None, preset=None, map=None, roster=None):
        """Waits until every bot of the match is alive in a running round and returns {agent: observation}.
        setup: a dict (keys preset, map, roster) or a MatchSetup, asked for in one request. preset, map and roster ask for one field each
        and take the place of the same field of setup. A field that is None is left as it is.
        preset: name of a gametype preset to play from the next round (Data/Presets), "" for the base values of the game variant.
        map: name of the match config whose map variant to play from the next round (Q98), "" for the one the match loaded with.
        roster: the bots to play with (a dict of the "roster" of a bench scenario, or the bots of each team from team 0).
        The setup is requested and the round is ended once, since it is written between rounds. A setup that cannot be applied (Rejected)
        raises RuntimeError with the reason: training with other settings than the ones asked would go unnoticed. So does a map variant
        left for the next load (a match is up and keeps its map) and one whose round has no 2 flag stands (a Megalo CTF finds its flags
        by label, so a map variant without them leaves a CTF without flags). A partial result (the roster did not fit after the preset
        or the map went in) is logged and reported in the info. With no setup, preset, map or roster of its own, a reset takes the next setup
        of the setups of the environment (one episode is one round, so one setup per round), if it has any."""
        if setup is None and preset is None and map is None and roster is None and self.setups is not None:
            setup = self.setups.next_setup()
        asked = MatchSetup.of(setup)
        asked = MatchSetup(preset if preset is not None else asked.preset, map if map is not None else asked.map,
                           roster if roster is not None else asked.roster)
        waited = 0.0
        deadline = time.monotonic() + self.config.reset_timeout
        is_asked = asked.preset is not None or asked.map is not None or asked.roster is not None
        if is_asked:
            if is_relative_roster(asked.roster):
                # The roster of a list is relative to the local player 0, which sits on a team the engine decides: ask the tick right before.
                asked = MatchSetup(asked.preset, asked.map, resolve_roster(asked.roster, self._local_team()))
            status = self.bridge.request_setup(preset=asked.preset, map=asked.map, roster=asked.roster)
            if status is None:
                raise RuntimeError(f"Artemis did not take the setup request {asked}")
            if status.result == BridgeSetupResult.Rejected:
                raise RuntimeError(f"setup {asked} rejected: {status.reason or 'it cannot be applied'}")
            if status.result == BridgeSetupResult.AtLoad and asked.map is not None:
                # A match is up: AtLoad means the request went to the next start and this one trains on the wrong map.
                raise RuntimeError(f"map variant {asked.map!r} was left for the next match load: the current match keeps its map")
            if status.result == BridgeSetupResult.Pending:
                self.bridge.end_round(wait=False)
                self.is_waiting_round = True
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
            if time.monotonic() > deadline:
                raise TimeoutError(self._ended(f"no round with every agent alive after {self.config.reset_timeout:.0f} s"))
        info = {}
        if is_asked:
            final = self.bridge.wait_setup()
            if final.result == BridgeSetupResult.Rejected:
                raise RuntimeError(f"setup {asked} rejected between rounds: {final.reason or 'it cannot be applied'}")
            if final.result == BridgeSetupResult.Pending:
                logging.getLogger(__name__).warning("setup %s is still pending after the wait: the round started without it", asked)
            if final.reason:
                logging.getLogger(__name__).warning("setup %s: %s", asked, final.reason)
            if asked.map is not None and len(self.published.stands) < 2:
                raise RuntimeError(f"map variant {asked.map!r} has no CTF for this gametype: the round has {len(self.published.stands)} flag stands, 2 needed")
            result = getattr(final.result, "name", final.result)
            if asked.preset is not None:
                info["preset"] = {"name": asked.preset, "result": result}
            if asked.map is not None:
                info["map"] = {"name": asked.map, "result": result}
            if asked.roster is not None:
                info["roster"] = {"value": asked.roster, "result": result}
            info["setup"] = {"result": result, "round": final.round, "reason": final.reason}
        self.agents = [int(p["Handle"]) for p in players]
        self.states = {handle: AgentState(handle, self.nvec) for handle in self.agents}
        mask = 0
        for handle in self.agents:
            mask |= 1 << (handle & 0xFFFF)
        self.bridge.set_control(agents=mask)
        self.is_cut = False
        self._wait_views()
        self._reference = self._clock()
        observations = {}
        for handle, (observations[handle], phi) in self._observe_all().items():
            self.states[handle].phi = phi
        return observations, {handle: {key: dict(value) for key, value in info.items()} for handle in self.agents}

    def _clock(self):
        return self.published.match_id, int(self.published.match["GameTick"])

    def _wait_views(self):
        """The Views are computed only for the agents of the mask, so the tick in which the mask is set has none: advances ticks (answering
        with no actions) until the View of every agent is computed, at most reset_timeout seconds."""
        deadline = time.monotonic() + self.config.reset_timeout
        while True:
            missing = [handle for handle in self.agents if not self._is_computed(self._view_of(handle))]
            if not missing:
                return
            if time.monotonic() > deadline:
                raise TimeoutError(self._ended(f"the Views of the agents were not computed after {self.config.reset_timeout:.0f} s "
                                               f"(missing: {', '.join(f'{h:#010x}' for h in missing)}): is the agent mask applied?"))
            self._next_tick()

    @staticmethod
    def _is_computed(view):
        return view is not None and int(view["Head"]["IsComputed"]) != 0

    def step(self, actions):
        """actions: {agent: array of the MultiDiscrete branches}. Returns observations, rewards, terminated, truncated, infos."""
        c = self.config
        players = self._players()
        bridge_actions = {}
        # A look delta above turn_limit * d (the window is min(N, d) ticks) is cut by Artemis and lost: a full step stays under it.
        per_decision = min(c.period, c.latency) * c.turn_limit if c.turn_limit > 0 else math.pi / 4
        for handle, branches in actions.items():
            state = self.states[handle]
            player = players.get(handle)
            branches = np.asarray(branches, dtype=np.int64)
            state.last_action = branches
            if player is None:
                continue
            bridge_actions[state.index] = MakeAction(handle, branches, c.profile, per_decision)
        self._next_tick(bridge_actions, measured=True)

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
        # D372 (P.3): a match that ended under the episode without RoundEnd / GameOver (another match_id, or the clock went back) cuts it
        # truncated; reset() then waits for the round of the new match as after any end.
        reference, self._reference = self._reference, self._clock()
        is_new_match = (not is_round_over and reference is not None
                        and (self._reference[0] != reference[0] or self._reference[1] < reference[1]))
        if is_new_match:
            self.is_waiting_round = True

        is_timeout = not is_round_over and not is_new_match and not self.is_cut and int(self.published.match["RoundClockTicks"]) >= c.max_seconds * 60
        if is_timeout:
            # The RoundEnd of the cut comes a few ticks later: reset() waits for the next round past it.
            self.bridge.end_round(wait=False)
            self.is_cut = True
            self.is_waiting_round = True

        observations = {}
        started = time.perf_counter_ns() if c.timing else 0
        for handle, (observations[handle], phi) in self._observe_all().items():
            state = self.states[handle]
            # note: phi' - phi without the discount telescopes exactly over the episode. With gamma it would add (1 - gamma) |phi|
            # per step, a reward for staying alive as large as the capture on a long map.
            if phi is not None and state.phi is not None:
                rewards[handle] += c.shaping * (phi - state.phi)
            state.phi = phi
        if c.timing:
            self.timings["observe"].append(time.perf_counter_ns() - started)
        terminated = {handle: is_round_over for handle in self.agents}
        truncated = {handle: is_timeout or is_new_match for handle in self.agents}
        return observations, rewards, terminated, truncated, infos

    # --- Observation ------------------------------------------------------------------------------------------------------------

    def _agent_views(self):
        """The Views of the agents of the episode as one array of len(self.agents) rows, in the order of self.agents: one indexing of
        published.views by the row of each agent. An agent with no row or no View (a row past ViewCount) gets a row of zeros (not computed)."""
        views = self.published.views
        rows = np.array([-1 if row is None else row for row in map(self._row_of, self.agents)], dtype=np.int64)
        is_there = (rows >= 0) & (rows < len(views))
        out = np.zeros(len(rows), dtype=views.dtype)
        out[is_there] = views[rows[is_there]]
        return out

    def _observe_all(self):
        """{handle: (observation, potential)} of every agent of the episode in ONE ObserveBatch over their Views (potential None while it has
        no body, side or both flags). Also records Head.Side in state.side of the agents whose View is computed. The observations are rows
        of a single float32 array [N, obs_size] (views, not copies): the consumers read them and never write on them (ppo.py stacks them
        into a new array, env_check copies what it keeps); whoever must modify one copies it."""
        views = self._agent_views()
        count = len(self.agents)
        last = np.array([self.states[handle].last_action for handle in self.agents], dtype=np.int64).reshape(count, len(self.nvec))
        obs, phi = ObserveBatch(views, self.published.match, last, self.nvec, self.config.max_seconds)
        computed = (views["Head"]["IsComputed"] != 0).tolist()
        sides = views["Head"]["Side"].tolist()
        potentials = phi.tolist()
        result = {}
        for i, handle in enumerate(self.agents):
            if computed[i]:
                self.states[handle].side = sides[i]
            result[handle] = (obs[i], None if math.isnan(potentials[i]) else potentials[i])
        return result


def TimeLeft(match, max_seconds):
    """Fraction of the episode left, from the Match: the ticks left of the round (RoundTicksRemaining, -1 without a time limit)
    or of the cap of the environment (max_seconds over RoundClockTicks), whichever ends first, over the horizon of the same one.
    It depends only on the Match: the same for every agent of a tick."""
    cap = max_seconds * 60
    left, horizon = cap - int(match["RoundClockTicks"]), cap
    remaining, limit = int(match["RoundTicksRemaining"]), int(match["RoundTimeLimitTicks"])
    if remaining >= 0:
        left = min(left, remaining)
        if limit > 0:
            horizon = min(horizon, limit)
    return min(1.0, max(0.0, left / horizon)) if horizon > 0 else 0.0


def ScoreLeft(match, teams):
    """(own, enemy): float64 arrays, one entry per team of teams (an array of Head.Team), with the fraction of ScoreToWin that the team and
    its enemy still need to win the round (TeamRoundScores of the Match); 0 when the variant has no score to win. The enemy is the other
    team with the most score (the one that needs the least), which in a CTF of two teams is the other one. A team outside the scores has
    0 of its own and the best of all as its enemy. One table per team of the Match (at most 8), then one indexing."""
    to_win = int(match["ScoreToWin"])
    if to_win <= 0:
        return np.zeros(len(teams)), np.zeros(len(teams))
    scores = [int(score) for score in match["TeamRoundScores"]]
    top = sorted(scores, reverse=True) + [-1, -1]
    # Best score of the others of each team (-1: none), and a last entry for a team outside the scores (the best of all).
    others = [top[1] if score == top[0] else top[0] for score in scores] + [top[0]]
    own_table = np.array([max(0, to_win - score) / to_win for score in scores] + [0.0])
    enemy_table = np.array([max(0, to_win - best) / to_win if best >= 0 else 0.0 for best in others])
    index = np.minimum(teams, len(scores))
    return own_table[index], enemy_table[index]


def ByteAt(*path, element=0):
    """Byte offset, in the row of a BridgeView, of the field at the path of names; element: which of the entries of the array (Flags) the
    first name is."""
    dtype, offset = BridgeView, 0
    for i, name in enumerate(path):
        sub, at = dtype.fields[name][:2]
        offset += at
        if sub.subdtype is not None:
            sub = sub.subdtype[0]
        if i == 0:
            offset += element * sub.itemsize
        dtype = sub
    return offset


def FloatAt(*path, element=0):
    """Index, in the row of a BridgeView read as float32, of the float field at the path (the layout aligns them to 4 bytes: checked)."""
    offset = ByteAt(*path, element=element)
    assert offset % 4 == 0 and BridgeView.itemsize % 4 == 0, path
    return offset // 4


# What ObserveBatch reads of a View, by its place in the row as bytes (BYTE) or as float32 (the others): one slice of a 2-D array each, not
# a field access (and a stack for the vectors of X, Y, Z) per entry, which costs more than the arithmetic over 16 agents.
FEET_AT, VELOCITY_AT = FloatAt("Self", "Feet"), FloatAt("Self", "LinearVelocity")
YAW_AT, PITCH_AT = FloatAt("Self", "FrameYaw"), FloatAt("Self", "AimPitch")
VITALITY_AT, SHIELD_AT = FloatAt("Self", "Vitality"), FloatAt("Self", "Shield")
OWN_STAND_FLOATS, ENEMY_FLAG_FLOATS = FloatAt("Flags", "StandPosition", element=0), FloatAt("Flags", "Position", element=1)
IS_COMPUTED_BYTE, HAS_BODY_BYTE = ByteAt("Head", "IsComputed"), ByteAt("Head", "HasBody")
TEAM_BYTE, SIDE_BYTE, KIND_BYTE = ByteAt("Head", "Team"), ByteAt("Head", "Side"), ByteAt("Interaction", "Kind")
CARRIES_BYTE = ByteAt("Self", "CarriesFlag")
OWN_HAS_FLAG_BYTE, OWN_STATE_BYTE = ByteAt("Flags", "HasFlag", element=0), ByteAt("Flags", "State", element=0)
ENEMY_HAS_FLAG_BYTE, ENEMY_STATE_BYTE = ByteAt("Flags", "HasFlag", element=1), ByteAt("Flags", "State", element=1)


def Rotate(vectors, cos, sin):
    """The world vectors [N, 3] in the frame of each agent (the yaw is cos / sin, one per row): x forward, y to the left."""
    out = np.empty_like(vectors)
    out[:, 0] = cos * vectors[:, 0] - sin * vectors[:, 1]
    out[:, 1] = sin * vectors[:, 0] + cos * vectors[:, 1]
    out[:, 2] = vectors[:, 2]
    return out


# Interaction.Kind (u1) -> column of the one-hot of the interaction, -1 for a Kind outside the enum.
INTERACTION_COLUMN = np.full(256, -1, dtype=np.int64)
INTERACTION_COLUMN[list(INTERACTION_KINDS)] = np.arange(INTERACTION_SIZE)


def ObserveBatch(views, match, last_actions, nvec, max_seconds):
    """(observations, potentials) of N agents from their Views (world frame, rotated to the frame of each agent here) and the Match, in one
    numpy pass with no loop over the agents. Pure: it reads nothing else. views: a structured array with the dtype BridgeView (exactly: not one rebuilt by np.resize or concatenate), one row per agent; a
    row that is all zero is an agent with no View (not computed). last_actions: int64 [N, len(nvec)]. Returns obs float32
    [N, ACTION_AT + sum(nvec)] and phi float64 [N], NaN where the agent has no shaping potential (without body, side or both flags).
    The potential exists only with the enemy flag and the own stand.
    Entries (the list lives here): 0-2 velocity, 3 pitch, 4 vitality, 5 shield, 6 carries the flag, 7-9 and 10 vector and distance to the
    enemy flag, 11 / 12 it is home / dropped, 13 carries, 14-16 and 17 vector and distance to the own stand, 18 the own flag is home,
    19 time left, 20 and 21 score left of the own and the enemy team, 22-28 the one-hot of View.Interaction.Kind (D372; order of
    BridgeInteractionKind: None, GrabWeapon, GrabArmorAbility, TakeHealthStation, EnterVehicle, Hijack, GrabObjective; all zero without
    View or body, none for a Kind outside the enum; Detail is not read), then the one-hot of the last action of each branch of nvec.
    Not computed View: only 19 and the last action. Computed without body: also 20-21. No side (Head.Side is NEUTRAL_TEAM): no flags
    (7-18 stay zero). Enemy flag without HasFlag: 7-13 zero. Own side without stand: 14-18 zero. A NaN vitality or shield is 0."""
    count = len(views)
    nvec = np.asarray(nvec, dtype=np.int64)
    obs = np.zeros((count, ACTION_AT + int(nvec.sum())), dtype=np.float32)
    phi = np.full(count, np.nan)
    if count == 0:
        return obs, phi
    # One-hot of the last action: the column of each branch is its offset (cumulative sum of nvec) plus the chosen state.
    offsets = ACTION_AT + np.cumsum(nvec) - nvec
    last = np.asarray(last_actions, dtype=np.int64).reshape(count, len(nvec))
    obs[np.arange(count)[:, None], offsets[None, :] + last] = 1.0
    obs[:, 19] = TimeLeft(match, max_seconds)
    # The rows as bytes: every field is a column (the 1-byte ones here, the floats below through a float32 view of the same memory).
    raw = np.ascontiguousarray(views).view(np.uint8).reshape(count, -1)
    is_computed = raw[:, IS_COMPUTED_BYTE] != 0
    computed = np.flatnonzero(is_computed)
    obs[computed, 20:22] = np.stack(ScoreLeft(match, raw[computed, TEAM_BYTE]), axis=1)
    rows = np.flatnonzero(is_computed & (raw[:, HAS_BODY_BYTE] != 0))
    if len(rows) == 0:
        return obs, phi
    # From here on the rows with a body, computed for all of them and masked where an entry does not apply (no per-row branches).
    body = raw[rows]
    floats = body.view(np.float32)
    block = np.zeros((len(rows), ACTION_AT), dtype=np.float32)
    block[:, 19:22] = obs[rows, 19:22]
    # An interaction Kind outside the enum sets no entry (no exception, no log: it would be one per tick).
    column = INTERACTION_COLUMN[body[:, KIND_BYTE]]
    block[np.arange(len(rows)), INTERACTION_AT + np.maximum(column, 0)] = column >= 0
    position = floats[:, FEET_AT:FEET_AT + 3]
    yaw = floats[:, YAW_AT].astype(np.float64)
    cos, sin = np.cos(-yaw).astype(np.float32), np.sin(-yaw).astype(np.float32)
    is_carrying = body[:, CARRIES_BYTE] != 0
    block[:, 0:3] = Rotate(floats[:, VELOCITY_AT:VELOCITY_AT + 3], cos, sin) / 10.0
    block[:, 3] = floats[:, PITCH_AT]
    vitality, shield = floats[:, VITALITY_AT], floats[:, SHIELD_AT]
    block[:, 4] = np.where(np.isnan(vitality), 0.0, vitality)
    block[:, 5] = np.where(np.isnan(shield), 0.0, shield)
    block[:, 6] = is_carrying
    # A team with no side (it owns no stand) sees no flags: 7-18 stay zero. HasFlag says whether the flag exists in this tick (State 0 is
    # Home, a row without flag looks Home): the flag entries stay zero without it. The stand is there whenever the side has one, even
    # while the engine recreates the flag (D370). Without both the enemy flag and the own stand there is no potential.
    is_sided = body[:, SIDE_BYTE] != NEUTRAL_TEAM
    stand, flag = floats[:, OWN_STAND_FLOATS:OWN_STAND_FLOATS + 3], floats[:, ENEMY_FLAG_FLOATS:ENEMY_FLAG_FLOATS + 3]
    to_flag, to_stand = flag - position, stand - position
    flag_distance, stand_distance = np.linalg.norm(to_flag, axis=1), np.linalg.norm(to_stand, axis=1)
    has_flag = is_sided & (body[:, ENEMY_HAS_FLAG_BYTE] != 0)
    has_stand = is_sided & (stand != 0.0).any(axis=1)
    state = body[:, ENEMY_STATE_BYTE]
    columns = (flag_distance / METERS, state == int(FlagState.Home), state == int(FlagState.Dropped), is_carrying)
    block[:, 7:10] = np.where(has_flag[:, None], Rotate(to_flag, cos, sin) / METERS, 0.0)
    for at, values in enumerate(columns, start=10):
        block[:, at] = np.where(has_flag, values, 0.0)
    block[:, 14:17] = np.where(has_stand[:, None], Rotate(to_stand, cos, sin) / METERS, 0.0)
    block[:, 17] = np.where(has_stand, stand_distance / METERS, 0.0)
    block[:, 18] = has_stand & (body[:, OWN_HAS_FLAG_BYTE] != 0) & (body[:, OWN_STATE_BYTE] == int(FlagState.Home))
    obs[rows, :ACTION_AT] = block
    # Potential: with the enemy flag and the own stand; to the stand when carrying, else to the flag and from it to the stand.
    through_flag = flag_distance + np.linalg.norm(stand - flag, axis=1)
    phi[rows] = np.where(has_flag & has_stand, -np.where(is_carrying, stand_distance, through_flag) / METERS, np.nan)
    return obs, phi


def Observe(view, match, last_action, nvec, max_seconds):
    """(observation, potential) of ONE agent: ObserveBatch over a single row (its entries are listed there). view is a row of BridgeView, or
    None for an agent with no View (a row of zeros). The potential is None where ObserveBatch gives NaN. For the tests, the checks and the
    benchmark; the environment calls ObserveBatch once for all its agents."""
    views = np.zeros(1, dtype=BridgeView) if view is None else np.asarray(view).reshape(1)
    obs, phi = ObserveBatch(views, match, np.asarray(last_action, dtype=np.int64).reshape(1, -1), nvec, max_seconds)
    return obs[0], (None if np.isnan(phi[0]) else float(phi[0]))
