"""
bridge_check.py
Tests of the bridge against a running MCC (D135, Diseno/12 sec. 8: T1-T5). Runs on Windows with the venv of Agent:
python Tools/game.py py Agent/scripts/bridge_check.py <command> [options]

    info                                   The open regions and the control block of the newest one.
    ticks [--count 60] [--scale S]        T1/T2: consecutive ticks, layout, copies that hit a write, clock of the ticks.
    run [--mode lockstep|free] [--seconds 60] [--agents bots|all] [--scale S] [--period N] [--latency D] [--crash-after K]
                                           T3-T5: trivial policy (walk forward, YawDelta --yaw-step rad per decision), the decision
                                           each player had applied (known answer) and how far its aim turned between two decisions
                                           against the delta sent (the AgentInput applied), the imitation block when the user possesses
                                           a player (D364), latencies, ticks per second. --crash-after K exits without closing after K decisions (the client dies).
        [--pause-every K --pause-seconds S]  P.5 (D143): every K decisions holds a pause of S s, alternating after answering the
                                           decision and before answering it (the game waits inside the tick). No tick may run
                                           during a pause and the known answer must hold across it.
    endround [--round-seconds 10] [--seconds 300] [--scale S] [--stale]
                                           D144: asks for the end of each round after --round-seconds of play (from the start, or
                                           the first Spawn after a RoundEnd) and checks that RoundEnd follows, or GameOver in the
                                           last round. Python owns the instance without agents (Free mode); the events from before it
                                           attached are skipped. --stale first asks
                                           for one outside a match (start it in the lobby): it must not end the first round.
    preset --names a,b [--rounds N] [--reject] [--seconds 120] [--scale S]
                                           Q97: for each of N rounds (default one per name) requests the next preset of the list in
                                           the middle of the round (Pending), ends the round and checks the result at the break:
                                           Applied and the active preset is the one asked. Then waits for the next Spawn. With
                                           --reject the names must be presets the match cannot take (artemis_ctf_colors changes
                                           load fields): the result is Rejected and the active preset does not change.
                                           The match has to start with artemis_ctf_base (Bench: preset artemis_ctf_base).
    map --names a,b [--rounds N] [--reject NAME] [--seconds 120] [--scale S]
                                           Q98: for each of N rounds (default one per name) requests the next map variant of the list (the
                                           name of a recorded match config of the loaded map) in the middle of the round (Pending), ends
                                           the round and checks the result at the break: Applied and the active map is the one asked. Then
                                           waits for the next Spawn and checks that the positions of the flag stands of the new round
                                           changed (a different layout) when the map variant is a different one than before. With --reject
                                           NAME it requests that map variant as well in the first round and expects Rejected (for
                                           example a config of another .map) with the active map unchanged. The match has to start with
                                           one of the names, or with the map variant of its config (Bench: map <name>).
    setup [--preset NAME] [--map NAME] [--roster 0:3,1:4,auto:2] [--reject] [--seconds 120] [--scale S]
                                           LS.2: requests one match setup (the fields given) in the middle of a round (Pending), ends the
                                           round and checks the result at the break: Applied, and the active preset, map and roster are
                                           the ones asked (a name "" asks for the base values or the map the match loaded with). With
                                           --reject the result must be Rejected with a reason and the active setup must not change.
                                           "preset" and "map" are the same request with one field.
    controls [--context foot|nogren|flag|vehicle] [--imitation] [--phase-seconds 6] [--scale 1] [--player N] [--only a,b] [--wait 120]
                                           E1 (D355, D364): one phase per control of the AgentInput, each checked by what the Tick shows
                                           (PASS / FAIL / SKIP / SEE). Lockstep N = d = 4, ONE agent (the first live bot with a body, or the
                                           player of table index N), the others stay with the engine. Follow it with Watch POV: each phase
                                           prints its name (and beeps). flag / vehicle wait for the user to possess the bot, take the flag /
                                           board the Warthog and release it. --imitation sends nothing: the user possesses the bot and does
                                           each control by hand; the PossessedInput of each phase is checked. Also written to
                                           Ignore/Temporal/vw9c/controls_<context>_<time>.txt. Exit 0 without FAIL, 1 with FAIL, 2 usage.
    turn [--turn-limit 0.1] [--turn-step 2.5] [--hold 15] [--seconds 60] [--scale S]
                                           D114: Lockstep, every bot gets a new target every --hold decisions (yaw +--turn-step,
                                           pitch +-0.4), asked as YawDelta / PitchDelta in steps under the turn limit. The aim seen between two ticks never turns faster than --turn-limit rad
                                           per tick and reaches each target before the next one.
Exit code 0 when every check of the command passes.
"""
import argparse
import math
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import numpy as np  # noqa: E402

from artemis import bridge_layout as L  # noqa: E402
from artemis.bridge import Action, Bridge, BridgeError, BridgeMode, BridgeOwner, BridgePhase, discover, qpc  # noqa: E402
from artemis.control_check import (CONTEXTS, COUNTERS, NO_HANDLE, DRAIN_SECONDS, FormatLine, GrewCounters, Grenades, PhaseInput,  # noqa: E402
                                   PhasesOf, PreInput, Result, SampleOf, Summarize, WindowOf, AngleGap, CheckImitation, CheckPhase)
from artemis.semantic import Event, MatchPhase  # noqa: E402

YAW_STEP = 0.25
AIM_TOLERANCE = 0.02


def Open(args) -> Bridge:
    regions = discover()
    if args.pid:
        return Bridge(args.pid)
    if not regions:
        raise BridgeError("no open region (is MCC running with Artemis?)")
    return Bridge(int(regions[0]["pid"]))


# ticks: seconds without a published tick, with the match still running, before the check stops waiting.
TICK_SILENCE_SECONDS = 10


def WaitRunning(bridge, seconds) -> bool:
    end = time.time() + seconds
    while time.time() < end:
        if int(bridge.artemis["Phase"]) == BridgePhase.Running:
            return True
        time.sleep(0.2)
    return False


def HoldPause(bridge, seconds):
    """Pauses for the given time and returns the action ticks that ran in it after the first 0.5 s (must be 0), or None."""
    try:
        if not bridge.pause():
            return None
        time.sleep(0.5)
        first = int(bridge.artemis["ActionTick"])
        time.sleep(max(seconds - 0.5, 0.0))
        return int(bridge.artemis["ActionTick"]) - first
    finally:
        bridge.resume()


def Percentile(values, p):
    return float(np.percentile(values, p)) if values else float("nan")


