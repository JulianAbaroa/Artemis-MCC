"""
bridge_check.py
Tests of the bridge against a running MCC (D135, Diseno/12 sec. 8: T1-T5). Runs on Windows with the venv of Agent:
python Tools/game.py py Agent/scripts/bridge_check.py <command> [options]

    info                                   The open regions and the control block of the newest one.
    frames [--count 60] [--scale S]        T1/T2: consecutive frames, layout, copies that hit a write, clock of the frames.
    run [--mode lockstep|free] [--seconds 60] [--agents bots|all] [--scale S] [--period N] [--latency D] [--crash-after K]
                                           T3-T5: trivial policy (walk forward, turn --yaw-step rad per decision), the decision each
                                           player had applied (known answer), the aim against the yaw sent, latencies, ticks per
                                           second. --crash-after K exits without closing after K decisions (the client dies).
        [--pause-every K --pause-seconds S]  P.5 (D143): every K decisions holds a pause of S s, alternating after answering the
                                           decision and before answering it (the game waits inside the tick). No tick may run
                                           during a pause and the known answer must hold across it.
    endround [--round-seconds 10] [--seconds 300] [--scale S] [--stale]
                                           D144: asks for the end of each round after --round-seconds of play (from the start, or
                                           the first Spawn after a RoundEnd) and checks that RoundEnd follows, or GameOver in the
                                           last round. Python owns the instance without agents (Free mode). --stale first asks
                                           for one outside a match (start it in the lobby): it must not end the first round.
    turn [--turn-limit 0.1] [--turn-step 2.5] [--hold 15] [--seconds 60] [--scale S]
                                           D114: Lockstep, every bot gets a new target every --hold decisions (yaw +--turn-step,
                                           pitch +-0.4). The aim seen between two frames never turns faster than --turn-limit rad
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
                  "Scale", "FrameCount", "ClockSkips", "LockstepTimeouts", "HandleMismatches"]
        print(f"layout {int(a['LayoutHash']):#018x} (client {L.LAYOUT_HASH:#018x}), version {int(a['LayoutVersion'])}, "
              f"{int(a['RegionSize'])} bytes, heartbeat {int(a['Heartbeat'])}")
        print(" | ".join(f"{name} {a[name]}" for name in fields))
    return 0


def Frames(args):
    with Open(args) as bridge:
        if not WaitRunning(bridge, args.wait):
            print(f"FAIL no match running after {args.wait} s")
            return 1
        if args.scale:
            bridge.set_control(owner=BridgeOwner.Python, mode=BridgeMode.Free, scale=args.scale, agents=0)
        before = bridge.counters()
        period = int(bridge.artemis["DecisionPeriod"])
        frames, gaps, bad, ticks, entities = 0, 0, [], [], []
        events, kinds, game_ticks = 0, {}, []
        start = time.time()
        while frames < args.count and time.time() - start < args.count + 30:
            frame = bridge.wait_frame(2.0)
            if frame is None:
                continue
            frames += 1
            gaps += frame.skipped
            ticks.append(frame.action_tick)
            entities.append(len(frame.entities))
            h = frame.header
            if frame.action_tick % period:
                bad.append(f"tick {frame.action_tick} not a multiple of N {period}")
            if int(h["PlayerCount"]) > L.MAX_PLAYERS or int(h["EntityCount"]) > L.MAX_ENTITIES:
                bad.append(f"counts {int(h['PlayerCount'])}/{int(h['EntityCount'])}")
            handles = frame.entities["Handle"]
            if len(handles) and (np.any(handles == L.NO_HANDLE) or np.any(np.diff(handles.astype(np.int64)) <= 0)):
                bad.append(f"entity handles not sorted or NONE at tick {frame.action_tick}")
            if np.any((frame.players["Handle"] & 0xFFFF) >= L.MAX_PLAYERS):
                bad.append(f"player index out of range at tick {frame.action_tick}")
            fresh = bridge.new_events(frame)
            if len(fresh) > 1 and np.any(np.diff(fresh["Sequence"].astype(np.int64)) != 1):
                bad.append(f"event sequences not consecutive at tick {frame.action_tick}")
            events += len(fresh)
            for kind in fresh["Kind"]:
                name = Event(int(kind)).name if int(kind) in Event._value2member_map_ else str(int(kind))
                kinds[name] = kinds.get(name, 0) + 1
            game_tick = int(frame.match["GameTick"])
            if game_ticks and game_tick < game_ticks[-1]:
                bad.append(f"match game tick went back {game_ticks[-1]} -> {game_tick}")
            game_ticks.append(game_tick)
        elapsed = time.time() - start
        after = bridge.counters()
        steps = np.diff(ticks) if len(ticks) > 1 else np.array([])
        irregular = int(np.sum(steps != period)) if len(steps) else 0
        print(f"frames {frames} in {elapsed:.1f} s, skipped {gaps}, steps != N: {irregular} (with skips), copies retried {bridge.tears}")
        print(f"entities avg {np.mean(entities) if entities else 0:.0f} max {max(entities) if entities else 0}, "
              f"players {len(frame.players) if frames else 0}, clock skips +{after['ClockSkips'] - before['ClockSkips']}")
        phase = MatchPhase(int(frame.match["Phase"])).name if frames else "-"
        print(f"events {events} (lost {bridge.event_gaps}) {kinds}, match ticks {game_ticks[0] if game_ticks else 0}-"
              f"{game_ticks[-1] if game_ticks else 0}, phase {phase}, scores {[int(s) for s in frame.match['PlayerScores'][:4]] if frames else []}")
        for line in bad[:10]:
            print("  bad:", line)
        bridge.set_control(owner=BridgeOwner.UI, scale=0.0)
        ok = frames >= args.count and not bad and after["ClockSkips"] == before["ClockSkips"] and bridge.event_gaps == 0
        print("PASS" if ok else "FAIL")
        return 0 if ok else 1


def Run(args):
    lockstep = args.mode == "lockstep"
    with Open(args) as bridge:
        if not WaitRunning(bridge, args.wait):
            print(f"FAIL no match running after {args.wait} s")
            return 1
        bridge.set_control(owner=BridgeOwner.Python, mode=BridgeMode.Lockstep if lockstep else BridgeMode.Free,
                           period=args.period, latency=args.latency, scale=args.scale, agents=0)
        if not bridge.wait_control(Owner=BridgeOwner.Python, IsClientConnected=1):
            print("FAIL Artemis did not hand the instance over")
            return 1
        period, latency = args.period, args.latency
        before = bridge.counters()
        freq = bridge.qpc_frequency
        mask, yaws, matches = 0, {}, {}
        recv, respond = [], []
        applied_ok = applied_total = aim_ok = aim_total = decisions = 0
        first_pos, last_pos = {}, {}
        pauses, paused_seconds, pause_just_ended = [], 0.0, False
        start = time.time()
        while time.time() - start < args.seconds:
            frame = bridge.wait_frame(2.0)
            if frame is None:
                if not bridge.is_alive:
                    print("Artemis closed the bridge")
                    break
                continue
            # note: A frame published just before a pause is read after it, so it is not a latency.
            if not pause_just_ended:
                recv.append((frame.received_qpc - int(frame.header["PublishQpc"])) / freq * 1000)
            pause_just_ended = False
            tick = frame.action_tick
            stats = matches.setdefault(frame.match_id, {"decisions": 0, "players": 0, "first": tick, "last": tick})
            stats["decisions"] += 1
            stats["last"] = tick
            agents = [p for p in frame.players if args.agents == "all" or int(p["LocalIndex"]) == L.NOT_LOCAL]
            stats["players"] = max(stats["players"], len(agents))
            new_mask = 0
            for p in agents:
                new_mask |= 1 << (int(p["Handle"]) & 0xFFFF)
            entities = {int(e["Handle"]): e for e in frame.entities}
            for p in agents:
                index = int(p["Handle"]) & 0xFFFF
                applied = int(p["AppliedDecision"])
                previous = tick - period
                if (mask >> index) & 1 and previous in yaws:
                    applied_total += 1
                    applied_ok += applied == previous
                if applied in yaws and int(p["HasBody"]) and int(p["IsAlive"]):
                    aim = p["Aim"]
                    seen = math.atan2(float(aim["Y"]), float(aim["X"])) % (2 * math.pi)
                    diff = abs((seen - yaws[applied] + math.pi) % (2 * math.pi) - math.pi)
                    aim_total += 1
                    aim_ok += diff < AIM_TOLERANCE
                body = entities.get(int(p["BipedHandle"]))
                if body is not None and int(p["IsAlive"]):
                    position = np.array([float(body["WorldOrigin"][k]) for k in ("X", "Y", "Z")])
                    key = (frame.match_id, int(p["Handle"]))
                    first_pos.setdefault(key, position)
                    last_pos[key] = max(last_pos.get(key, 0.0), float(np.linalg.norm(position - first_pos[key])))
            if new_mask != mask:
                bridge.set_control(agents=new_mask)
                mask = new_mask
            yaw = ((tick // period) * args.yaw_step) % (2 * math.pi)
            yaws[tick] = yaw
            for old in [t for t in yaws if t < tick - 16 * period]:
                del yaws[old]
            is_pause = bool(args.pause_every) and decisions > 0 and decisions % args.pause_every == 0
            is_answered = len(pauses) % 2 == 0
            if is_pause and not is_answered:
                pause_start = time.time()
                pauses.append(("before answering", tick, HoldPause(bridge, args.pause_seconds)))
                paused_seconds += time.time() - pause_start
                pause_just_ended = True
            bridge.write_actions(tick, {int(p["Handle"]) & 0xFFFF: Action(handle=int(p["Handle"]), yaw=yaw, forward=1.0)
                                        for p in agents})
            if not is_pause:
                respond.append((qpc() - int(frame.header["PublishQpc"])) / freq * 1000)
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
        print(f"known answer: applied = previous decision {applied_ok}/{applied_total}, aim = yaw sent {aim_ok}/{aim_total}, "
              f"farthest from the start min {min(moved) if moved else 0:.1f} m over {len(moved)} lives")
        print(f"latency ms: publish->read p50 {Percentile(recv, 50):.3f} p99 {Percentile(recv, 99):.3f} | publish->answer "
              f"p50 {Percentile(respond, 50):.3f} p99 {Percentile(respond, 99):.3f}")
        print(f"game waits {waits} avg {(delta['LockstepWaitNs'] / waits / 1e6) if waits else 0:.3f} ms, timeouts "
              f"{delta['LockstepTimeouts']}, late {delta['LateActions']}, handle mismatches {delta['HandleMismatches']}, rejected "
              f"{delta['RejectedActions']}, clock skips {delta['ClockSkips']}, frames skipped by the client "
              f"{delta['FrameCount'] - decisions}, copies retried {bridge.tears}")
        print(f"since load: wait max {after['LockstepWaitMaxNs'] / 1e6:.3f} ms, flatten max {after['FlattenMaxNs'] / 1e6:.3f} ms | "
              f"this run: flatten avg "
              f"{(delta['FlattenNs'] / delta['FrameCount'] / 1e6) if delta['FrameCount'] else 0:.3f} ms, paused frames "
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
                       ("no frame skipped", delta["FrameCount"] - decisions <= 1)]
        if aim_total:
            checks.append(("aim = yaw sent >= 95%", aim_ok >= 0.95 * aim_total))
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
        requested, round_start, is_over = None, None, False
        start = time.time()
        while time.time() - start < args.seconds and not is_over:
            frame = bridge.wait_frame(2.0)
            if frame is None:
                continue
            tick = int(frame.match["GameTick"])
            if round_start is None and not rounds:
                round_start = tick
            for event in bridge.new_events(frame):
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


def AimAngles(player):
    """Yaw in [0, 2 pi) and pitch of the aim of a player, in radians."""
    aim = player["Aim"]
    x, y, z = float(aim["X"]), float(aim["Y"]), float(aim["Z"])
    return math.atan2(y, x) % (2 * math.pi), math.atan2(z, math.hypot(x, y))


def AngleGap(a, b):
    return abs((a - b + math.pi) % (2 * math.pi) - math.pi)


def Turn(args):
    limit, period = args.turn_limit, args.period
    with Open(args) as bridge:
        if not WaitRunning(bridge, args.wait):
            print(f"FAIL no match running after {args.wait} s")
            return 1
        bridge.set_control(owner=BridgeOwner.Python, mode=BridgeMode.Lockstep, period=period, latency=args.latency,
                           scale=args.scale, agents=0, turn_limit=limit)
        if not bridge.wait_control(Owner=BridgeOwner.Python, IsClientConnected=1):
            print("FAIL Artemis did not hand the instance over")
            return 1
        targets, last_aim, over = {}, {}, []
        mask = decisions = settled_ok = settled_total = 0
        max_rate = 0.0
        start = time.time()
        while time.time() - start < args.seconds:
            frame = bridge.wait_frame(2.0)
            if frame is None:
                if not bridge.is_alive:
                    print("Artemis closed the bridge")
                    break
                continue
            tick = frame.action_tick
            is_change = decisions > 0 and decisions % args.hold == 0
            actions, new_mask = {}, 0
            for p in frame.players:
                if int(p["LocalIndex"]) != L.NOT_LOCAL:
                    continue
                index = int(p["Handle"]) & 0xFFFF
                new_mask |= 1 << index
                if not (int(p["HasBody"]) and int(p["IsAlive"])):
                    last_aim.pop(index, None)
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
                elif is_change:
                    target_yaw, target_pitch = targets[index]
                    settled_total += 1
                    settled_ok += AngleGap(yaw, target_yaw) < AIM_TOLERANCE and abs(pitch - target_pitch) < AIM_TOLERANCE
                    targets[index] = ((target_yaw + args.turn_step) % (2 * math.pi), -target_pitch if target_pitch else 0.4)
                target_yaw, target_pitch = targets[index]
                actions[index] = Action(handle=int(p["Handle"]), yaw=target_yaw, pitch=target_pitch)
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


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", choices=["info", "frames", "run", "endround", "turn"])
    parser.add_argument("--pid", type=int)
    parser.add_argument("--wait", type=int, default=120, help="seconds to wait for a running match")
    parser.add_argument("--count", type=int, default=60)
    parser.add_argument("--scale", type=float, default=0.0)
    parser.add_argument("--mode", choices=["lockstep", "free"], default="lockstep")
    parser.add_argument("--seconds", type=float, default=60)
    parser.add_argument("--agents", choices=["bots", "all"], default="bots")
    parser.add_argument("--period", type=int, default=4)
    parser.add_argument("--latency", type=int, default=2)
    parser.add_argument("--crash-after", type=int, default=0)
    parser.add_argument("--pause-every", type=int, default=0, help="run: decisions between two pauses (0 = none)")
    parser.add_argument("--pause-seconds", type=float, default=30.0, help="run: length of each pause")
    parser.add_argument("--stale", action="store_true", help="endround: ask for one in the lobby first")
    parser.add_argument("--round-seconds", type=float, default=10.0, help="endround: play before each request")
    parser.add_argument("--yaw-step", type=float, default=YAW_STEP, help="run: radians the yaw turns per decision")
    parser.add_argument("--turn-limit", type=float, default=0.1, help="turn: radians per tick")
    parser.add_argument("--turn-step", type=float, default=2.5, help="turn: radians the target yaw moves each time")
    parser.add_argument("--hold", type=int, default=15, help="turn: decisions each target is held")
    args = parser.parse_args()
    try:
        return {"info": Info, "frames": Frames, "run": Run, "endround": EndRound, "turn": Turn}[args.command](args)
    except BridgeError as error:
        print(f"FAIL {error}")
        return 2


if __name__ == "__main__":
    sys.exit(Main())
