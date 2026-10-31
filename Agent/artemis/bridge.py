"""
Client of the Artemis bridge (D135-D138): opens the shared memory of one MCC instance, reads the tick of each decision tick and
writes the action of each player Python controls. Windows only (named shared memory and events of the same session).

    from artemis.bridge import Bridge, discover
    bridge = Bridge(discover()[0]["pid"])
    bridge.set_control(owner=BridgeOwner.Python, mode=BridgeMode.Lockstep, agents=mask)
    tick = bridge.wait_tick()
    bridge.write_actions(tick.action_tick, {index: Action(handle=..., yaw=..., forward=1.0)})

The layout comes from bridge_layout.py, generated from Data/Bridge/Bridge.xml: a region with another LayoutHash is refused.
"""
import contextlib
import ctypes
import glob
import json
import mmap
import os
import threading
import time
from ctypes import wintypes
from dataclasses import dataclass

import numpy as np

from . import bridge_layout as L
from .bridge_layout import BridgeMode, BridgeOwner, BridgePhase, BridgeRequest
from .semantic import RoundState

_kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
_kernel32.OpenEventW.restype = wintypes.HANDLE
_kernel32.OpenEventW.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.LPCWSTR]
_kernel32.SetEvent.argtypes = [wintypes.HANDLE]
_kernel32.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
_kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
_kernel32.OpenProcess.restype = wintypes.HANDLE
_kernel32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
_kernel32.GetExitCodeProcess.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]

_SYNCHRONIZE = 0x00100000
_EVENT_MODIFY_STATE = 0x0002
_PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
_STILL_ACTIVE = 259

# How often the client heartbeat is raised (Artemis drops a client after 2 s without it).
HEARTBEAT_INTERVAL = 0.2

# Copies of a tick tried before giving up on it (each failed copy means Artemis was writing that buffer).
READ_RETRIES = 8


class BridgeError(Exception):
    """The region does not exist, is not an Artemis bridge, or has another layout."""


def qpc() -> int:
    """QueryPerformanceCounter, the clock of PublishQpc."""
    value = ctypes.c_int64()
    _kernel32.QueryPerformanceCounter(ctypes.byref(value))
    return value.value


def _is_alive(pid: int) -> bool:
    handle = _kernel32.OpenProcess(_PROCESS_QUERY_LIMITED_INFORMATION, False, pid)
    if not handle:
        return False
    try:
        code = wintypes.DWORD()
        return bool(_kernel32.GetExitCodeProcess(handle, ctypes.byref(code))) and code.value == _STILL_ACTIVE
    finally:
        _kernel32.CloseHandle(handle)


def discover() -> list:
    """Open regions (D136): the discovery files of %LOCALAPPDATA%\\Artemis\\Bridge whose MCC process is alive, newest first."""
    folder = os.path.join(os.environ.get("LOCALAPPDATA", ""), "Artemis", L.DISCOVERY_FOLDER)
    found = []
    for path in sorted(glob.glob(os.path.join(folder, "*.json")), key=os.path.getmtime, reverse=True):
        try:
            with open(path, encoding="utf-8") as handle:
                entry = json.load(handle)
        except (OSError, ValueError):
            continue
        if _is_alive(int(entry.get("pid", 0))):
            found.append(entry)
    return found


@dataclass
class Action:
    """Action of one player for one decision (D109). Yaw and pitch are absolute radians, the throttle goes from -1 to 1."""
    handle: int
    buttons: int = 0
    yaw: float = 0.0
    pitch: float = 0.0
    forward: float = 0.0
    left: float = 0.0
    lanes: int = 0
    requests: BridgeRequest = BridgeRequest(0)


