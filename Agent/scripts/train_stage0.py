"""
train_stage0.py
Trains the stage 0 agent (D115, D146, D147): PPO on CtfEnv over the running MCC, in Lockstep at a high time scale, with the ticks
paused during each update. Writes Agent/runs/<run>/log.txt and policy.pt (the last policy).
python Tools/game.py py Agent/scripts/train_stage0.py [--updates 20] [--steps 1024] [--scale 10] [--max-seconds 60] --no-wait
"""
import argparse
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from artemis.bridge import Bridge, discover  # noqa: E402
from artemis.env import ACTION_NVEC, OBS_SIZE, CtfConfig, CtfEnv  # noqa: E402
from artemis.train.ppo import Ppo, PpoConfig  # noqa: E402

RUNS = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "runs")


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--updates", type=int, default=20)
    parser.add_argument("--steps", type=int, default=1024, help="decisions per agent per update")
    parser.add_argument("--scale", type=float, default=10.0)
    parser.add_argument("--max-seconds", type=float, default=60.0, help="game seconds before an episode is cut")
    parser.add_argument("--turn-limit", type=float, default=0.1)
    parser.add_argument("--name", default=time.strftime("stage0_%Y%m%d_%H%M%S"))
    args = parser.parse_args()
    regions = discover()
    if not regions:
        print("FAIL no open region (is MCC running with Artemis?)")
        return 2
    folder = os.path.join(RUNS, args.name)
    os.makedirs(folder, exist_ok=True)
    log_file = open(os.path.join(folder, "log.txt"), "a", encoding="ascii", errors="replace")

    def Log(line):
        line = f"{time.strftime('%H:%M:%S')} {line}"
        print(line, flush=True)
        log_file.write(line + "\n")
        log_file.flush()

    Log(f"run {args.name}: {vars(args)}")
    with Bridge(int(regions[0]["pid"])) as bridge:
        env = CtfEnv(bridge, CtfConfig(scale=args.scale, max_seconds=args.max_seconds, turn_limit=args.turn_limit))
        if not env.take_control():
            Log("FAIL Artemis did not hand the instance over")
            return 1
        try:
            ppo = Ppo(env, bridge, OBS_SIZE, ACTION_NVEC, PpoConfig(steps=args.steps), log=Log)
            ppo.train(args.updates, checkpoint=os.path.join(folder, "policy.pt"))
        finally:
            env.release()
            Log(f"end: lost events {bridge.event_gaps}, counters {bridge.counters()}")
    return 0


if __name__ == "__main__":
    sys.exit(Main())