def Info(args):
    regions = discover()
    print(f"regions: {len(regions)} " + ", ".join(f"pid {r['pid']}" for r in regions))
    with Open(args) as bridge:
        time.sleep(0.3)
        a = bridge.artemis
        fields = ["Pid", "Phase", "Owner", "Mode", "DecisionPeriod", "Latency", "IsClientConnected", "MatchId", "ActionTick",
                  "Scale", "PublishedTicks", "ClockSkips", "LockstepTimeouts", "HandleMismatches"]
        print(f"layout {int(a['LayoutHash']):#018x} (client {L.LAYOUT_HASH:#018x}), version {int(a['LayoutVersion'])}, "
              f"{int(a['RegionSize'])} bytes, heartbeat {int(a['Heartbeat'])}")
        print(" | ".join(f"{name} {a[name]}" for name in fields))
    return 0


def Ticks(args):
    with Open(args) as bridge:
        if not WaitRunning(bridge, args.wait):
            print(f"FAIL no match running after {args.wait} s")
            return 1
        if args.scale:
            bridge.set_control(owner=BridgeOwner.Python, mode=BridgeMode.Free, scale=args.scale, agents=0)
        before = bridge.counters()
        period = int(bridge.artemis["DecisionPeriod"])
        received, gaps, bad, ticks, entities = 0, 0, [], [], []
        events, kinds, game_ticks = 0, {}, []
        start = time.time()
        last, silent_since, stopped = None, time.time(), ""
        while received < args.count and time.time() - start < args.count + 30:
            published = bridge.wait_tick(2.0)
            if published is None:
                # The match ended (or the game stopped publishing): report what arrived instead of waiting for the whole count.
                if int(bridge.artemis["Phase"]) != BridgePhase.Running:
                    stopped = "match no longer running"
                elif time.time() - silent_since > TICK_SILENCE_SECONDS:
                    stopped = f"no tick for {TICK_SILENCE_SECONDS} s"
                if stopped:
                    break
                continue
            last, silent_since = published, time.time()
            received += 1
            gaps += published.skipped
            ticks.append(published.action_tick)
            entities.append(len(published.entities))
            h = published.header
            if published.action_tick % period:
                bad.append(f"tick {published.action_tick} not a multiple of N {period}")
            if int(h["PlayerCount"]) > L.MAX_PLAYERS or int(h["EntityCount"]) > L.MAX_ENTITIES:
                bad.append(f"counts {int(h['PlayerCount'])}/{int(h['EntityCount'])}")
            if int(h["ObjectiveCount"]) > L.MAX_OBJECTIVES or int(h["StandCount"]) > L.MAX_OBJECTIVES:
                bad.append(f"objective counts {int(h['ObjectiveCount'])}/{int(h['StandCount'])}")
            if int(h["WeaponCount"]) > L.MAX_WEAPONS or int(h["ProjectileCount"]) > L.MAX_PROJECTILES:
                bad.append(f"weapon/projectile counts {int(h['WeaponCount'])}/{int(h['ProjectileCount'])}")
            handles = published.entities["Handle"]
            if len(handles) and (np.any(handles == L.NO_HANDLE) or np.any(np.diff(handles.astype(np.int64)) <= 0)):
                bad.append(f"entity handles not sorted or NONE at tick {published.action_tick}")
            if np.any((published.players["Handle"] & 0xFFFF) >= L.MAX_PLAYERS):
                bad.append(f"player index out of range at tick {published.action_tick}")
            fresh = bridge.new_events(published)
            if len(fresh) > 1 and np.any(np.diff(fresh["Sequence"].astype(np.int64)) != 1):
                bad.append(f"event sequences not consecutive at tick {published.action_tick}")
            events += len(fresh)
            for kind in fresh["Kind"]:
                name = Event(int(kind)).name if int(kind) in Event._value2member_map_ else str(int(kind))
                kinds[name] = kinds.get(name, 0) + 1
            game_tick = int(published.match["GameTick"])
            if game_ticks and game_tick < game_ticks[-1]:
                bad.append(f"match game tick went back {game_ticks[-1]} -> {game_tick}")
            game_ticks.append(game_tick)
        elapsed = time.time() - start
        after = bridge.counters()
        steps = np.diff(ticks) if len(ticks) > 1 else np.array([])
        irregular = int(np.sum(steps != period)) if len(steps) else 0
        if stopped:
            print(f"stopped early: {stopped}")
        print(f"received {received} in {elapsed:.1f} s, skipped {gaps}, steps != N: {irregular} (with skips), copies retried {bridge.tears}")
        print(f"entities avg {np.mean(entities) if entities else 0:.0f} max {max(entities) if entities else 0}, "
              f"players {len(last.players) if received else 0}, flags {len(last.objectives) if received else 0}, "
              f"stands {len(last.stands) if received else 0}, clock skips +{after['ClockSkips'] - before['ClockSkips']}")
        phase = MatchPhase(int(last.match["Phase"])).name if received else "-"
        print(f"events {events} (lost {bridge.event_gaps}) {kinds}, match ticks {game_ticks[0] if game_ticks else 0}-"
              f"{game_ticks[-1] if game_ticks else 0}, phase {phase}, scores {[int(s) for s in last.match['PlayerScores'][:4]] if received else []}")
        for line in bad[:10]:
            print("  bad:", line)
        bridge.set_control(owner=BridgeOwner.UI, scale=0.0)
        ok = received >= args.count and not bad and after["ClockSkips"] == before["ClockSkips"] and bridge.event_gaps == 0
        print("PASS" if ok else "FAIL")
        return 0 if ok else 1


def CheckSparse(bridge, args):
    """Q101: with --sparse Artemis must report the effective sparse mode once Python owns the instance; without it, never.
    A recorder or a tool that needs every tick (TickDump, the bench) keeps it off on purpose, so that is a failure to look at."""
    if not bridge.wait_control(IsSparse=1 if args.sparse else 0, timeout=3.0):
        print(f"FAIL sparse {'requested' if args.sparse else 'not requested'} but Artemis reports IsSparse={int(bridge.is_sparse)} "
              f"(a TickDump recording or the bench keeps every tick)")
        return False
    print(f"sparse: requested {args.sparse}, effective {bridge.is_sparse}")
    return True


