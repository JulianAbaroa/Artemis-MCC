"""VW.9a.y (red) probe: what changes in the unit of each player on THIS machine (host or client), to compare bots and humans.

On a client (the secondary PC) the bots move but do not turn or animate. Run this at the same time on the host and on the client,
with the bots in Test action (they walk in circles) and a human moving too; compare the summaries: the words of the unit datum
that change for a human but not for a bot on the client are the replicated state that a bot lacks (facing, aim, animation).
Standalone (ctypes, no Artemis code, nothing written): copy it with variant_poke.py and run it with any Python 3 on Windows.

    net_probe.py SECONDS [MAXWORDS]   samples every player's unit for SECONDS; prints per player the travel, the distinct aim
                                      vectors and the unit words (4 bytes, offset hex) that changed, up to MAXWORDS (default 60)
"""
import math
import os
import struct
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from variant_poke import Proc, find_pid, thread_contexts, tls_index  # noqa: E402

UNIT_BYTES = 0x700


def resolve():
    pid = find_pid()
    if not pid:
        raise SystemExit("MCC not running")
    p = Proc(pid)
    idx = tls_index(p)
    for _, tls in thread_contexts(p, idx):
        try:
            gg = p.u64(tls + 0x48)
            if p.read(gg + 0x10, 1)[0] in (1, 2, 3):
                return p, tls
        except IOError:
            continue
    raise SystemExit("no engine thread with a game mode 1..3 (not in a match?)")


def players(p, tls):
    base = p.u64(p.u64(tls + 0x18) + 0x50)
    out = []
    for i in range(16):
        try:
            d = p.read(base + i * 0x490, 0xD0)
        except IOError:
            break
        handle = struct.unpack_from("<I", d, 0)[0]
        if d[:2] == b"\0\0" or handle == 0xFFFFFFFF:
            continue
        name = d[0xB0:0xD0].decode("utf-16-le", "replace").split("\0")[0]
        alive, dead, current = (struct.unpack_from("<I", d, o)[0] for o in (0x28, 0x2C, 0x34))
        # note: A client may not map a remote player to its biped in +0x28; +0x34 (current biped) is the fallback.
        out.append((i, handle, name, alive if alive != 0xFFFFFFFF else current, (d[4], alive, dead, current)))
    return out


def print_roster(p, tls, title):
    print(f"--- players ({title}): index handle name connection alive(+0x28) dead(+0x2C) current(+0x34)")
    for i, handle, name, _, (conn, alive, dead, current) in players(p, tls):
        print(f"p{i:<2} {handle:08X} {name!r:<18} {conn:02X} {alive:08X} {dead:08X} {current:08X}")


def bipeds(p, tls):
    """Handles of every biped of the object table (entry kind 0), with a player or not."""
    header = p.u64(tls + 0x10)
    used = struct.unpack("<H", p.read(header + 0x44, 2))[0]
    raw = p.read(p.u64(header + 0x50), used * 0x18)
    out = []
    for i in range(used):
        salt, kind = struct.unpack_from("<H2xB", raw, i * 0x18)
        if salt != 0 and kind == 0:
            out.append((salt << 16) | i)
    return out


def unit_bytes(p, tls, unit):
    if unit == 0xFFFFFFFF:
        return None
    try:
        table = p.u64(p.u64(tls + 0x10) + 0x50)
        obj = p.u64(table + 0x10 + (unit & 0xFFFF) * 0x18)
        return p.read(obj, UNIT_BYTES)
    except IOError:
        return None


def main(seconds, maxwords):
    p, tls = resolve()
    stats = {}
    end = time.time() + seconds
    samples = 0
    print_roster(p, tls, "start")
    while time.time() < end:
        try:
            roster = players(p, tls)
        except IOError:
            time.sleep(0.01)
            continue
        owned = {unit for _, _, _, unit, _ in roster}
        try:
            # note: A biped without a player on this machine (a bot seen by a client) is keyed by its unit handle.
            roster += [(99, unit, "(no player)", unit, None) for unit in bipeds(p, tls) if unit not in owned]
        except IOError:
            pass
        for i, handle, name, unit, _ in roster:
            b = unit_bytes(p, tls, unit)
            if b is None:
                continue
            s = stats.setdefault(handle, {"name": name, "index": i, "unit": unit, "first": b, "last": b, "changed": set(),
                                          "aims": set(), "travel": 0.0, "units": set()})
            s["units"].add(unit)
            if unit != s["unit"]:
                s["unit"], s["last"] = unit, b
                continue
            last = s["last"]
            for o in range(0, UNIT_BYTES, 4):
                if b[o:o + 4] != last[o:o + 4]:
                    s["changed"].add(o)
            x0, y0, z0 = struct.unpack_from("<3f", last, 0x20)
            x1, y1, z1 = struct.unpack_from("<3f", b, 0x20)
            s["travel"] += math.dist((x0, y0, z0), (x1, y1, z1))
            ax, ay, az = struct.unpack_from("<3f", b, 0x208)
            s["aims"].add((round(ax, 3), round(ay, 3), round(az, 3)))
            s["last"] = b
        samples += 1
        time.sleep(0.005)
    print_roster(p, tls, "end")
    print(f"--- {samples} samples over {seconds} s, {len(stats)} players with a unit (+0x28, else +0x34)")
    for handle, s in sorted(stats.items(), key=lambda kv: kv[1]["index"]):
        words = sorted(s["changed"])
        shown = " ".join(f"{o:03X}" for o in words[:maxwords]) + (" ..." if len(words) > maxwords else "")
        print(f"p{s['index']:<2} {handle:08X} unit {s['unit']:08X} {s['name']!r:<18} units {len(s['units'])} travel {s['travel']:.2f} wu, "
              f"distinct aim {len(s['aims'])}, changed words {len(words)}")
        print(f"    {shown}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    main(float(sys.argv[1]), int(sys.argv[2]) if len(sys.argv) > 2 else 60)
