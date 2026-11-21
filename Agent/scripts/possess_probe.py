"""VW.9c probe: which bytes of the control block of input user 0 change with each kind of input while Watch POV looks at a bot
(temporary, not Artemis code). The block ctrl + 0x180*i (ctrl = *(tls+0xD8), A17 sec. 1, A21 sec. 9.2) is what 0x628b0 copies into
the 0xA8 action of the local player. A beep starts each phase; the person does what the phase says, in order.

    possess_probe.py run [PHASE_SECONDS]    10 phases (default 6 s each) after a 3 s lead-in; prints per phase the words that changed
    possess_probe.py events [PHASE_SECONDS] [FIRST] [LAST]
        input EVENT table of input user 0 (g_InputEventTable_Player0, 0x700 per user; read by 0x5df80 through
        InputState_GetMappedEventData 0xd1070 BEFORE the client turns it into requests against the local unit) and the device state
        block (0x5df80: &DAT_182a03c58 + 0xAB8 * user), per phase of EVENT_PHASES (default 6 s each, phases FIRST..LAST, 1-based).
        Entry of 0x10: +0 u16 ms held, +2 u8 down, +3 flags (bit 0 = blocked), +8 / +0xC binding (0x5d338 / 0x5d358). Mode 1:
        entry of event e = +0x224 + 0x10 * e; mode 0: +0x10 * remap[e], remap = int[0x49] at +0x100. Both are reported.
        Output also goes to Ignore/Temporal/vw9c/events_<time>.txt.
"""
import os
import struct
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pov_probe import resolve  # noqa: E402

try:
    import winsound
except ImportError:
    winsound = None

PHASES = ["quiet", "wasd", "mouse", "fire", "jump+crouch", "switch weapon", "grenade throw+switch", "zoom", "melee+ability",
          "use/pickup"]
SIZE = 0x180


def beep(n=1):
    if winsound:
        for _ in range(n):
            winsound.Beep(1200, 120)


def fmt(v):
    f = struct.unpack("<f", struct.pack("<I", v))[0]
    if v != 0 and 1e-4 < abs(f) < 1e4:
        return f"{v:08X}({f:+.3f})"
    return f"{v:08X}"


def run(phase_seconds, count=len(PHASES)):
    p, tls, gg = resolve()
    ctrl = p.u64(tls + 0xD8)
    mapping = p.u64(tls + 0x158)
    m = p.read(mapping, 0xD0)
    print(f"ctrl={ctrl:#x} local0={struct.unpack_from('<I', m, 4)[0]:08X} view0 player={struct.unpack_from('<I', m, 0xB8)[0]:08X}"
          f" unit={struct.unpack_from('<I', m, 0xC8)[0]:08X}")
    beep(3)
    time.sleep(3)
    base = None
    for name in PHASES[:count]:
        beep(1)
        end = time.time() + phase_seconds
        start = time.time()
        words = {}
        stamps = {}
        samples = 0
        while time.time() < end:
            try:
                b = p.read(ctrl, SIZE)
            except IOError:
                time.sleep(0.005)
                continue
            samples += 1
            w = struct.unpack(f"<{SIZE // 4}I", b)
            if base is None:
                base = w
            for i, v in enumerate(w):
                s = words.setdefault(i, [])
                if not s or s[-1] != v:
                    s.append(v)
                    stamps.setdefault(i, []).append(time.time() - start)
            time.sleep(0.002)
        print(f"--- phase {name} ({samples} samples)")
        for i, seq in sorted(words.items()):
            if len(seq) < 2:
                continue
            if len(seq) <= 8:
                ts = " ".join(f"{t:.1f}s" for t in stamps[i][1:])
                print(f"  +{i * 4:03X}: {len(seq) - 1} changes  " + " ".join(fmt(v) for v in seq) + f"  at {ts}")
            else:
                fs = [struct.unpack('<f', struct.pack('<I', v))[0] for v in seq]
                print(f"  +{i * 4:03X}: {len(seq) - 1} changes  first {fmt(seq[0])} last {fmt(seq[-1])} "
                      f"float min {min(fs):+.3f} max {max(fs):+.3f} distinct {len(set(seq))}")
    beep(3)