def Run(args):
    lockstep = args.mode == "lockstep"
    with Open(args) as bridge:
        if not WaitRunning(bridge, args.wait):
            print(f"FAIL no match running after {args.wait} s")
            return 1
        bridge.set_control(owner=BridgeOwner.Python, mode=BridgeMode.Lockstep if lockstep else BridgeMode.Free,
                           period=args.period, latency=args.latency, scale=args.scale, agents=0, sparse=args.sparse, turn_limit=0.0)
        if not bridge.wait_control(Owner=BridgeOwner.Python, IsClientConnected=1):
            print("FAIL Artemis did not hand the instance over")
            return 1
        if not CheckSparse(bridge, args):
            return 1
        period, latency = args.period, args.latency
        before = bridge.counters()
        freq = bridge.qpc_frequency
        mask, matches = 0, {}
        # Per (match, player): the aim yaw of the previous decision and the decisions in a row it was seen alive with a body.
        seen_aim, streak, sent = {}, {}, set()
        possessed_ticks = possessed_moves = 0
        recv, respond = [], []
        applied_ok = applied_total = aim_ok = aim_total = decisions = 0
        first_pos, last_pos = {}, {}
        pauses, paused_seconds, pause_just_ended = [], 0.0, False
        start = time.time()
        while time.time() - start < args.seconds:
            published = bridge.wait_tick(2.0)
            if published is None:
                if not bridge.is_alive:
                    print("Artemis closed the bridge")
                    break
                continue
            # note: A tick published just before a pause is read after it, so it is not a latency.
            if not pause_just_ended:
                recv.append((published.received_qpc - int(published.header["PublishQpc"])) / freq * 1000)
            pause_just_ended = False
            tick = published.action_tick
            stats = matches.setdefault(published.match_id, {"decisions": 0, "players": 0, "first": tick, "last": tick})
            stats["decisions"] += 1
            stats["last"] = tick
            agents = [p for p in published.players if args.agents == "all" or int(p["LocalIndex"]) == L.NOT_LOCAL]
            stats["players"] = max(stats["players"], len(agents))
            new_mask = 0
            for p in agents:
                new_mask |= 1 << (int(p["Handle"]) & 0xFFFF)
            entities = {int(e["Handle"]): e for e in published.entities}
            # note: the imitation block (D364): PossessedPlayer is a handle, PossessedInput what that player did in the last closed window.
            if published.possessed_player != L.NO_HANDLE:
                possessed_ticks += 1
                pi = published.possessed_input
                possessed_moves += bool(pi["MoveForward"] or pi["MoveLeft"] or pi["YawDelta"] or pi["PitchDelta"] or pi["Level"] or pi["Edge"])
            for p in agents:
                index = int(p["Handle"]) & 0xFFFF
                applied = int(p["AppliedDecision"])
                previous = tick - period
                if (mask >> index) & 1 and (published.match_id, previous) in sent:
                    applied_total += 1
                    applied_ok += applied == previous
                key = (published.match_id, int(p["Handle"]))
                # note: the possessed player turns with the user, not with the delta sent: its known answer starts again after the release.
                if int(p["HasBody"]) and int(p["IsAlive"]) and int(p["Handle"]) != published.possessed_player:
                    seen = AimAngles(p)[0]
                    streak[key] = streak.get(key, 0) + 1
                    # Known answer of the AgentInput applied: with yaw_step per decision and no turn limit the aim turns yaw_step between
                    # two decisions in a row (the delta of the window of the previous decision has been played). The first decisions of
                    # a life or of the attach have no previous window to play, so they are not counted.
                    if streak[key] > 3 and key in seen_aim:
                        turned = (seen - seen_aim[key] + math.pi) % (2 * math.pi) - math.pi
                        aim_total += 1
                        aim_ok += abs(turned - args.yaw_step) < AIM_TOLERANCE
                    seen_aim[key] = seen
                else:
                    streak.pop(key, None)
                    seen_aim.pop(key, None)
                body = entities.get(int(p["BipedHandle"]))
                if body is not None and int(p["IsAlive"]):
                    position = np.array([float(body["WorldOrigin"][k]) for k in ("X", "Y", "Z")])
                    key = (published.match_id, int(p["Handle"]))
                    first_pos.setdefault(key, position)
                    last_pos[key] = max(last_pos.get(key, 0.0), float(np.linalg.norm(position - first_pos[key])))
            if new_mask != mask:
                bridge.set_control(agents=new_mask)
                mask = new_mask
            is_pause = bool(args.pause_every) and decisions > 0 and decisions % args.pause_every == 0
            is_answered = len(pauses) % 2 == 0
            if is_pause and not is_answered:
                pause_start = time.time()
                pauses.append(("before answering", tick, HoldPause(bridge, args.pause_seconds)))
                paused_seconds += time.time() - pause_start
                pause_just_ended = True
            bridge.write_actions(tick, {int(p["Handle"]) & 0xFFFF: Action(handle=int(p["Handle"]), forward=1.0, yaw_delta=args.yaw_step)
                                        for p in agents})
            sent.add((published.match_id, tick))
            sent -= {s for s in sent if s[1] < tick - 16 * period}
            if not is_pause:
                respond.append((qpc() - int(published.header["PublishQpc"])) / freq * 1000)
            if is_pause and is_answered:
                pause_start = time.time()
                pauses.append(("after answering", tick, HoldPause(bridge, args.pause_seconds)))
                paused_seconds += time.time() - pause_start
                pause_just_ended = True
            decisions += 1
            if args.crash_after and decisions >= args.crash_after:
                print(f"crash: exiting after {decisions} decisions without closing (Lockstep was {lockstep})")
                sys.stdout.flush()
                os._exit(3)
        elapsed = time.time() - start - paused_seconds
        after = bridge.counters()
        delta = {k: after[k] - before[k] for k in after}
        moved = list(last_pos.values())
        # note: The action tick starts over with every match, so the ticks are added match by match.
        ticks = sum(stats["last"] - stats["first"] for stats in matches.values())
        waits = delta["LockstepWaits"]
        print(f"mode {args.mode} N {period} d {latency} scale {args.scale or 'engine'} agents {args.agents}: {decisions} decisions "
              f"in {elapsed:.1f} s, {ticks / elapsed:.0f} ticks/s, matches {len(matches)}")
        for match, stats in matches.items():
            print(f"  match {match}: {stats['decisions']} decisions, ticks {stats['first']}-{stats['last']}, up to {stats['players']} agents")
        print(f"known answer: applied = previous decision {applied_ok}/{applied_total}, turned = yaw delta sent {aim_ok}/{aim_total}, "
              f"farthest from the start min {min(moved) if moved else 0:.1f} m over {len(moved)} lives")
        print(f"possessed block: {possessed_ticks} ticks with a PossessedPlayer, {possessed_moves} of them with a non-empty PossessedInput")
        print(f"latency ms: publish->read p50 {Percentile(recv, 50):.3f} p99 {Percentile(recv, 99):.3f} | publish->answer "
              f"p50 {Percentile(respond, 50):.3f} p99 {Percentile(respond, 99):.3f}")
        print(f"game waits {waits} avg {(delta['LockstepWaitNs'] / waits / 1e6) if waits else 0:.3f} ms, timeouts "
              f"{delta['LockstepTimeouts']}, late {delta['LateActions']}, handle mismatches {delta['HandleMismatches']}, rejected "
              f"{delta['RejectedActions']}, clock skips {delta['ClockSkips']}, ticks skipped by the client "
              f"{delta['PublishedTicks'] - decisions}, copies retried {bridge.tears}")
        print(f"since load: wait max {after['LockstepWaitMaxNs'] / 1e6:.3f} ms, flatten max {after['FlattenMaxNs'] / 1e6:.3f} ms | "
              f"this run: flatten avg "
              f"{(delta['FlattenNs'] / delta['PublishedTicks'] / 1e6) if delta['PublishedTicks'] else 0:.3f} ms, paused frames "
              f"{delta['PausedFrames']}")
        for when, tick, ran in pauses:
            print(f"  pause {when} decision {tick}: {'not confirmed' if ran is None else f'{ran} ticks ran'}")
        # note: The restart of the clock with every match is not a skip (P.5), so none is tolerated.
        checks = [("decisions", decisions > 0), ("no clock skips", delta["ClockSkips"] == 0)]
        if args.pause_every:
            checks += [("pauses held", pauses and all(ran == 0 for _, _, ran in pauses)), ("resumed", int(bridge.artemis["IsPaused"]) == 0)]
            if len(pauses) >= 2:
                checks.append(("pauses of both kinds", len({when for when, _, _ in pauses}) == 2))
            if not lockstep:
                # note: In Lockstep the game mostly holds inside the wait. In Free mode only the frames without ticks hold it.
                checks.append(("frames without ticks", delta["PausedFrames"] > 0))
        if lockstep:
            checks += [("applied = previous decision >= 99%", applied_total and applied_ok >= 0.99 * applied_total),
                       ("no Lockstep timeouts", delta["LockstepTimeouts"] == 0),
                       ("no tick skipped", delta["PublishedTicks"] - decisions <= 1)]
        if aim_total:
            checks.append(("turned = yaw delta sent >= 95%", aim_ok >= 0.95 * aim_total))
        failed = [name for name, ok in checks if not ok]
        print("PASS" if not failed else "FAIL " + ", ".join(failed))
        return 0 if not failed else 1


