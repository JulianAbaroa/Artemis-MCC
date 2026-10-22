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
        start = time.time()
        while time.time() - start < args.seconds:
            frame = bridge.wait_frame(2.0)
            if frame is None:
                if not bridge.is_alive:
                    print("Artemis closed the bridge")
                    break
                continue
            recv.append((frame.received_qpc - int(frame.header["PublishQpc"])) / freq * 1000)
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
            bridge.write_actions(tick, {int(p["Handle"]) & 0xFFFF: Action(handle=int(p["Handle"]), yaw=yaw, forward=1.0)
                                        for p in agents})
            respond.append((qpc() - int(frame.header["PublishQpc"])) / freq * 1000)
            decisions += 1
            if args.crash_after and decisions >= args.crash_after:
                print(f"crash: exiting after {decisions} decisions without closing (Lockstep was {lockstep})")
                sys.stdout.flush()
                os._exit(3)
        elapsed = time.time() - start
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
        # note: One clock skip per match seen is tolerated: they come at the start or the end of a match, never inside (BACKLOG).
        checks = [("decisions", decisions > 0), ("clock skips only at match boundaries", delta["ClockSkips"] <= len(matches))]
        if lockstep:
            checks += [("applied = previous decision >= 99%", applied_total and applied_ok >= 0.99 * applied_total),
                       ("no Lockstep timeouts", delta["LockstepTimeouts"] == 0),
                       ("no frame skipped", delta["FrameCount"] - decisions <= 1)]
        if aim_total:
            checks.append(("aim = yaw sent >= 95%", aim_ok >= 0.95 * aim_total))
        failed = [name for name, ok in checks if not ok]
        print("PASS" if not failed else "FAIL " + ", ".join(failed))
        return 0 if not failed else 1


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", choices=["info", "frames", "run"])
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
    parser.add_argument("--yaw-step", type=float, default=YAW_STEP, help="run: radians the yaw turns per decision")
    args = parser.parse_args()
    try:
        return {"info": Info, "frames": Frames, "run": Run}[args.command](args)
    except BridgeError as error:
        print(f"FAIL {error}")
        return 2


if __name__ == "__main__":
    sys.exit(Main())