TABLE_RVA = 0x287FF94    # g_InputEventTable_Player0 (decompiled 0x18287FF94; 0x0000.c init loop, 0x000d.c remap loop at +0x100)
TABLE_SIZE = 0x700
DEVICE_RVA = 0x2A03C58   # DAT_182a03c58, 0x2AE dwords per input user (0x5df80 -> 0x5dc64 with the control block)
DEVICE_SIZE = 0x2AE * 4
EVENT_COUNT = 0x49
MODE1_BASE = 0x224
EVENT_NAMES = {1: "switch grenade (A17 s6)", 2: "switch weapon (A17 s6)", 3: "use (A17 s8)", 7: "analog 7 (+0x38)",
               9: "zoom (A17 s6)", 10: "analog 10 (+0x3C)", 0x10: "lane 4/5", 0x1E: "vertical up", 0x1F: "vertical down",
               0x28: "zoom (mode 0x715ac)", 0x36: "lane 25", 0x37: "lane 27/29", 0x3C: "zoom +", 0x3D: "zoom -", 0x3E: "lane 16/17"}
EVENT_PHASES = [
    ("quiet", "nothing"),
    ("move", "W, then A, then S, then D"),
    ("look", "mouse left / right, then look at the ceiling and keep pushing up"),
    ("fire", "left click: 3 taps, then hold 1 s"),
    ("jump", "Space x3"),
    ("crouch", "Ctrl x2, then hold 1 s"),
    ("melee", "Q x3"),
    ("ability", "Shift alone x2, then W + Shift"),
    ("throw grenade", "F x2"),
    ("switch weapon", "1 x3"),
    ("switch grenade", "2 x3"),
    ("zoom in", "right click x3, then Z x2"),
    ("zoom out", "X x2"),
    ("use tap", "E x3"),
    ("use hold", "hold E 1.5 s, twice"),
    ("reload", "R x3"),
]


class Tee:
    def __init__(self, path):
        os.makedirs(os.path.dirname(path), exist_ok=True)
        self.f = open(path, "w", newline="\r\n")

    def __call__(self, line=""):
        print(line)
        self.f.write(line + "\n")
        self.f.flush()


ENTRY_OFFSETS = list(range(0, 0x100, 0x10)) + [MODE1_BASE + 0x10 * e for e in range(EVENT_COUNT)]
TAIL = range(MODE1_BASE + 0x10 * EVENT_COUNT, 0x6F8, 4)   # after the last mode 1 entry (axes? +0x6C4.. moved with the mouse)


def entry_of(off):
    """Start of the entry that holds byte off (mode 0 slots from 0, mode 1 entries from +0x224), or None."""
    if off < 0x100:
        return off - off % 0x10
    if MODE1_BASE <= off < MODE1_BASE + 0x10 * EVENT_COUNT:
        return off - (off - MODE1_BASE) % 0x10
    return None


def entry_owner(off, remap):
    """Names the event(s) an entry of the table belongs to: mode 1 by position, mode 0 through the remap."""
    if MODE1_BASE <= off < MODE1_BASE + 0x10 * EVENT_COUNT:
        e = (off - MODE1_BASE) // 0x10
        return f"m1 e{e:02X} {EVENT_NAMES.get(e, '')}".rstrip()
    if off < 0x100:
        slot = off // 0x10
        ids = [e for e in range(EVENT_COUNT) if remap[e] == slot]
        return f"m0 slot {slot} = events " + ",".join(f"{e:02X}" for e in ids)
    return "other"