def EndRound(args):
    with Open(args) as bridge:
        if args.stale:
            bridge.set_control(owner=BridgeOwner.Python, mode=BridgeMode.Free, agents=0)
            if int(bridge.artemis["Phase"]) == BridgePhase.Running or not bridge.wait_control(Owner=BridgeOwner.Python):
                print("FAIL --stale needs the lobby and the instance handed to Python")
                return 1
            bridge.end_round()
            print("  end of round requested outside a match")
        if not WaitRunning(bridge, args.wait):
            print(f"FAIL no match running after {args.wait} s")
            return 1
        bridge.set_control(owner=BridgeOwner.Python, mode=BridgeMode.Free, scale=args.scale, agents=0)
        if not bridge.wait_control(Owner=BridgeOwner.Python):
            print("FAIL Artemis did not hand the instance to Python")
            return 1
        rounds, bad, kinds = [], [], {}
        requested, round_start, is_over, attached = None, None, False, False
        start = time.time()
        while time.time() - start < args.seconds and not is_over:
            published = bridge.wait_tick(2.0)
            if published is None:
                continue
            tick = int(published.match["GameTick"])
            if not attached:
                # note: The first read returns the events still in the window, from before this client attached (an earlier
                # RoundEnd would count as one without a request).
                bridge.new_events(published)
                attached = True
            if round_start is None and not rounds:
                round_start = tick
            for event in bridge.new_events(published):
                kind, event_tick = int(event["Kind"]), int(event["GameTick"])
                name = Event(kind).name if kind in Event._value2member_map_ else str(kind)
                if kind in (Event.RoundEnd, Event.GameOver, Event.Spawn):
                    kinds[name] = kinds.get(name, 0) + 1
                if kind == Event.Spawn and round_start is None and requested is None:
                    round_start = event_tick
                elif kind in (Event.RoundEnd, Event.GameOver):
                    if requested is None:
                        bad.append(f"{name} at tick {event_tick} without a request")
                    rounds.append((requested, event_tick, name))
                    requested, round_start, is_over = None, None, kind == Event.GameOver
            if requested is not None and tick - requested > 300:
                bad.append(f"no RoundEnd or GameOver 300 ticks after the request at tick {requested}")
                break
            if requested is None and round_start is not None and tick - round_start >= args.round_seconds * 60:
                if not bridge.end_round():
                    bad.append(f"request not taken at tick {tick} (EndRoundSequence did not echo it)")
                    break
                requested = tick
                print(f"  end of round requested at match tick {tick}")
        bridge.set_control(owner=BridgeOwner.UI, scale=0.0)
        for asked, ended, name in rounds:
            print(f"  {name} at match tick {ended}" + (f", {ended - asked} ticks after the request" if asked is not None else ""))
        print(f"events {kinds}, lost {bridge.event_gaps}, game over {is_over}")
        for line in bad[:10]:
            print("  bad:", line)
        checks = [("at least one round ended on request", any(asked is not None for asked, _, _ in rounds)),
                  ("every request answered", not bad), ("match ended with GameOver", is_over),
                  ("no lost events", bridge.event_gaps == 0)]
        failed = [name for name, ok in checks if not ok]
        print("PASS" if not failed else "FAIL " + ", ".join(failed))
        return 0 if not failed else 1


# controls: the lockstep of the test (D304), the quiet between two phases, the most the runner waits for a tick.
CONTROLS_PERIOD = 4
QUIET_SECONDS = 1.0
LEAD_IN_SECONDS = 3.0
TICK_WAIT_SECONDS = 15.0


class Driver:
    """One agent of the controls test: waits for each decision and answers it (Lockstep needs an answer to every tick)."""

    def __init__(self, bridge):
        self.bridge = bridge
        self.mask = 0
        self.handle = NO_HANDLE
        self.published = None

    def Wait(self):
        end = time.time() + TICK_WAIT_SECONDS
        while True:
            published = self.bridge.wait_tick(2.0)
            if published is not None:
                self.published = published
                return published
            if not self.bridge.is_alive:
                raise BridgeError("Artemis closed the bridge")
            if time.time() > end:
                raise BridgeError(f"no tick in {TICK_WAIT_SECONDS:.0f} s (is the match running?)")

    def Agents(self, on):
        mask = (1 << (self.handle & 0xFFFF)) if on and self.handle != NO_HANDLE else 0
        if mask != self.mask:
            self.bridge.set_control(agents=mask)
            self.mask = mask

    def Answer(self, fields=None):
        actions = {}
        if fields is not None and self.mask:
            actions[self.handle & 0xFFFF] = Action(handle=self.handle, **fields)
        self.bridge.write_actions(self.published.action_tick, actions)

    def Deliver(self, make_input, seconds, decisions=0, stop=None):
        """The input of make_input(first) on every decision for `seconds` and at least `decisions`; returns the samples (one per
        decision: what the agent was like at the start of that decision, so the effect of an input is in the next ones)."""
        end, samples, first = time.time() + seconds, [], True
        while time.time() < end or len(samples) < decisions:
            published = self.Wait()
            sample = SampleOf(published, self.handle)
            if sample is not None:
                samples.append(sample)
            self.Answer(make_input(first))
            first = False
            if stop is not None and sample is not None and stop(sample):
                break
        return samples

    def Quiet(self, seconds, decisions=0):
        samples = self.Deliver(lambda first: PhaseInput(None, first, CONTROLS), seconds, decisions)
        # note: a dead agent is waited for (up to 30 s) so the next phase starts with a body.
        end = time.time() + 30.0
        while samples and not (samples[-1].alive and samples[-1].has_body) and time.time() < end:
            samples = self.Deliver(lambda first: PhaseInput(None, first, CONTROLS), 0.0, 1)
        return samples


