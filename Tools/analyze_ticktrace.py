"""
analyze_ticktrace.py
Reads the tick traces that the Simulation tab of Artemis records (TickTraces in the storage folder, *.attrace, A8 sec. 10.2):
one record per engine tick with the engine clock, plus the bipeds, vehicles, projectiles and weapons of the object table.

    python Tools/analyze_ticktrace.py FILE summary
        Header, ticks, engine tick range and gaps, speeds seen, real rate, objects per tick.
    python Tools/analyze_ticktrace.py A B diff [--tol 0] [--max 20]
        Compares two traces tick by tick, aligned by engine tick (V1: same film at different speeds must give 0 differences).
        Compares the event sequence and every object field. Speed, tick fraction, generation and real time are not compared.
    python Tools/analyze_ticktrace.py FILE dump TICK
        Prints one engine tick and its objects.
"""
import argparse, math, struct, sys

HEADER = struct.Struct("<8s4I64s32s")
TICK = struct.Struct("<QQIffIII")
OBJECT = struct.Struct("<II4sf18f")
VECTORS = ["BoundingCenter", "WorldOrigin", "Forward", "Up", "LinearVelocity", "AngularVelocity"]
FIELDS = ["ParentHandle", "DamageReceived"] + [f"{v}.{a}" for v in VECTORS for a in "XYZ"]
MAGIC = b"ATTRACE\0"
VERSION = 1
CLOCK_VALID = 1

def Text(raw):
    return raw.split(b"\0", 1)[0].decode("ascii", errors="replace")

def Load(path):
    data = open(path, "rb").read()
    magic, version, tickSize, objectSize, _, mapName, start = HEADER.unpack_from(data, 0)
    if magic != MAGIC:
        sys.exit(f"{path}: not a tick trace")
    if version != VERSION or tickSize != TICK.size or objectSize != OBJECT.size:
        sys.exit(f"{path}: version {version}, tick {tickSize} B, object {objectSize} B; this script reads version {VERSION}, "
                 f"tick {TICK.size} B, object {OBJECT.size} B")
    ticks = []
    offset = HEADER.size
    while offset + TICK.size <= len(data):
        generation, realNs, engineTick, scale, fraction, events, count, flags = TICK.unpack_from(data, offset)
        offset += TICK.size
        if offset + count * OBJECT.size > len(data):
            break
        objects = {}
        for _ in range(count):
            values = OBJECT.unpack_from(data, offset)
            offset += OBJECT.size
            handle, parent, fourcc, damage = values[:4]
            objects[handle] = {"FourCC": fourcc.decode("ascii", errors="replace"),
                               "Fields": (parent, damage) + values[4:]}
        ticks.append({"Generation": generation, "RealNs": realNs, "EngineTick": engineTick, "Scale": scale,
                      "Fraction": fraction, "Events": events, "IsClockValid": bool(flags & CLOCK_VALID), "Objects": objects})
    return {"Path": path, "Map": Text(mapName), "Start": Text(start), "Ticks": ticks}

def Summary(trace):
    ticks = trace["Ticks"]
    print(f"{trace['Path']}\n  map {trace['Map']}, started {trace['Start']}, {len(ticks)} ticks")
    valid = [t for t in ticks if t["IsClockValid"]]
    if len(valid) != len(ticks):
        print(f"  {len(ticks) - len(valid)} ticks without a valid clock")
    if not valid:
        return
    engine = [t["EngineTick"] for t in valid]
    print(f"  engine ticks {engine[0]} .. {engine[-1]}")
    gaps, repeats, backwards = [], 0, 0
    for a, b in zip(engine, engine[1:]):
        if b == a:
            repeats += 1
        elif b < a:
            backwards += 1
        elif b > a + 1:
            gaps.append((a, b))
    print(f"  gaps {len(gaps)} (missing {sum(b - a - 1 for a, b in gaps)} ticks), repeated {repeats}, backwards {backwards}")
    for a, b in gaps[:10]:
        print(f"    {a} -> {b}")
    scales = {}
    for t in valid:
        key = round(t["Scale"], 3)
        scales[key] = scales.get(key, 0) + 1
    print("  speeds " + ", ".join(f"x{k:g}: {v}" for k, v in sorted(scales.items())))
    seconds = (ticks[-1]["RealNs"] - ticks[0]["RealNs"]) / 1e9
    if seconds > 0:
        print(f"  real rate {(len(ticks) - 1) / seconds:.0f} ticks/s over {seconds:.1f} s")
    counts = [len(t["Objects"]) for t in ticks]
    groups = {}
    for t in ticks:
        for o in t["Objects"].values():
            groups[o["FourCC"]] = groups.get(o["FourCC"], 0) + 1
    print(f"  objects per tick {min(counts)} .. {max(counts)}; " + ", ".join(f"{k} {v}" for k, v in sorted(groups.items())))