class PublishedTick:
    """A copy of one published tick: the header and the rows in use."""

    def __init__(self, data: np.ndarray, received_qpc: int):
        self.data = data
        self.header = data["Header"]
        self.players = data["Players"][: int(self.header["PlayerCount"])]
        self.entities = data["Entities"][: int(self.header["EntityCount"])]
        self.match = data["Match"]
        # Flags and stands of a capture game, sorted by BaseTeam then Handle. The engine recreates a flag on reset, return and
        # capture, so its Handle changes and BaseTeam is what identifies it.
        self.objectives = data["Objectives"][: int(self.header["ObjectiveCount"])]
        self.stands = data["Stands"][: int(self.header["StandCount"])]
        # Live weapons (in a hand, on the floor or mounted) and live projectiles, sorted by Handle. The pose and the team are in the
        # entity of the same Handle. ChargeFraction and Energy are NaN when the weapon has no charge or no energy.
        self.weapons = data["Weapons"][: int(self.header["WeaponCount"])]
        self.projectiles = data["Projectiles"][: int(self.header["ProjectileCount"])]
        # Live explosives (fusion coils, land mines...) with the radius and damage of their explosion, sorted by Handle (D259).
        self.explosives = data["Explosives"][: int(self.header["ExplosiveCount"])]
        # The newest events of the match up to this tick, oldest first. Bridge.new_events keeps only the unseen ones (D141).
        self.events = data["Events"][: int(self.header["EventCount"])]
        self.last_event_sequence = int(self.header["LastEventSequence"])
        self.action_tick = int(self.header["ActionTick"])
        self.match_id = int(self.header["MatchId"])
        self.received_qpc = received_qpc


    @property
    def round_index(self) -> int:
        """Round of the match, from 0."""
        return int(self.match["RoundIndex"])

    @property
    def round_state(self) -> RoundState:
        return RoundState(int(self.match["RoundState"]))

    @property
    def team_round_scores(self) -> np.ndarray:
        """Score of each team in the current round, indexed by team."""
        return self.match["TeamRoundScores"]

    @property
    def team_total_scores(self) -> np.ndarray:
        """Score of each team over the match, indexed by team."""
        return self.match["TeamTotalScores"]