def Beep():
    try:
        import winsound
        winsound.Beep(880, 200)
    except (ImportError, RuntimeError):
        pass


def PickAgent(published, index):
    """Handle of the agent: the player of table index `index`, or the first live bot with a body."""
    bots = [p for p in published.players if int(p["LocalIndex"]) == L.NOT_LOCAL]
    if index is not None:
        found = [p for p in published.players if int(p["Handle"]) & 0xFFFF == index]
        return int(found[0]["Handle"]) if found else NO_HANDLE
    for p in bots:
        if int(p["IsAlive"]) and int(p["HasBody"]):
            return int(p["Handle"])
    return int(bots[0]["Handle"]) if bots else NO_HANDLE


def WaitFor(driver, ready, seconds, request, say):
    """Answers every decision with nothing (the engine plays the bot) until ready(sample, published), up to `seconds`."""
    driver.Agents(False)
    end, next_print = time.time() + seconds, 0.0
    say(request)
    while time.time() < end:
        published = driver.Wait()
        driver.Answer(None)
        sample = SampleOf(published, driver.handle)
        if sample is not None and ready(sample, published):
            return True
        if time.time() > next_print:
            next_print = time.time() + 10.0
            say(f"  waiting ... ({end - time.time():.0f} s left)")
    return False


CONTROLS = None  # bridge.CONTROLS, set in Controls (the module of the layout is the one the runner imports)


def Controls(args):
    global CONTROLS
    from artemis.bridge import CONTROLS as BRIDGE_CONTROLS
    CONTROLS = BRIDGE_CONTROLS
    context = "foot" if args.imitation else args.context
    try:
        phases = PhasesOf(context, args.only.split(",") if args.only else None)
    except ValueError as error:
        print(f"usage: {error}")
        return 2
    label = "imitation" if args.imitation else args.context
    lines = []

    def Say(text):
        print(text)
        sys.stdout.flush()
        lines.append(text)

    results = []

    def Record(name, result):
        results.append(result)
        Say(FormatLine(label, name, result))

    with Open(args) as bridge:
        if not WaitRunning(bridge, args.wait):
            print(f"FAIL no match running after {args.wait} s")
            return 1
        bridge.set_control(owner=BridgeOwner.Python, mode=BridgeMode.Lockstep, period=CONTROLS_PERIOD, latency=CONTROLS_PERIOD,
                           scale=args.scale or 1.0, agents=0, turn_limit=0.0)
        if not bridge.wait_control(Owner=BridgeOwner.Python, IsClientConnected=1):
            print("FAIL Artemis did not hand the instance over")
            return 1
        driver = Driver(bridge)
        before = bridge.counters()
        failed_run = []
        driver.Wait()
        driver.handle = PickAgent(driver.published, args.player)
        if not args.imitation and driver.handle == NO_HANDLE:
            print("FAIL no bot (or no player with that index) to play")
            return 1
        Say(f"{label}: agent handle {driver.handle:#x} (table index {driver.handle & 0xFFFF}), {len(phases)} phases of {args.phase_seconds:g} s")
        if args.imitation:
            Imitation(driver, phases, args, Say, Record)
        else:
            driver.Agents(context in ("foot", "nogren"))
            if context == "nogren" and not Drain(driver, Say, Record):
                failed_run.append("no se vacio")
            elif context in ("flag", "vehicle") and not Prepare(driver, context, args, Say):
                print(f"FAIL the condition of the {context} context did not happen in {args.wait} s")
                return 1
            else:
                driver.Agents(True)
                driver.Quiet(QUIET_SECONDS, 2)
                for phase in phases:
                    Say(f">>> {phase.name}")
                    Beep()
                    pre = []
                    if phase.pre:
                        pre = driver.Deliver(lambda first, ph=phase: PreInput(ph.pre, first, CONTROLS), phase.pre.seconds, phase.pre.decisions)
                        driver.Quiet(0.0, 2)
                    got = driver.Deliver(lambda first, ph=phase: PhaseInput(ph, first, CONTROLS), args.phase_seconds)
                    Record(phase.name, CheckPhase(phase, got[1:], got[0] if got else None, context, {"pre": pre}))
                    driver.Quiet(QUIET_SECONDS, 2)
        after = bridge.counters()
        grew = GrewCounters(before, after)
        counts = Summarize(results)
        if grew:
            failed_run.append("counters grew: " + ", ".join(grew))
        Say(f"summary {label}: PASS {counts['PASS']} FAIL {counts['FAIL']} SKIP {counts['SKIP']} SEE {counts['SEE']}")
        Say("counters before -> after: " + ", ".join(f"{n} {before[n]} -> {after[n]}" for n in COUNTERS))
        for name in failed_run:
            Say(f"FAIL {name}")
        bad = counts["FAIL"] or failed_run
        Say("FAIL" if bad else "PASS")
    Save(label, lines)
    return 1 if bad else 0


def Drain(driver, Say, Record):
    """nogren: throws grenades until FragGrenades + PlasmaGrenades = 0 (up to DRAIN_SECONDS)."""
    from artemis.bridge import CONTROLS as controls
    throw = PhasesOf("foot", ["throw_grenade"])[0]
    Say(">>> drain: throwing every grenade")
    samples = driver.Deliver(lambda first: PhaseInput(throw, first, controls), DRAIN_SECONDS, 1, stop=lambda s: Grenades(s) == 0 and s.alive)
    if not samples or Grenades(samples[-1]) != 0:
        Record("drain", Result("FAIL", f"not emptied in {DRAIN_SECONDS:.0f} s: grenades left {Grenades(samples[-1]) if samples else '?'}"))
        return False
    Say(f"drain: empty after {len(samples)} decisions")
    return True