def ByEngineTick(trace):
    result = {}
    for t in trace["Ticks"]:
        if t["IsClockValid"]:
            result.setdefault(t["EngineTick"], t)
    return result

def Differs(a, b, tol):
    if isinstance(a, float) or isinstance(b, float):
        if math.isnan(a) and math.isnan(b):
            return False
        return abs(a - b) > tol
    return a != b

def Diff(traceA, traceB, tol, limit):
    a, b = ByEngineTick(traceA), ByEngineTick(traceB)
    common = sorted(set(a) & set(b))
    print(f"A {traceA['Path']}: {len(a)} ticks, B {traceB['Path']}: {len(b)} ticks, common {len(common)}")
    onlyA, onlyB = sorted(set(a) - set(b)), sorted(set(b) - set(a))
    if onlyA:
        print(f"  only in A: {len(onlyA)} ({onlyA[0]} .. {onlyA[-1]})")
    if onlyB:
        print(f"  only in B: {len(onlyB)} ({onlyB[0]} .. {onlyB[-1]})")
    shown, total, ticksWithDiff = 0, 0, 0
    for tick in common:
        ta, tb = a[tick], b[tick]
        lines = []
        if ta["Events"] != tb["Events"]:
            lines.append(f"event sequence {ta['Events']} != {tb['Events']}")
        for handle in sorted(set(ta["Objects"]) | set(tb["Objects"])):
            oa, ob = ta["Objects"].get(handle), tb["Objects"].get(handle)
            if oa is None or ob is None:
                which = "A" if ob is None else "B"
                fourcc = (oa or ob)["FourCC"]
                lines.append(f"{handle:08X} {fourcc} only in {which}")
                continue
            for name, va, vb in zip(FIELDS, oa["Fields"], ob["Fields"]):
                if Differs(va, vb, tol):
                    lines.append(f"{handle:08X} {oa['FourCC']} {name}: {va!r} != {vb!r}")
        if lines:
            ticksWithDiff += 1
            total += len(lines)
            for line in lines:
                if shown < limit:
                    print(f"  tick {tick}: {line}")
                    shown += 1
    print(f"differences: {total} in {ticksWithDiff} of {len(common)} common ticks (tolerance {tol})")
    return total

def Dump(trace, engineTick):
    tick = ByEngineTick(trace).get(engineTick)
    if tick is None:
        sys.exit(f"engine tick {engineTick} not in the trace")
    print(f"tick {engineTick}: generation {tick['Generation']}, x{tick['Scale']:g}, fraction {tick['Fraction']:.4f}, "
          f"events {tick['Events']}, {len(tick['Objects'])} objects")
    for handle in sorted(tick["Objects"]):
        o = tick["Objects"][handle]
        f = o["Fields"]
        print(f"  {handle:08X} {o['FourCC']} parent {f[0]:08X} damage {f[1]:g}")
        for i, name in enumerate(VECTORS):
            x, y, z = f[2 + 3 * i: 5 + 3 * i]
            print(f"      {name:16} {x: .6f} {y: .6f} {z: .6f}")

def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("args", nargs="+")
    parser.add_argument("--tol", type=float, default=0.0)
    parser.add_argument("--max", type=int, default=20)
    options = parser.parse_args()
    args = options.args
    if len(args) == 2 and args[1] == "summary":
        Summary(Load(args[0]))
    elif len(args) == 3 and args[2] == "diff":
        sys.exit(1 if Diff(Load(args[0]), Load(args[1]), options.tol, options.max) else 0)
    elif len(args) == 3 and args[1] == "dump":
        Dump(Load(args[0]), int(args[2]))
    else:
        parser.print_help()

if __name__ == "__main__":
    Main()
