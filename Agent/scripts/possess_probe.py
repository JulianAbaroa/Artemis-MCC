"""VW.9c probe: which bytes of the control block of input user 0 change with each kind of input while Watch POV looks at a bot
(temporary, not Artemis code). The block ctrl + 0x180*i (ctrl = *(tls+0xD8), A17 sec. 1, A21 sec. 9.2) is what 0x628b0 copies into
the 0xA8 action of the local player. A beep starts each phase; the person does what the phase says, in order.

    possess_probe.py run [PHASE_SECONDS]    10 phases (default 6 s each) after a 3 s lead-in; prints per phase the words that changed
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


if __name__ == "__main__":
    if len(sys.argv) < 2 or sys.argv[1] != "run":
        raise SystemExit(__doc__)
    run(float(sys.argv[2]) if len(sys.argv) > 2 else 6.0, int(sys.argv[3]) if len(sys.argv) > 3 else len(PHASES))
