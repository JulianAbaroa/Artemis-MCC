"""
probe_ctf.py
Prints what the frame says about CTF (stage 0 design, D146): players (team, local, objective, aim) and the entities of the
objective functions (flags, flag stands, capture zones) with team and position, a few times over a match.
python Tools/game.py py Agent/scripts/probe_ctf.py [--samples 3] [--every 5]
"""
import argparse
import math
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from artemis.bridge import Bridge, discover  # noqa: E402
from artemis.semantic import Function  # noqa: E402

OBJECTIVE = (Function.ObjectiveItem, Function.ObjectiveSpawn, Function.ObjectiveZone)


def Vec(v):
    return f"({float(v['X']):8.2f} {float(v['Y']):8.2f} {float(v['Z']):7.2f})"


def Main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--samples", type=int, default=3)
    parser.add_argument("--every", type=float, default=5.0)
    args = parser.parse_args()
    regions = discover()
    if not regions:
        print("FAIL no region")
        return 1
    with Bridge(int(regions[0]["pid"])) as bridge:
        for sample in range(args.samples):
            frame = bridge.read_frame()
            if frame is None:
                print("no frame")
            else:
                print(f"--- sample {sample} match {frame.match_id} tick {int(frame.match['GameTick'])} phase {int(frame.match['Phase'])}"
                      f" scores {list(frame.match['PlayerScores'][:4])}")
                for p in frame.players:
                    aim = p["Aim"]
                    yaw = math.degrees(math.atan2(float(aim["Y"]), float(aim["X"])))
                    print(f"player {int(p['Handle']):#010x} team {int(p['Team'])}"
                          f" local {int(p['LocalIndex'])} alive {int(p['IsAlive'])} biped {int(p['BipedHandle']):#010x}"
                          f" objective {int(p['Objective']):#010x} yaw {yaw:7.1f}")
                entities = {int(e["Handle"]): e for e in frame.entities}
                for p in frame.players:
                    body = entities.get(int(p["BipedHandle"]))
                    if body is not None:
                        print(f"  biped of {int(p['Handle']):#010x} at {Vec(body['WorldOrigin'])}")
                for e in frame.entities:
                    if int(e["Function"]) in [int(f) for f in OBJECTIVE]:
                        team = int(e["Team"])
                        print(f"entity {int(e['Handle']):#010x} {Function(int(e['Function'])).name}/{int(e['Subtype'])} team {team}"
                              f" lifecycle {int(e['Lifecycle'])} binding {int(e['Binding'])} bound {int(e['BoundTo']):#010x} parent {int(e['ParentHandle']):#010x}"
                              f" radius {float(e['BoundingRadius']):.2f} at {Vec(e['WorldOrigin'])}")
            time.sleep(args.every)
    return 0


if __name__ == "__main__":
    sys.exit(Main())