def Prepare(driver, context, args, Say):
    if context == "flag":
        return WaitFor(driver, lambda s, p: s.carrying and p.possessed_player == NO_HANDLE, args.wait,
                       f"POSSESS the bot (table index {driver.handle & 0xFFFF}), take the flag, then RELEASE it (D367: it repeats until the first delivery)", Say)
    Say("in vehicle = the Entity of the biped has Binding = Seated and BoundTo = the vehicle handle")
    return WaitFor(driver, lambda s, p: s.vehicle != NO_HANDLE and p.possessed_player == NO_HANDLE, args.wait,
                   f"POSSESS the bot (table index {driver.handle & 0xFFFF}), board the Warthog as the driver, then RELEASE it", Say)


def Imitation(driver, phases, args, Say, Record):
    """--imitation: sends nothing; the user possesses the bot and does each control; the PossessedInput of each phase is checked."""
    driver.Agents(False)

    def Windows(seconds):
        windows = []
        end = time.time() + seconds
        while time.time() < end:
            published = driver.Wait()
            windows.append(WindowOf(published))
            driver.Answer(None)
        return windows
    Say(f"POSSESS the bot (table index {driver.handle & 0xFFFF}); the first phase starts in {LEAD_IN_SECONDS:g} s")
    Windows(LEAD_IN_SECONDS)
    for phase in phases:
        Say(f">>> {phase.name}: {phase.hint}")
        Beep()
        Record(phase.name, CheckImitation(phase, Windows(args.phase_seconds), CONTROLS))
        Windows(QUIET_SECONDS)


def Save(label, lines):
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    folder = os.path.join(root, "Ignore", "Temporal", "vw9c")
    try:
        os.makedirs(folder, exist_ok=True)
        path = os.path.join(folder, f"controls_{label}_{time.strftime('%H%M%S')}.txt")
        with open(path, "w", encoding="ascii", errors="replace", newline="\r\n") as out:
            out.write("\n".join(lines))
        print(f"written {path}")
    except OSError as error:
        print(f"could not write the report: {error}")


def AimAngles(player):
    """Yaw in [0, 2 pi) and pitch of the aim of a player, in radians."""
    aim = player["Aim"]
    x, y, z = float(aim["X"]), float(aim["Y"]), float(aim["Z"])
    return math.atan2(y, x) % (2 * math.pi), math.atan2(z, math.hypot(x, y))


def Turn(args):
    limit, period = args.turn_limit, args.period
    with Open(args) as bridge:
        if not WaitRunning(bridge, args.wait):
            print(f"FAIL no match running after {args.wait} s")
            return 1
        bridge.set_control(owner=BridgeOwner.Python, mode=BridgeMode.Lockstep, period=period, latency=args.latency,
                           scale=args.scale, agents=0, turn_limit=limit, sparse=args.sparse)
        if not bridge.wait_control(Owner=BridgeOwner.Python, IsClientConnected=1):
            print("FAIL Artemis did not hand the instance over")
            return 1
        if not CheckSparse(bridge, args):
            return 1
        # targets: the aim each agent has to reach (absolute, for the check). pending: what is left to ask of it as YawDelta / PitchDelta,
        # open loop (the aim seen lags the decision by d ticks). A delta above limit * window ticks would be cut and lost, so it goes in
        # steps of 90% of that.
        targets, pending, last_aim, over = {}, {}, {}, []
        window = min(period, args.latency)
        step = 0.9 * limit * window if limit > 0 else math.inf
        mask = decisions = settled_ok = settled_total = 0
        max_rate = 0.0
        start = time.time()
        while time.time() - start < args.seconds:
            published = bridge.wait_tick(2.0)
            if published is None:
                if not bridge.is_alive:
                    print("Artemis closed the bridge")
                    break
                continue
            tick = published.action_tick
            is_change = decisions > 0 and decisions % args.hold == 0
            actions, new_mask = {}, 0
            for p in published.players:
                if int(p["LocalIndex"]) != L.NOT_LOCAL:
                    continue
                index = int(p["Handle"]) & 0xFFFF
                new_mask |= 1 << index
                if not (int(p["HasBody"]) and int(p["IsAlive"])):
                    last_aim.pop(index, None)
                    targets.pop(index, None)
                    pending.pop(index, None)
                    continue
                yaw, pitch = AimAngles(p)
                if index in last_aim:
                    seen_tick, seen_yaw, seen_pitch = last_aim[index]
                    ticks = tick - seen_tick
                    if 0 < ticks <= 4 * period:
                        rate = max(AngleGap(yaw, seen_yaw), abs(pitch - seen_pitch)) / ticks
                        max_rate = max(max_rate, rate)
                        if rate > limit + AIM_TOLERANCE / period:
                            over.append(f"player {index} turned {rate:.4f} rad per tick at action tick {tick}")
                last_aim[index] = (tick, yaw, pitch)
                if index not in targets:
                    targets[index] = (yaw, 0.0)
                    pending[index] = [0.0, -pitch]
                elif is_change:
                    target_yaw, target_pitch = targets[index]
                    settled_total += 1
                    settled_ok += AngleGap(yaw, target_yaw) < AIM_TOLERANCE and abs(pitch - target_pitch) < AIM_TOLERANCE
                    new_pitch = -target_pitch if target_pitch else 0.4
                    targets[index] = ((target_yaw + args.turn_step) % (2 * math.pi), new_pitch)
                    pending[index][0] += args.turn_step
                    pending[index][1] += new_pitch - target_pitch
                send = [max(-step, min(step, value)) for value in pending[index]]
                pending[index][0] -= send[0]
                pending[index][1] -= send[1]
                actions[index] = Action(handle=int(p["Handle"]), yaw_delta=send[0], pitch_delta=send[1])
            if new_mask != mask:
                bridge.set_control(agents=new_mask)
                mask = new_mask
            bridge.write_actions(tick, actions)
            decisions += 1
        bridge.set_control(owner=BridgeOwner.UI, scale=0.0, agents=0, turn_limit=0.0)
        print(f"decisions {decisions}, agents {bin(mask).count('1')}, limit {limit:.4f} rad per tick, "
              f"fastest turn seen {max_rate:.4f} rad per tick, targets reached {settled_ok}/{settled_total}")
        for line in over[:10]:
            print("  over:", line)
        checks = [("agents turned", max_rate > 0.5 * limit), ("never over the limit", not over),
                  ("every target reached before the next", settled_total > 0 and settled_ok == settled_total)]
        failed = [name for name, ok in checks if not ok]
        print("PASS" if not failed else "FAIL " + ", ".join(failed))
        return 0 if not failed else 1


