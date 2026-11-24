"""
env_check.py
Checks the stage 0 environment against a running CTF match (Diseno/14 sec. 9.2): plays --episodes rounds with a policy and prints,
per episode, steps, return, how it ended, the events of the agents and the time and score left of the observation (from the Match, P.6)
at its start and its end, and how many decisions had the engine's interaction on GrabObjective (the selection reaches the observation).
Policies: random, or seek (steer toward the enemy flag, or the own stand while carrying it, with the vectors of the observation; walk
forward; F5.1 B: while the interaction is GrabObjective and it carries no flag, stop and hold Use: press and hold the first decision,
still held after; otherwise Use on nothing and Jump taps now and then).
python Tools/game.py py Agent/scripts/env_check.py [--policy seek|random] [--episodes 3] [--max-seconds 90] [--scale 5] [--list FILE [--seed 0]] [--timing]
--timing (F4.9): at the end, per phase of a decision (wait for the tick, copy of read_tick, new_events, observe, answer, and the policy, measured here),
p50 / p95 / max in ms and its share of the Python side (copy + events + observe + answer + policy; the wait is not Python's); the slack of the
Lockstep (publish -> answer against the budget of d ticks at the effective scale, scale 0 taken as 1x) and what the counters of Artemis did
per decision (waits, wait per wait, longest wait, flatten per tick).
Exit code 0 when every episode ended (terminated or truncated) and the observations were finite.
"""
import argparse
import math
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import numpy as np  # noqa: E402

from artemis.bridge import CONTROL_STATES, Bridge, discover  # noqa: E402
from artemis.env import PROFILES, ActionNvec, CtfConfig, CtfEnv  # noqa: E402
from artemis.env.ctf import (ACTION_AT, CARRIES_AT, ENEMY_FLAG_AT, GRAB_OBJECTIVE_AT, MOVE_NVEC, OWN_STAND_AT,  # noqa: E402
                             YAW_STEPS)
from artemis.training import load_list  # noqa: E402


def Seek(obs, config, rng):
    is_carrying = obs[CARRIES_AT] > 0.5
    vector = obs[OWN_STAND_AT] if is_carrying else obs[ENEMY_FLAG_AT]
    error = math.atan2(float(vector[1]), float(vector[0]))
    per_decision = min(config.period, config.latency) * config.turn_limit
    yaw_bin = int(np.argmin([abs(step * per_decision - error) for step in YAW_STEPS]))
    names = PROFILES[config.profile]
    is_grabbing = obs[GRAB_OBJECTIVE_AT] > 0.5 and not is_carrying
    # Every control of the profile on "nothing" (state 0) but Jump, which taps now and then (state 1), and Use while grabbing (below).
    states = [1 if name == "Jump" and rng.random() < 0.05 else 0 for name in names]
    forward = 2
    if is_grabbing and "Use" in names:
        # Stand still and hold Use: 2 (press and hold) if the last action had it up, 3 (still held) if it had it down. The last action is
        # read from its one-hot in the observation, so Seek stays a function of the observation.
        use = names.index("Use")
        start = ACTION_AT + sum(MOVE_NVEC) + len(CONTROL_STATES) * use
        last = int(np.argmax(obs[start:start + len(CONTROL_STATES)]))
        states[use] = 3 if last in (2, 3) else 2
        forward = 1
    return np.array([forward, 1, yaw_bin, 2, *states])


def Random(obs, config, rng):
    return np.array([rng.integers(n) for n in ActionNvec(config.profile)])


def Percentile(values, p):
    return float(np.percentile(values, p)) if len(values) else float("nan")