def events(phase_seconds, first, last):
    p, tls, gg = resolve()
    base, _ = p.module("haloreach.dll")
    table = base + TABLE_RVA
    device = base + DEVICE_RVA
    out = Tee(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "Ignore", "Temporal", "vw9c",
                           time.strftime("events_%Y%m%d_%H%M%S.txt")))
    t0 = p.read(table, TABLE_SIZE)
    remap = struct.unpack_from(f"<{EVENT_COUNT}i", t0, 0x100)
    out(f"haloreach={base:#x} table={table:#x} +6F8={t0[0x6F8]} +6FC={u32(t0, 0x6FC)} device={device:#x}")
    out("remap (event:slot) " + " ".join(f"{e:02X}:{remap[e]}" for e in range(EVENT_COUNT)))
    out("phases: " + " | ".join(f"{i + 1} {n}: {what}" for i, (n, what) in enumerate(EVENT_PHASES)))
    beep(3)
    time.sleep(3)
    for index in range(first - 1, last):
        name, what = EVENT_PHASES[index]
        beep(1)
        start = time.time()
        end = start + phase_seconds
        presses, held, aux, flags = {}, {}, {}, {}
        prev = None
        tword, dword = {}, {}
        samples = 0
        while time.time() < end:
            try:
                t = p.read(table, TABLE_SIZE)
                d = p.read(device, DEVICE_SIZE)
            except IOError:
                time.sleep(0.003)
                continue
            samples += 1
            for off in ENTRY_OFFSETS:
                down = t[off + 2]
                ms = struct.unpack_from("<H", t, off)[0]
                if down and (prev is None or not prev[off + 2]):
                    presses.setdefault(off, []).append(time.time() - start)
                if ms:
                    held[off] = max(held.get(off, 0), ms)
                aux.setdefault(off, set()).add(u32(t, off + 4))
                flags.setdefault(off, set()).add(t[off + 3])
            for i, v in enumerate(struct.unpack(f"<{TABLE_SIZE // 4}I", t)):
                s = tword.setdefault(i, [])
                if not s or s[-1] != v:
                    s.append(v)
            for i, v in enumerate(struct.unpack(f"<{DEVICE_SIZE // 4}I", d)):
                s = dword.setdefault(i, [])
                if not s or s[-1] != v:
                    s.append(v)
            prev = t
            time.sleep(0.002)
        out(f"--- phase {index + 1} {name} ({what}) {samples} samples")
        for off in sorted(set(presses) | set(held)):
            ts = " ".join(f"{x:.1f}s" for x in presses.get(off, [])[:6])
            extra = ""
            if len(aux[off]) > 1:
                fs = [struct.unpack("<f", struct.pack("<I", v))[0] for v in aux[off]]
                extra += f" +4 distinct {len(aux[off])} [{min(fs):+.3f}, {max(fs):+.3f}]"
            if len(flags[off]) > 1:
                extra += f" flags {sorted(flags[off])}"
            out(f"  +{off:03X} {entry_owner(off, remap)}: downs {len(presses.get(off, []))} ({ts}) max ms {held.get(off, 0)}{extra}")
        for i, s in tword.items():
            o = i * 4
            if len(s) < 2 or (entry_of(o) is not None and entry_of(o) in presses or entry_of(o) in held):
                continue
            if 0x100 <= o < MODE1_BASE:
                continue   # remap ints
            fs = [struct.unpack("<f", struct.pack("<I", v))[0] for v in s]
            where = entry_owner(entry_of(o), remap) + f" +{o - entry_of(o):X}" if entry_of(o) is not None else "tail"
            out(f"  word +{o:03X} ({where}): {len(s) - 1} changes first {fmt(s[0])} last {fmt(s[-1])} "
                f"float [{min(fs):+.3f}, {max(fs):+.3f}] distinct {len(set(s))}")
        for i, s in [(i, s) for i, s in dword.items() if len(s) > 1][:16]:
            if len(s) <= 6:
                out(f"  device +{i * 4:03X}: " + " ".join(fmt(v) for v in s))
            else:
                fs = [struct.unpack("<f", struct.pack("<I", v))[0] for v in s]
                out(f"  device +{i * 4:03X}: {len(s) - 1} changes float min {min(fs):+.3f} max {max(fs):+.3f} distinct {len(set(s))}")
    beep(3)


def u32(b, o):
    return struct.unpack_from("<I", b, o)[0]


if __name__ == "__main__":
    if len(sys.argv) >= 2 and sys.argv[1] == "run":
        run(float(sys.argv[2]) if len(sys.argv) > 2 else 6.0, int(sys.argv[3]) if len(sys.argv) > 3 else len(PHASES))
    elif len(sys.argv) >= 2 and sys.argv[1] == "events":
        seconds = float(sys.argv[2]) if len(sys.argv) > 2 else 6.0
        first = int(sys.argv[3]) if len(sys.argv) > 3 else 1
        last = int(sys.argv[4]) if len(sys.argv) > 4 else len(EVENT_PHASES)
        events(seconds, first, last)
    else:
        raise SystemExit(__doc__)
