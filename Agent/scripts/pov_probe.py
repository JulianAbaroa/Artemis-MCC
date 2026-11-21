"""VW.9a.y probe: why the action of Artemis does not turn the camera of Watch POV on a bot (temporary, not Artemis code).

Static reading (Ignore/Referencia/Engine/A21_Camara.txt sec. 9.2): the first person camera (FollowFirstPerson 0xc6ee8) takes its
direction from the view entry of the output user (ctrl = *(tls+0xD8); entry = ctrl + 0x680 + 0xB0*o: +0x0 unit, +0x14 yaw,
+0x18 pitch, read by 0x62e00). Two writers: per frame 0x663bc copies the control block of an input user (ctrl + 0x180*i) into the
entries whose unit is its unit; per tick 0x47050 -> 0x62fcc copies the action of the player of each view, but ONLY when
game globals +0x1DA (theater) is set. Prediction: live play + POV on a bot = nobody writes the entry; it keeps the facing that
0x662b4 gave it when the unit was assigned.

    pov_probe.py watch SECONDS    polls view 0, input user 0 and the unit of view 0; prints changes and a summary
    pov_probe.py follow SECONDS   writes the aim of the unit of view 0 into the yaw / pitch of view entry 0 (does the camera follow?)
    pov_probe.py bytes RVA [N]    N bytes (default 48) of haloreach.dll at RVA (hex) and how many times the first 24 appear
"""
import math
import os
import struct
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from variant_poke import Proc, find_pid, thread_contexts, tls_index  # noqa: E402


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
                return p, tls, gg
        except IOError:
            continue
    raise SystemExit("no engine thread with a game mode 1..3 (not in a match?)")


def f32(b, o):
    return struct.unpack_from("<f", b, o)[0]


def u32(b, o):
    return struct.unpack_from("<I", b, o)[0]


def unit_aim(p, tls, unit):
    """Yaw / pitch (rad) of the aim vector of a unit (+0x208, as 0x662b4 reads it), or None."""
    if unit == 0xFFFFFFFF:
        return None
    try:
        table = p.u64(p.u64(tls + 0x10) + 0x50)
        obj = p.u64(table + 0x10 + (unit & 0xFFFF) * 0x18)
        x, y, z = struct.unpack("<3f", p.read(obj + 0x208, 12))
    except IOError:
        return None
    yaw = math.atan2(y, x)
    if yaw < 0:
        yaw += 2 * math.pi
    return yaw, math.atan2(z, math.hypot(x, y))


def sample(p, tls, gg):
    ctrl = p.u64(tls + 0xD8)
    mapping = p.u64(tls + 0x158)
    m = p.read(mapping, 0xD0)
    entry = p.read(ctrl + 0x680, 0x20)
    block = p.read(ctrl + 0x80, 0x20)
    s = {
        "tick": p.u32(p.u64(tls + 0xA0) + 0xC),
        "theater": p.read(gg + 0x1DA, 1)[0],
        "local0": u32(m, 0x04),
        "viewPlayer": u32(m, 0xB8),
        "viewUnit": u32(m, 0xC8),
        "entryUnit": u32(entry, 0x0),
        "entryYaw": f32(entry, 0x14),
        "entryPitch": f32(entry, 0x18),
        "inputUnit": u32(block, 0x0),
        "inputYaw": f32(block, 0x14),
        "inputPitch": f32(block, 0x18),
    }
    aim = unit_aim(p, tls, s["viewUnit"])
    s["unitYaw"], s["unitPitch"] = aim if aim else (float("nan"), float("nan"))
    return s, ctrl


def line(s):
    return (f"t={s['tick']:<7} theater={s['theater']} local0={s['local0']:08X} view0 player={s['viewPlayer']:08X} "
            f"unit={s['viewUnit']:08X} | entry0 unit={s['entryUnit']:08X} yaw={s['entryYaw']:+.3f} pitch={s['entryPitch']:+.3f} | "
            f"input0 unit={s['inputUnit']:08X} yaw={s['inputYaw']:+.3f} pitch={s['inputPitch']:+.3f} | "
            f"unit aim yaw={s['unitYaw']:+.3f} pitch={s['unitPitch']:+.3f}")


def watch(seconds):
    p, tls, gg = resolve()
    end = time.time() + seconds
    first, last, printed = None, None, 0
    distinct = {"entryYaw": set(), "inputYaw": set(), "unitYaw": set()}
    samples = 0
    while time.time() < end:
        try:
            s, _ = sample(p, tls, gg)
        except IOError:
            time.sleep(0.01)
            continue
        samples += 1
        first = first or s
        for k in distinct:
            distinct[k].add(round(s[k], 4))
        keys = ("theater", "viewPlayer", "viewUnit", "entryUnit", "inputUnit")
        moved = last is None or any(s[k] != last[k] for k in keys) or abs(s["entryYaw"] - last["entryYaw"]) > 0.05 \
            or abs(s["unitYaw"] - last["unitYaw"]) > 0.3
        if moved and printed < 40:
            print(line(s))
            printed += 1
            last = s
        time.sleep(0.002)
    print(f"--- {samples} samples over {seconds} s, ticks {first['tick']}..{s['tick']}")
    print(line(s))
    for k, v in distinct.items():
        print(f"distinct {k}: {len(v)}")
    print(f"entry0 unit == view0 unit: {s['entryUnit'] == s['viewUnit']}; input0 unit == view0 unit: {s['inputUnit'] == s['viewUnit']}")


def follow(seconds):
    p, tls, gg = resolve()
    end = time.time() + seconds
    writes = 0
    while time.time() < end:
        try:
            s, ctrl = sample(p, tls, gg)
            if not math.isnan(s["unitYaw"]) and s["entryUnit"] == s["viewUnit"]:
                p.write(ctrl + 0x680 + 0x14, struct.pack("<2f", s["unitYaw"], s["unitPitch"]))
                writes += 1
        except IOError:
            pass
        time.sleep(0.001)
    print(f"follow: {writes} writes over {seconds} s")
    print(line(s))


def dump_bytes(rva_hex, n=48):
    p = Proc(find_pid())
    base, size = p.module("haloreach.dll")
    rva = int(rva_hex, 16)
    code = p.read(base + rva, n)
    img = b""
    for off in range(0, size, 0x100000):
        try:
            img += p.read(base + off, min(0x100000, size - off))
        except IOError:
            img += b"\0" * min(0x100000, size - off)
    print(f"rva {rva:#x}: {code.hex(' ').upper()}")
    print(f"first 24 bytes found {img.count(code[:24])} time(s) in the image")


if __name__ == "__main__":
    if len(sys.argv) < 3 or sys.argv[1] not in ("watch", "follow", "bytes"):
        raise SystemExit(__doc__)
    if sys.argv[1] == "bytes":
        dump_bytes(sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 48)
    else:
        {"watch": watch, "follow": follow}[sys.argv[1]](float(sys.argv[2]))