def TimingReport(env, policy_ns, before, after, config):
    """The lines of --timing from the measurements of the environment (CtfConfig.timing), the time of the policy and the counters of the interval."""
    decisions = len(env.timings["wait"])
    if not decisions:
        return ["timing: no decision measured"]
    python = {phase: env.timings[phase] for phase in ("copy", "events", "observe", "answer")}
    python["policy"] = policy_ns
    total = sum(sum(values) for values in python.values()) or 1
    lines = [f"timing over {decisions} decisions (ms per decision: p50 / p95 / max, share of the Python side):"]

    def Line(name, values, share):
        ms = [v / 1e6 for v in values]
        text = f"  {name:8s} {Percentile(ms, 50):8.3f} {Percentile(ms, 95):8.3f} {max(ms) if ms else float('nan'):8.3f}"
        return text + (f"  {100 * sum(values) / total:5.1f}%" if share else "  (blocked in wait_tick)")
    lines.append(Line("wait", env.timings["wait"], False))
    for name, values in python.items():
        lines.append(Line(name, values, True))
    budget = config.latency / 60.0 / (config.scale if config.scale > 0 else 1.0) * 1000.0
    slack = env.slack_ms
    over = 100.0 * sum(1 for v in slack if v > budget) / len(slack) if slack else float("nan")
    lines.append(f"  slack publish->answer ms p50 {Percentile(slack, 50):.3f} p95 {Percentile(slack, 95):.3f} max {max(slack) if slack else float('nan'):.3f}; "
                 f"budget d = {config.latency} ticks at scale {config.scale or 1:g} = {budget:.3f} ms; {over:.1f}% of the decisions pass it")
    delta = {k: after[k] - before[k] for k in after}
    waits, published = delta["LockstepWaits"], delta["PublishedTicks"]
    lines.append(f"  counters: waits/decision {waits / decisions:.3f}, wait per wait {(delta['LockstepWaitNs'] / waits / 1e6) if waits else 0:.3f} ms, "
                 f"longest wait {after['LockstepWaitMaxNs'] / 1e6:.3f} ms (since load), flatten per tick {(delta['FlattenNs'] / published / 1e6) if published else 0:.3f} ms")
    return lines


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--policy", choices=["seek", "random"], default="seek")
    parser.add_argument("--episodes", type=int, default=3)
    parser.add_argument("--max-seconds", type=float, default=90.0)
    parser.add_argument("--scale", type=float, default=0.0)
    parser.add_argument("--turn-limit", type=float, default=0.1)
    parser.add_argument("--profile", choices=sorted(PROFILES), default="stage0", help="input profile of the policy (Diseno/22 5.3)")
    parser.add_argument("--sparse", action="store_true", help="ask Artemis for its pipeline only on decisions (default: every tick)")
    parser.add_argument("--list", help="training list (Data/Training/<name>.json): every episode starts with its next setup")
    parser.add_argument("--timing", action="store_true", help="measure the phases of each decision and the slack of the Lockstep (F4.9)")
    parser.add_argument("--seed", type=int, default=0, help="seed of a weighted list")
    args = parser.parse_args()
    setups = load_list(args.list, seed=args.seed) if args.list else None
    regions = discover()
    if not regions:
        print("FAIL no open region (is MCC running with Artemis?)")
        return 2
    config = CtfConfig(scale=args.scale, max_seconds=args.max_seconds, turn_limit=args.turn_limit, sparse=args.sparse,
                       profile=args.profile, timing=args.timing)
    policy = Seek if args.policy == "seek" else Random
    rng = np.random.default_rng(0)
    failures = []
    policy_ns = []
    with Bridge(int(regions[0]["pid"])) as bridge:
        env = CtfEnv(bridge, config)
        if not env.take_control():
            print("FAIL Artemis did not hand the instance over")
            return 1
        before = bridge.counters()
        try:
            for episode in range(args.episodes):
                started = time.time()
                observations, _ = env.reset(setup=setups.next_setup() if setups else None)
                returns = {h: 0.0 for h in env.agents}
                events = {h: [] for h in env.agents}
                steps, ending = 0, None
                first = {h: float(o[10]) * 50 for h, o in observations.items()}
                clock = {h: o[19:22].copy() for h, o in observations.items()}
                closest = dict(first)
                grabbing = {h: 0 for h in env.agents}
                while ending is None:
                    decided = time.perf_counter_ns()
                    actions = {h: policy(o, config, rng) for h, o in observations.items()}
                    if args.timing:
                        policy_ns.append(time.perf_counter_ns() - decided)
                    observations, rewards, terminated, truncated, infos = env.step(actions)
                    steps += 1
                    for h in env.agents:
                        grabbing[h] += int(observations[h][GRAB_OBJECTIVE_AT] > 0.5)
                        returns[h] += rewards[h]
                        if observations[h][4] > 0.5 and observations[h][6] < 0.5:
                            closest[h] = min(closest[h], float(observations[h][10]) * 50)
                        events[h] += infos[h]["events"]
                        if not np.all(np.isfinite(observations[h])):
                            failures.append(f"episode {episode}: observation not finite")
                    if any(terminated.values()):
                        ending = "terminated"
                    elif any(truncated.values()):
                        ending = "truncated"
                for h in env.agents:
                    print(f"episode {episode} agent {h:#010x}: {steps} steps, return {returns[h]:+.3f}, {ending}, "
                          f"events {events[h]}, flag at {first[h]:.0f} m, closest {closest[h]:.0f} m, GrabObjective in {grabbing[h]} decisions, "
                          f"{time.time() - started:.1f} s")
                    start, end = clock[h], observations[h][19:22]
                    print(f"  time left {start[0]:.3f} -> {end[0]:.3f}, score left own/enemy {start[1]:.2f}/{start[2]:.2f} -> "
                          f"{end[1]:.2f}/{end[2]:.2f}")
        except (RuntimeError, TimeoutError) as error:
            # The match ended (a team left empty ends it) or the environment gave up waiting: say so and fail, never wait on.
            print(f"FAIL {error}")
            failures.append(str(error))
        finally:
            env.release()
        after = bridge.counters()
    print(f"lost events {bridge.event_gaps}")
    if args.timing:
        print("\n".join(TimingReport(env, policy_ns, before, after, config)))
    for line in failures[:5]:
        print("  bad:", line)
    print("PASS" if not failures else "FAIL")
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(Main())
