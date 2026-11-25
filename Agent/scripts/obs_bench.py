"""
obs_bench.py
Offline measurement of the Python side of a decision (F4.9, D317 B, R1 of Diseno/19), with no MCC: takes real ticks of a golden .tdump (v7)
and builds them as a BridgeTick buffer with the dtype of bridge_layout, then times in microseconds (p50 / p95 over N repetitions):
  copy_full    the copy of a whole BridgeTick (what Bridge.read_tick does)
  copy_small   what Bridge.read_tick_view copies (P.3 B, D374): Header + Players + Match + Objectives + Stands + Events; the Views stay in place
  copy_b       the copy of only Header + Players + Match + Objectives + Stands + Events + Views (what would be left with D317 B)
  copy_views   the copy of only Header + Views + Match
  observeN     ObserveBatch of the agents whose View is computed in the tick (the golden has as many as it has)
  observe16    ObserveBatch (P.3, one numpy pass, no loop per agent) over 16 real rows: the computed Views of the golden repeated up to 16 (by indexing)
  observe1     Observe of one row (the wrapper over ObserveBatch): the floor of numpy per call
  new_events   Bridge.new_events over a tick with 64 events
The last line says how much of the Python work per decision the full copy is. Nothing here is an assertion: it is a measurement.
python Agent/scripts/obs_bench.py [--film ctf_c | --golden PATH] [--reps 300]
"""
import argparse
import os
import sys
import time
import types

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "Agent"))
sys.path.insert(0, os.path.join(ROOT, "Tools"))

import numpy as np  # noqa: E402

import tickdump  # noqa: E402
from artemis import bridge_layout as L  # noqa: E402
from artemis.bridge import Bridge, PublishedTick  # noqa: E402
from artemis.env import ActionNvec  # noqa: E402
from artemis.env.ctf import Observe, ObserveBatch  # noqa: E402

AGENTS = 16
EVENTS = 64
REPETITIONS = 300
GROUPS = {
    "copy_small": ("Header", "Players", "Match", "Objectives", "Stands", "Events"),
    "copy_b": ("Header", "Players", "Match", "Objectives", "Stands", "Events", "Views"),
    "copy_views": ("Header", "Views", "Match"),
}


def Percentile(values, p):
    return float(np.percentile(values, p))


def BuildTick(record, table):
    """A BridgeTick buffer (one row) with what the record of the golden has: the header, the players, the entities, the match, the objectives,
    the stands, the weapons, projectiles, explosives, lifts and the Views of the rows it has."""
    buffer = np.zeros(1, L.BridgeTick)
    tick = buffer[0]
    tick["Header"] = record.header
    tick["Match"] = record.match
    for name, rows in (("Players", record.players), ("Entities", table), ("Objectives", record.objectives), ("Stands", record.stands),
                       ("Weapons", record.weapons), ("Projectiles", record.projectiles), ("Explosives", record.explosives),
                       ("Lifts", record.lifts), ("Views", record.views)):
        tick[name][:len(rows)] = rows
    return buffer


def Pick(dump):
    """(record, entities) with the most computed Views (the first of them)."""
    best, best_count = None, -1
    for record, table in dump.states():
        count = int(np.count_nonzero(record.views["Head"]["IsComputed"]))
        if count > best_count:
            best, best_count = (record, table.copy()), count
    return best


def Measure(function, reps):
    """Microseconds of each of reps calls, after a warm-up of a tenth of them."""
    for _ in range(max(reps // 10, 3)):
        function()
    times = []
    for _ in range(reps):
        started = time.perf_counter_ns()
        function()
        times.append((time.perf_counter_ns() - started) / 1e3)
    return times


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--film", default="ctf_c", help="film of Ignore/Regresion whose golden.tdump gives the ticks")
    parser.add_argument("--golden", help="a .tdump (v7) instead of the golden of the film")
    parser.add_argument("--reps", type=int, default=REPETITIONS)
    args = parser.parse_args()
    path = args.golden or os.path.join(ROOT, "Ignore", "Regresion", args.film, "golden.tdump")
    if not os.path.isfile(path):
        print(f"FAIL no {path}")
        return 2
    dump = tickdump.Dump(path)
    if dump.version < 7:
        print(f"FAIL {path} is format version {dump.version}: the Views come with 7")
        return 2
    record, table = Pick(dump)
    buffer = BuildTick(record, table)
    rows = [i for i in range(len(record.views)) if int(record.views["Head"]["IsComputed"][i])]
    print(f"{path}: tick {record.game_tick}, {len(record.players)} players, {len(table)} entities, {len(rows)} computed Views, "
          f"BridgeTick {buffer.itemsize} bytes, {args.reps} repetitions")
    if not rows:
        print("FAIL the golden has no computed View")
        return 2

    results = {"copy_full": Measure(lambda: buffer.copy()[0], args.reps)}
    for name, fields in GROUPS.items():
        results[name] = Measure(lambda fields=fields: [buffer[0][f].copy() for f in fields], args.reps)

    # ObserveBatch over the Views the tick has, over 16 rows (the computed ones repeated: np.resize), and Observe of one row.
    nvec = ActionNvec("stage0")
    published = PublishedTick(buffer[0].copy(), 0)
    views = published.views[rows]
    views16 = views[np.arange(AGENTS) % len(views)]  # not np.resize: it drops the padding of the dtype
    last = np.zeros((len(views), len(nvec)), dtype=np.int64)
    last16 = np.zeros((AGENTS, len(nvec)), dtype=np.int64)
    first = published.views[rows[0]]
    results[f"observe{len(rows)}"] = Measure(lambda: ObserveBatch(views, published.match, last, nvec, 120.0), args.reps)
    results["observe16"] = Measure(lambda: ObserveBatch(views16, published.match, last16, nvec, 120.0), args.reps)
    results["observe1"] = Measure(lambda: Observe(first, published.match, last[0], nvec, 120.0), args.reps)

    # new_events over a tick with 64 events: fresh state each time (the cursor starts over with the match).
    events = buffer.copy()
    events["Header"]["EventCount"] = EVENTS
    events["Header"]["LastEventSequence"] = EVENTS
    events["Events"][0]["Sequence"][:EVENTS] = np.arange(1, EVENTS + 1)
    event_tick = PublishedTick(events[0], 0)

    def NewEvents():
        stub = types.SimpleNamespace(_event_match=None, event_cursor=0, event_gaps=0)
        return Bridge.new_events(stub, event_tick)
    results["new_events"] = Measure(NewEvents, args.reps)

    print(f"{'what':12s} {'p50 us':>10s} {'p95 us':>10s}")
    for name, times in results.items():
        note = f"  ({len(rows)} computed Views repeated up to {AGENTS})" if name == "observe16" else ""
        print(f"{name:12s} {Percentile(times, 50):10.1f} {Percentile(times, 95):10.1f}{note}")
    full, ev, obs = (Percentile(results[k], 50) for k in ("copy_full", "new_events", "observe16"))
    print(f"copy_full = {100 * full / (full + ev + obs):.1f}% de (copy_full + new_events + observe16)  "
          f"[{full:.1f} / ({full:.1f} + {ev:.1f} + {obs:.1f}) us, p50]")
    return 0


if __name__ == "__main__":
    sys.exit(Main())
