"""
ctf.py
Environment of stage 0 of the curriculum (D115, D146, Diseno/14): every agent alone against the enemy flag, take it and bring it to
its own flag stand. Parallel multi-agent API in the style of PettingZoo without the package: dicts keyed by agent (the full player
handle, D40). One step = one decision of the bridge (N ticks, Lockstep). The policy sees only its View and the Match (F4.9, D317):
Observe is a pure function of the View of the agent (published.views[row of the agent], in world coordinates), the Match and the last
action; the critic (Q68), when it is separated, reads Players, Objectives and Entities.

The View has Head.Side, the BaseTeam of the stand the team of the agent owns in this round (so the swap of sides between rounds needs
no deduction here), Flags[0], the flag of the own team (State and the origin of its stand), and Flags[1], a rival flag (State and
Position, the carrier's when carried). A flag row that is all zero is a side without flag. What is not observation (who the agents are,
whether the round started, the end of the match, the events and the rewards) is read from the omniscient tick: it is the management of
the environment, not information of the agent. The rewards read the FlagTaken and FlagCaptured events Artemis generates, whose cause is
the carrier. The clock and the score of the round come from the Match of the tick (P.6, D255): the environment measures no time of its own.
"""
import logging
import math
import time
from dataclasses import dataclass

import numpy as np

from ..bridge import CONTROL_STATES, CONTROLS, Action, Bridge, BridgeMode, BridgeOwner, PackControls, qpc
from ..bridge_layout import NOT_LOCAL, BridgePhase, BridgeSetupResult
from ..match_setup import MatchSetup, is_relative_roster, resolve_roster
from ..semantic import Event, FlagState

METERS = 50.0