def WaitSpawn(bridge, seconds) -> bool:
    """Reads ticks until a Spawn event of a round arrives (the round that follows a break is running), or the time is out."""
    end = time.time() + seconds
    while time.time() < end:
        published = bridge.wait_tick(2.0)
        if published is None:
            continue
        if any(int(event["Kind"]) == Event.Spawn for event in bridge.new_events(published)):
            return True
    return False


def Preset(args):
    names = [name for name in args.names.split(",") if name]
    if not names:
        print("FAIL --names needs at least one preset")
        return 1
    rounds = args.rounds or len(names)
    with Open(args) as bridge:
        if not WaitRunning(bridge, args.wait):
            print(f"FAIL no match running after {args.wait} s")
            return 1
        bridge.set_control(owner=BridgeOwner.Python, mode=BridgeMode.Free, scale=args.scale, agents=0)
        if not bridge.wait_control(Owner=BridgeOwner.Python):
            print("FAIL Artemis did not hand the instance to Python")
            return 1
        expected = L.BridgeSetupResult.Rejected if args.reject else L.BridgeSetupResult.Applied
        bad = []
        for step in range(rounds):
            name = names[step % len(names)]
            before = bridge.active_preset
            asked = bridge.request_preset(name)
            if asked != L.BridgeSetupResult.Pending:
                bad.append(f"round {step + 1}: {name} answered {asked} in the middle of a round, not Pending")
                break
            if not bridge.end_round():
                bad.append(f"round {step + 1}: the end of the round was not taken")
                break
            result = bridge.wait_setup(args.seconds).result
            active = bridge.active_preset
            print(f"  round {step + 1}: {name} -> {result.name}, active {active!r} (was {before!r})")
            # A match that ended with the request may answer Applied (Ended also happens at game over): the active block carries it to the next start.
            if result != expected and not (result == L.BridgeSetupResult.AtLoad and not args.reject):
                bad.append(f"round {step + 1}: {name} ended as {result.name}, expected {expected.name}")
            if args.reject and active != before:
                bad.append(f"round {step + 1}: a rejected preset changed the active one to {active!r}")
            if not args.reject and result == expected and active != name:
                bad.append(f"round {step + 1}: active preset {active!r}, expected {name!r}")
            if not WaitSpawn(bridge, args.seconds):
                bad.append(f"round {step + 1}: no round started after the break")
                break
        bridge.set_control(owner=BridgeOwner.UI, scale=0.0)
        for line in bad[:10]:
            print("  bad:", line)
        checks = [("every request answered as expected", not bad)]
        failed = [name for name, ok in checks if not ok]
        print("PASS" if not failed else "FAIL " + ", ".join(failed))
        return 0 if not failed else 1


def ParseRoster(text):
    """The roster of the option --roster: "0:3,1:4,auto:2" is 3 bots of team 0, 4 of team 1 and 2 automatic ones."""
    roster = {}
    for item in text.split(","):
        key, _, count = item.partition(":")
        roster[key.strip()] = int(count)
    return roster


def Setup(args):
    if args.preset is None and args.map is None and not args.roster:
        print("FAIL setup needs at least one of --preset, --map and --roster")
        return 1
    roster = ParseRoster(args.roster) if args.roster else None
    with Open(args) as bridge:
        if not WaitRunning(bridge, args.wait):
            print(f"FAIL no match running after {args.wait} s")
            return 1
        bridge.set_control(owner=BridgeOwner.Python, mode=BridgeMode.Free, scale=args.scale, agents=0)
        if not bridge.wait_control(Owner=BridgeOwner.Python):
            print("FAIL Artemis did not hand the instance to Python")
            return 1
        bad = []
        before = (bridge.active_preset, bridge.active_map, bridge.active_roster)
        asked = bridge.request_setup(preset=args.preset, map=args.map, roster=roster)
        if asked is None or asked.result != L.BridgeSetupResult.Pending:
            bad.append(f"the request answered {asked} in the middle of a round, not Pending")
        elif not bridge.end_round():
            bad.append("the end of the round was not taken")
        else:
            final = bridge.wait_setup(args.seconds)
            active = (bridge.active_preset, bridge.active_map, bridge.active_roster)
            print(f"  setup -> {final.result.name}, round {final.round}, reason {final.reason!r}")
            print(f"  active: preset {active[0]!r}, map {active[1]!r}, roster {active[2]} (was {before})")
            if args.reject:
                if final.result != L.BridgeSetupResult.Rejected or not final.reason:
                    bad.append(f"ended as {final.result.name} with reason {final.reason!r}, expected Rejected with a reason")
                if active != before:
                    bad.append("a rejected setup changed the active one")
            elif final.result not in (L.BridgeSetupResult.Applied, L.BridgeSetupResult.AtLoad):
                bad.append(f"ended as {final.result.name}, expected Applied")
            elif final.result == L.BridgeSetupResult.Applied:
                if args.preset is not None and active[0] != args.preset:
                    bad.append(f"active preset {active[0]!r}, expected {args.preset!r}")
                if args.map is not None and active[1] != args.map:
                    bad.append(f"active map {active[1]!r}, expected {args.map!r}")
                if roster is not None and active[2] != {k: v for k, v in roster.items() if v}:
                    bad.append(f"active roster {active[2]}, expected {roster}")
            if not WaitSpawn(bridge, args.seconds):
                bad.append("no round started after the break")
        bridge.set_control(owner=BridgeOwner.UI, scale=0.0)
        for line in bad[:10]:
            print("  bad:", line)
        checks = [("the request was answered as expected", not bad)]
        failed = [name for name, ok in checks if not ok]
        print("PASS" if not failed else "FAIL " + ", ".join(failed))
        return 0 if not failed else 1


def StandPositions(published):
    """The positions of the flag stands of a tick, sorted, rounded to 0.1: what tells one layout of the map from another."""
    positions = []
    for stand in published.stands:
        rows = published.entities[published.entities["Handle"] == stand["Handle"]]
        if len(rows):
            positions.append(tuple(round(float(value), 1) for value in rows[0]["WorldOrigin"]))
    return sorted(positions)


def WaitStands(bridge, seconds):
    """The stand positions of the first tick that has any, or None when none came in time."""
    end = time.time() + seconds
    while time.time() < end:
        published = bridge.wait_tick(2.0)
        if published is not None and len(published.stands):
            positions = StandPositions(published)
            if positions:
                return positions
    return None


def Break(bridge, args, name, label):
    """Requests the map variant in the middle of a round, ends the round and waits for the break. Returns (result, active) or None
    with the reason in the second place when the request or the end of the round was not taken."""
    asked = bridge.request_map(name)
    if asked != L.BridgeSetupResult.Pending:
        return None, f"{label}: {name} answered {asked} in the middle of a round, not Pending"
    if not bridge.end_round():
        return None, f"{label}: the end of the round was not taken"
    return bridge.wait_setup(args.seconds).result, bridge.active_map