class Bridge:
    """One open region. Raises BridgeError when it is not a live Artemis bridge with this layout."""

    def __init__(self, pid: int):
        self.pid = pid
        self.name = L.REGION_PREFIX + str(pid)
        # note: mmap with a tagname opens the region if Artemis created it, or creates an empty one: Magic tells them apart.
        self._mm = mmap.mmap(-1, L.REGION_SIZE, tagname=self.name)
        self.region = np.ndarray((), dtype=L.BridgeRegion, buffer=self._mm)
        self.artemis = self.region["Control"]["Artemis"]
        self.client = self.region["Control"]["Client"]
        self.buffers = self.region["Ticks"]
        self.actions = self.region["Actions"]
        self._action_sequence = ctypes.c_uint64.from_buffer(self._mm, L.OFFSETS["Control.Client.ActionSequence"])
        self._heartbeat = ctypes.c_uint64.from_buffer(self._mm, L.OFFSETS["Control.Client.ClientHeartbeat"])
        self._events = []
        self._stop = threading.Event()
        self._thread = None
        try:
            self._check()
            self._tick_event = self._open_event(L.TICK_EVENT_SUFFIX)
            self._action_event = self._open_event(L.ACTION_EVENT_SUFFIX)
        except BridgeError:
            self.close()
            raise
        self.qpc_frequency = int(self.artemis["QpcFrequency"]) or 1
        self.last_published_ticks = int(self.artemis["PublishedTicks"])
        self.tears = 0
        # Cursor of new_events: last event sequence returned, the match it belongs to, and events lost from the window.
        self.event_cursor = 0
        self.event_gaps = 0
        self._event_match = None
        # A new client starts clean: requests left by a client that died must not be taken as this one's.
        self.client["AgentMask"] = 0
        self.client["RequestPause"] = 0
        self.client["RequestTurnLimit"] = 0.0
        self.client["RequestEndRound"] = int(self.artemis["EndRoundSequence"])
        self.client["RequestMode"] = BridgeMode.Free
        self.client["RequestOwner"] = BridgeOwner.UI
        self.client["ClientPid"] = os.getpid()
        self._thread = threading.Thread(target=self._beat, name="ArtemisBridgeHeartbeat", daemon=True)
        self._thread.start()

    def _check(self):
        if int(self.artemis["Magic"]) != L.MAGIC:
            raise BridgeError(f"{self.name}: no Artemis bridge (Magic {int(self.artemis['Magic']):#x})")
        if int(self.artemis["LayoutHash"]) != L.LAYOUT_HASH:
            raise BridgeError(f"{self.name}: layout {int(self.artemis['LayoutHash']):#018x}, this client has {L.LAYOUT_HASH:#018x}: "
                              "regenerate (GenerateBridge.py) or deploy the matching DLL")

    def _open_event(self, suffix):
        handle = _kernel32.OpenEventW(_SYNCHRONIZE | _EVENT_MODIFY_STATE, False, self.name + suffix)
        if not handle:
            raise BridgeError(f"{self.name}{suffix}: event not found (error {ctypes.get_last_error()})")
        self._events.append(handle)
        return handle

    def _beat(self):
        while not self._stop.wait(HEARTBEAT_INTERVAL):
            self._heartbeat.value += 1

    @property
    def is_alive(self) -> bool:
        """Artemis still has the region open with this layout."""
        return int(self.artemis["Magic"]) == L.MAGIC

    def set_control(self, owner=None, mode=None, period=None, latency=None, scale=None, agents=None, turn_limit=None):
        """Asks for the owner, the mode, N, d, the time scale and the players (mask of player table indices) Python controls.
        Artemis applies them in its next control update (50 ms) and reports what is in effect in the Artemis part of the control.
        turn_limit: most radians per tick the facing of each agent turns toward the yaw and pitch of its action (D114), 0 = straight
        to them. Artemis reads it when it applies each decision, so a decision applied after the change already turns with it."""
        if turn_limit is not None:
            self.client["RequestTurnLimit"] = turn_limit
        if period is not None:
            self.client["RequestPeriod"] = period
        if latency is not None:
            self.client["RequestLatency"] = latency
        if scale is not None:
            self.client["RequestScale"] = scale
        if agents is not None:
            self.client["AgentMask"] = agents
        if mode is not None:
            self.client["RequestMode"] = mode
        if owner is not None:
            self.client["RequestOwner"] = owner

    def pause(self, wait=True, timeout=2.0) -> bool:
        """Holds the logic ticks of the instance while Python owns it (D143): frames run without ticks and, in Lockstep, the wait
        for a decision has no cap, so a long step (a PPO update) never times out and no action is ever applied stale. Safe at any
        point, answered or not. The heartbeat keeps running on its own thread. With wait, returns once Artemis reports IsPaused."""
        self.client["RequestPause"] = 1
        return self.wait_control(timeout, IsPaused=1) if wait else True

    def resume(self, wait=True, timeout=2.0) -> bool:
        """Lets the ticks run again. The cap of the Lockstep wait starts over from now."""
        self.client["RequestPause"] = 0
        return self.wait_control(timeout, IsPaused=0) if wait else True

    def end_round(self, wait=True, timeout=2.0) -> bool:
        """Ends the round once while Python owns the instance (D144), as the end_round action of Megalo: the team or player with the
        highest score wins, and the last round ends the match. With wait, returns once Artemis saw the request, which is not proof:
        it is dropped without ownership or outside a match. The proof, and the end of the episode, is the RoundEnd or GameOver event
        of a later tick (after the pause, inside bridge.paused(): the engine posts it in a tick)."""
        sequence = (int(self.client["RequestEndRound"]) + 1) & 0xFFFFFFFF
        self.client["RequestEndRound"] = sequence
        return self.wait_control(timeout, EndRoundSequence=sequence) if wait else True

    @contextlib.contextmanager
    def paused(self):
        """with bridge.paused(): train() -- pauses for the block and resumes after it, also on an exception."""
        self.pause()
        try:
            yield self
        finally:
            self.resume()

    def wait_control(self, timeout=2.0, **expected) -> bool:
        """Waits until the Artemis part of the control has the expected values (e.g. Owner=1, Mode=1, IsClientConnected=1)."""
        end = time.time() + timeout
        while time.time() < end:
            if all(int(self.artemis[k]) == int(v) for k, v in expected.items()):
                return True
            time.sleep(0.02)
        return False

    def read_tick(self):
        """The last published tick, copied, or None if Artemis was writing it every time it was copied."""
        for _ in range(READ_RETRIES):
            index = int(self.artemis["ActiveBuffer"])
            before = int(self.buffers["Sequence"][index])
            if before & 1:
                self.tears += 1
                continue
            data = self.buffers[index:index + 1].copy()[0]
            after = int(self.buffers["Sequence"][index])
            if after == before and int(data["Sequence"]) == before and int(data["SequenceEnd"]) == before:
                return PublishedTick(data, qpc())
            self.tears += 1
        return None

    def wait_tick(self, timeout=2.0):
        """The next tick after the last one returned, or None after the timeout (no match running, or no client connected yet).
        In Free mode ticks may be skipped if this client is slower than the decisions (PublishedTick.skipped tells how many)."""
        end = time.time() + timeout
        while True:
            count = int(self.artemis["PublishedTicks"])
            if count != self.last_published_ticks:
                published = self.read_tick()
                if published is not None:
                    published.skipped = max(0, count - self.last_published_ticks - 1)
                    self.last_published_ticks = count
                    return published
            left = end - time.time()
            if left <= 0 or not self.is_alive:
                return None
            _kernel32.WaitForSingleObject(self._tick_event, int(min(left, 0.05) * 1000) + 1)

    def new_events(self, published):
        """The events of the tick this client has not seen yet (by Sequence), oldest first. Starts over with each match.
        Events that left the window of MaxTickEvents before a tick was read are counted in event_gaps."""
        if published.match_id != self._event_match or published.last_event_sequence < self.event_cursor:
            self._event_match = published.match_id
            self.event_cursor = 0
        fresh = published.events[published.events["Sequence"] > self.event_cursor]
        if len(fresh):
            oldest = int(fresh["Sequence"][0])
            if self.event_cursor and oldest > self.event_cursor + 1:
                self.event_gaps += oldest - self.event_cursor - 1
            self.event_cursor = int(fresh["Sequence"][-1])
        return fresh

    def write_actions(self, decision_tick: int, actions: dict):
        """Writes the actions of one decision ({player table index: Action}) and publishes it. Players left out keep their previous
        action. Must be called for every tick in Lockstep, even with no actions, or the game waits for it."""
        for index, action in actions.items():
            # The slot is marked as being written (DecisionTick 0) until its fields are complete, so Artemis never copies it half done.
            self.actions["DecisionTick"][index] = 0
            self.actions["PlayerHandle"][index] = action.handle
            self.actions["Buttons"][index] = action.buttons
            self.actions["Yaw"][index] = action.yaw
            self.actions["Pitch"][index] = action.pitch
            self.actions["ThrottleForward"][index] = action.forward
            self.actions["ThrottleLeft"][index] = action.left
            self.actions["Lanes"][index] = action.lanes
            self.actions["Requests"][index] = int(action.requests)
            self.actions["DecisionTick"][index] = decision_tick
        # note: x64 keeps the order of stores, so Artemis sees the actions once it sees the sequence.
        self._action_sequence.value = decision_tick
        _kernel32.SetEvent(self._action_event)

    def counters(self) -> dict:
        """The counters of the Artemis part of the control."""
        names = ["PublishedTicks", "LateActions", "LockstepWaits", "LockstepWaitNs", "LockstepTimeouts", "HandleMismatches",
                 "RejectedActions", "ClockSkips", "ActionTick", "MatchId", "LockstepWaitMaxNs", "FlattenNs", "FlattenMaxNs",
                 "PausedFrames", "IsPaused"]
        return {name: int(self.artemis[name]) for name in names}

    def close(self):
        """Gives the instance back to the UI and leaves: heartbeat 0 tells Artemis at once (a client that dies is noticed in 2 s)."""
        if self._thread is not None:
            self.client["AgentMask"] = 0
            self.client["RequestPause"] = 0
            self.client["RequestMode"] = BridgeMode.Free
            self.client["RequestOwner"] = BridgeOwner.UI
            self._stop.set()
            self._thread.join()
            self._thread = None
            self._heartbeat.value = 0
        for handle in self._events:
            _kernel32.CloseHandle(handle)
        self._events = []
        self.artemis = self.client = self.buffers = self.actions = self.region = None
        self._action_sequence = self._heartbeat = None
        self._mm.close()

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()