# Input profiles (D365, Diseno/22 5.3): the controls the policy may use. The others stay on "nothing" and are not in the action space.
PROFILES = {"keyboard": CONTROLS, "stage0": ("Jump", "Crouch")}
# MultiDiscrete branches: forward, left, yaw delta, pitch delta, then one of the four states of CONTROL_STATES per control of the profile.
MOVE_NVEC = (3, 3, 9, 5)
THROTTLE = (-1.0, 0.0, 1.0)
YAW_STEPS = (-1.0, -0.5, -0.25, -0.1, 0.0, 0.1, 0.25, 0.5, 1.0)
PITCH_STEPS = (-0.5, -0.2, 0.0, 0.2, 0.5)
# Entries of the state before the one-hot of the last action: 19 = time left, 20 and 21 = score left of the own and the enemy team.
STATE_SIZE = 22
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
    Bridge.end_round when the round clock of the Match reaches max_seconds (truncated), the cap of the environment."""

    def __init__(self, bridge: Bridge, config: CtfConfig = None, setups=None):
        """setups: an object with next_setup() (a TrainingList). A reset() that is not given a setup of its own asks for the next one."""
        self.bridge = bridge
        self.config = config or CtfConfig()
        if self.config.profile not in PROFILES:
            raise ValueError(f"unknown input profile {self.config.profile!r} (have: {', '.join(PROFILES)})")
        self.nvec = ActionNvec(self.config.profile)
        self.obs_size = STATE_SIZE + sum(self.nvec)
        self.setups = setups
        self.agents = []
        self.states = {}
        self.published = None
        self.match_id = None
        self.is_waiting_round = False
        self.is_cut = False
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
        observations = {}
        for handle in self.agents:
            observations[handle], phi = self._observe(handle)
            self.states[handle].phi = phi
        return observations, {handle: {key: dict(value) for key, value in info.items()} for handle in self.agents}

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

        is_timeout = not is_round_over and not self.is_cut and int(self.published.match["RoundClockTicks"]) >= c.max_seconds * 60
        if is_timeout:
            # The RoundEnd of the cut comes a few ticks later: reset() waits for the next round past it.
            self.bridge.end_round(wait=False)
            self.is_cut = True
            self.is_waiting_round = True

        observations = {}
        started = time.perf_counter_ns() if c.timing else 0
        for handle in self.agents:
            observations[handle], phi = self._observe(handle)
            state = self.states[handle]
            # note: phi' - phi without the discount telescopes exactly over the episode. With gamma it would add (1 - gamma) |phi|
            # per step, a reward for staying alive as large as the capture on a long map.
            if phi is not None and state.phi is not None:
                rewards[handle] += c.shaping * (phi - state.phi)
            state.phi = phi
        if c.timing:
            self.timings["observe"].append(time.perf_counter_ns() - started)
        terminated = {handle: is_round_over for handle in self.agents}
        truncated = {handle: is_timeout for handle in self.agents}
        return observations, rewards, terminated, truncated, infos

    # --- Observation ------------------------------------------------------------------------------------------------------------

    def _observe(self, handle):
        """Observation vector of one agent and the shaping potential of its state (None while it has no body): Observe over its View."""
        view = self._view_of(handle)
        state = self.states[handle]
        if self._is_computed(view):
            state.side = int(view["Head"]["Side"])
        return Observe(view, self.published.match, state.last_action, self.nvec, self.config.max_seconds)


def TimeLeft(match, max_seconds):
    """Fraction of the episode left, from the Match: the ticks left of the round (RoundTicksRemaining, -1 without a time limit)
    or of the cap of the environment (max_seconds over RoundClockTicks), whichever ends first, over the horizon of the same one."""
    cap = max_seconds * 60
    left, horizon = cap - int(match["RoundClockTicks"]), cap
    remaining, limit = int(match["RoundTicksRemaining"]), int(match["RoundTimeLimitTicks"])
    if remaining >= 0:
        left = min(left, remaining)
        if limit > 0:
            horizon = min(horizon, limit)
    return min(1.0, max(0.0, left / horizon)) if horizon > 0 else 0.0


def ScoreLeft(match, team):
    """Fraction of ScoreToWin that the team and its enemy still need to win the round (TeamRoundScores of the Match); 0 when the
    variant has no score to win. The enemy is the other team with the most score (the one that needs the least), which in a CTF of two
    teams is the other one."""
    to_win = int(match["ScoreToWin"])
    if to_win <= 0:
        return 0.0, 0.0
    scores = [int(score) for score in match["TeamRoundScores"]]

    def Left(score):
        return max(0, to_win - score) / to_win
    others = [score for t, score in enumerate(scores) if t != team]
    return Left(scores[team]) if 0 <= team < len(scores) else 0.0, Left(max(others)) if others else 0.0


def IsFlag(flag):
    """Whether a row of View.Flags is a flag: a row with no flag is all zero (Views CONTRACT), and State 0 is Home, so the position of
    the flag or of its stand tells."""
    return any(float(flag[field][axis]) != 0.0 for field in ("Position", "StandPosition") for axis in "XYZ")


def Observe(view, match, last_action, nvec, max_seconds):
    """(observation, potential) of one agent from its View (world frame, rotated to the frame of the agent here) and the Match. Pure: it
    reads nothing else. view is None, or a row not computed, for an agent with no View. The potential is None without body, without side
    or without both flags (no shaping). Entries: 0-2 velocity, 3 pitch, 4 vitality, 5 shield, 6 carries the flag, 7-9 and 10 vector and
    distance to the enemy flag, 11 / 12 it is home / dropped, 13 carries, 14-16 and 17 vector and distance to the own stand, 18 the own
    flag is home, 19 time left, 20 and 21 score left of the own and the enemy team, then the one-hot of the last action."""
    obs = np.zeros(STATE_SIZE + sum(nvec), dtype=np.float32)
    offset = STATE_SIZE
    for branch, n in zip(last_action, nvec):
        obs[offset + int(branch)] = 1.0
        offset += n
    obs[19] = TimeLeft(match, max_seconds)
    if view is None or not int(view["Head"]["IsComputed"]):
        return obs, None
    head, me = view["Head"], view["Self"]
    obs[20:22] = ScoreLeft(match, int(head["Team"]))
    if not int(head["HasBody"]):
        return obs, None
    position = Vec(me["Feet"])
    yaw, pitch = float(me["FrameYaw"]), float(me["AimPitch"])
    cos, sin = math.cos(-yaw), math.sin(-yaw)

    def Local(v):
        return np.array([cos * v[0] - sin * v[1], sin * v[0] + cos * v[1], v[2]], dtype=np.float32)

    is_carrying = int(me["CarriesFlag"]) != 0
    obs[0:3] = Local(Vec(me["LinearVelocity"])) / 10.0
    obs[3] = pitch
    vitality, shield = float(me["Vitality"]), float(me["Shield"])
    obs[4] = 0.0 if math.isnan(vitality) else vitality
    obs[5] = 0.0 if math.isnan(shield) else shield
    obs[6] = float(is_carrying)
    flags = view["Flags"]
    # A team with no side (it owns no stand) sees no flags: obs[7:19] stay zero. A row of Flags that is all zero is a flag the View does not
    # have (a flag the engine is recreating, or a side without one): its entries stay zero and, without both the enemy flag and the own
    # stand, there is no potential.
    if int(head["Side"]) == NEUTRAL_TEAM:
        return obs, None
    own, enemy = flags[0], flags[1]
    has_stand = IsFlag(own)
    stand_position = Vec(own["StandPosition"])
    phi = None
    if IsFlag(enemy):
        flag_position = Vec(enemy["Position"])
        to_flag = flag_position - position
        obs[7:10] = Local(to_flag) / METERS
        obs[10] = np.linalg.norm(to_flag) / METERS
        flag_state = int(enemy["State"])
        obs[11] = float(flag_state == FlagState.Home)
        obs[12] = float(flag_state == FlagState.Dropped)
        obs[13] = float(is_carrying)
        if has_stand:
            if is_carrying:
                phi = -np.linalg.norm(stand_position - position) / METERS
            else:
                phi = -(np.linalg.norm(to_flag) + np.linalg.norm(stand_position - flag_position)) / METERS
    if has_stand:
        to_stand = stand_position - position
        obs[14:17] = Local(to_stand) / METERS
        obs[17] = np.linalg.norm(to_stand) / METERS
        obs[18] = float(int(own["State"]) == FlagState.Home)
    return obs, (float(phi) if phi is not None else None)
