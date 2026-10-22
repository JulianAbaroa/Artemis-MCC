"""
env_check.py
Checks the stage 0 environment against a running CTF match (Diseno/14 sec. 9.2): plays --episodes rounds with a policy and prints,
per episode, steps, return, how it ended and the events of the agents. Policies: random, or seek (steer toward the enemy flag, or
the own stand while carrying it, with the vectors of the observation; walk forward).
python Tools/game.py py Agent/scripts/env_check.py [--policy seek|random] [--episodes 3] [--max-seconds 90] [--scale 5]
Exit code 0 when every episode ended (terminated or truncated) and the observations were finite.
"""
import argparse
import math
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import numpy as np  # noqa: E402

from artemis.bridge import Bridge, discover  # noqa: E402
from artemis.env import ACTION_NVEC, CtfConfig, CtfEnv  # noqa: E402
from artemis.env.ctf import YAW_STEPS  # noqa: E402


def Seek(obs, config, rng):
    vector = obs[14:16] if obs[6] > 0.5 else obs[7:9]
    error = math.atan2(float(vector[1]), float(vector[0]))
    per_decision = config.period * config.turn_limit
    yaw_bin = int(np.argmin([abs(step * per_decision - error) for step in YAW_STEPS]))
    jump = int(rng.random() < 0.05)
    return np.array([2, 1, yaw_bin, 2, jump, 0])


def Random(obs, config, rng):
    return np.array([rng.integers(n) for n in ACTION_NVEC])


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--policy", choices=["seek", "random"], default="seek")
    parser.add_argument("--episodes", type=int, default=3)
    parser.add_argument("--max-seconds", type=float, default=90.0)
    parser.add_argument("--scale", type=float, default=0.0)
    parser.add_argument("--turn-limit", type=float, default=0.1)
    args = parser.parse_args()
    regions = discover()
    if not regions:
        print("FAIL no open region (is MCC running with Artemis?)")
        return 2
    config = CtfConfig(scale=args.scale, max_seconds=args.max_seconds, turn_limit=args.turn_limit)
    policy = Seek if args.policy == "seek" else Random
    rng = np.random.default_rng(0)
    failures = []
    with Bridge(int(regions[0]["pid"])) as bridge:
        env = CtfEnv(bridge, config)
        if not env.take_control():
            print("FAIL Artemis did not hand the instance over")
            return 1
        try:
            for episode in range(args.episodes):
                started = time.time()
                observations, _ = env.reset()
                returns = {h: 0.0 for h in env.agents}
                events = {h: [] for h in env.agents}
                steps, ending = 0, None
                first = {h: float(o[10]) * 50 for h, o in observations.items()}
                closest = dict(first)
                while ending is None:
                    actions = {h: policy(o, config, rng) for h, o in observations.items()}
                    observations, rewards, terminated, truncated, infos = env.step(actions)
                    steps += 1
                    for h in env.agents:
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
                          f"events {events[h]}, flag at {first[h]:.0f} m, closest {closest[h]:.0f} m, {time.time() - started:.1f} s")
        finally:
            env.release()
    print(f"lost events {bridge.event_gaps}")
    for line in failures[:5]:
        print("  bad:", line)
    print("PASS" if not failures else "FAIL")
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(Main())