def MapVariant(args):
    names = [name for name in args.names.split(",") if name]
    if not names:
        print("FAIL --names needs at least one map variant")
        return 1
    if args.reject is True:
        print("FAIL --reject needs the name of a map variant")
        return 1
    rounds = args.rounds or len(names)
    with Open(args) as bridge:
        if not WaitRunning(bridge, args.wait):
            print(f"FAIL no match running after {args.wait} s")
            return 1
        bridge.set_control(owner=BridgeOwner.Python, mode=BridgeMode.Free, scale=args.scale, agents=0)
        if not bridge.wait_control(Owner=BridgeOwner.Python):
            print("FAIL Artemis did not hand the instance to Python")
            return 1
        bad = []
        # The layout the match runs now, to compare the first switch with.
        previous = WaitStands(bridge, args.seconds)
        if previous is None:
            bad.append("no flag stands in the running round")
        if args.reject and not bad:
            # A map variant the loaded map cannot take is Rejected at the break and nothing changes.
            before = bridge.active_map
            result, active = Break(bridge, args, args.reject, "--reject")
            if result is None:
                bad.append(active)
            else:
                print(f"  reject: {args.reject} -> {result.name}, active {active!r} (was {before!r})")
                if result != L.BridgeSetupResult.Rejected:
                    bad.append(f"--reject {args.reject} ended as {result.name}, expected Rejected")
                if active != before:
                    bad.append(f"a rejected map variant changed the active one to {active!r}")
                if not WaitSpawn(bridge, args.seconds):
                    bad.append("no round started after the rejected request")
                else:
                    previous = WaitStands(bridge, args.seconds) or previous
        for step in range(rounds if not bad else 0):
            name = names[step % len(names)]
            before = bridge.active_map
            result, active = Break(bridge, args, name, f"round {step + 1}")
            if result is None:
                bad.append(active)
                break
            print(f"  round {step + 1}: {name} -> {result.name}, active {active!r} (was {before!r})")
            # A match that ended with the request may answer Applied (Ended also happens at game over): the active block carries it to the next start.
            if result not in (L.BridgeSetupResult.Applied, L.BridgeSetupResult.AtLoad):
                bad.append(f"round {step + 1}: {name} ended as {result.name}, expected Applied")
            elif result == L.BridgeSetupResult.Applied and active != name:
                bad.append(f"round {step + 1}: active map {active!r}, expected {name!r}")
            if not WaitSpawn(bridge, args.seconds):
                bad.append(f"round {step + 1}: no round started after the break")
                break
            now = WaitStands(bridge, args.seconds)
            if now is None:
                bad.append(f"round {step + 1}: no flag stands after the break (a map variant without the CTF objects)")
                break
            if name != before and previous is not None and now == previous:
                bad.append(f"round {step + 1}: the stands did not move after {name!r}")
            previous = now
        bridge.set_control(owner=BridgeOwner.UI, scale=0.0)
        for line in bad[:10]:
            print("  bad:", line)
        checks = [("every request answered as expected", not bad)]
        failed = [name for name, ok in checks if not ok]
        print("PASS" if not failed else "FAIL " + ", ".join(failed))
        return 0 if not failed else 1


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", choices=["info", "ticks", "run", "endround", "preset", "map", "setup", "turn", "controls"])
    parser.add_argument("--pid", type=int)
    parser.add_argument("--wait", type=int, default=120, help="seconds to wait for a running match")
    parser.add_argument("--count", type=int, default=60)
    parser.add_argument("--scale", type=float, default=0.0)
    parser.add_argument("--mode", choices=["lockstep", "free"], default="lockstep")
    parser.add_argument("--seconds", type=float, default=None, help="how long to run (endround 300, preset and map 120, the rest 60)")
    parser.add_argument("--agents", choices=["bots", "all"], default="bots")
    parser.add_argument("--period", type=int, default=4)
    parser.add_argument("--sparse", action="store_true", help="run, turn: ask Artemis for its pipeline only on decision ticks (Q101)")
    parser.add_argument("--latency", type=int, default=4)
    parser.add_argument("--crash-after", type=int, default=0)
    parser.add_argument("--pause-every", type=int, default=0, help="run: decisions between two pauses (0 = none)")
    parser.add_argument("--pause-seconds", type=float, default=30.0, help="run: length of each pause")
    parser.add_argument("--names", default="", help="preset: comma separated presets to request, one per round")
    parser.add_argument("--rounds", type=int, default=0, help="preset: rounds to request one in (0 = one per name)")
    parser.add_argument("--reject", nargs="?", const=True, default="",
                        help="preset: the names must be rejected (a flag). map: a map variant (NAME) that must be Rejected, tried first")
    parser.add_argument("--preset", default=None, help="setup: the preset to request (empty = the base values)")
    parser.add_argument("--map", default=None, help="setup: the map variant to request (empty = the one the match loaded with)")
    parser.add_argument("--roster", default="", help="setup: the bots, for example 0:3,1:4,auto:2")
    parser.add_argument("--stale", action="store_true", help="endround: ask for one in the lobby first")
    parser.add_argument("--round-seconds", type=float, default=10.0, help="endround: play before each request")
    parser.add_argument("--yaw-step", type=float, default=YAW_STEP, help="run: radians the yaw turns per decision")
    parser.add_argument("--turn-limit", type=float, default=0.1, help="turn: radians per tick")
    parser.add_argument("--turn-step", type=float, default=2.5, help="turn: radians the target yaw moves each time")
    parser.add_argument("--hold", type=int, default=15, help="turn: decisions each target is held")
    parser.add_argument("--context", choices=sorted(CONTEXTS), default="foot", help="controls: foot, nogren, flag or vehicle")
    parser.add_argument("--imitation", action="store_true", help="controls: send nothing, check the PossessedInput of what the user does")
    parser.add_argument("--phase-seconds", type=float, default=6.0, help="controls: seconds of each phase")
    parser.add_argument("--player", type=int, default=None, help="controls: table index of the agent (default: the first live bot)")
    parser.add_argument("--only", default="", help="controls: phase names to run, comma separated")
    args = parser.parse_args()
    if args.seconds is None:
        args.seconds = {"endround": 300.0, "preset": 120.0, "map": 120.0, "setup": 120.0}.get(args.command, 60.0)
    try:
        return {"info": Info, "ticks": Ticks, "run": Run, "endround": EndRound, "preset": Preset, "map": MapVariant, "setup": Setup, "turn": Turn, "controls": Controls}[args.command](args)
    except BridgeError as error:
        print(f"FAIL {error}")
        return 2


if __name__ == "__main__":
    sys.exit(Main())
