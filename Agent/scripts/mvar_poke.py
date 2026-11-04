"""Q98 probe: read the map variant of the running match and replace it with the one of a recorded match config (ctypes, outside MCC).

Runtime map variant R = (*(tls+0x290) + 0x30B) & ~3 (0xD9BC bytes, MapVariant.txt sec. 7); the copy the load starts from is
O = *(tls+0x48) + 0x110F0; a recorded config (<LOCALAPPDATA>/Artemis/MatchConfigs/<name>.bin, 0x2BF30) carries it at +0x1D308.

    mvar_poke.py configs                 map id, version, placements in use and name of the map variant of every recorded config
    mvar_poke.py live                    the same for R and O, and which config each one matches (bytes that differ)
    mvar_poke.py snap NAME               saves R and O to Ignore/Temporal/q98/NAME.{r,o}.bin
    mvar_poke.py diff CONFIG [NAME]      fields that differ between the config and R (or the snap NAME)
    mvar_poke.py load CONFIG [--r-only]  writes the map variant of CONFIG into R (and O unless --r-only); refuses another map id
    mvar_poke.py loadatend CONFIG [SECONDS]  waits (default 160 s) until the round state gee+0xD8 leaves the in-round value, then load
"""
import glob, os, struct, sys, time
from variant_poke import Proc, find_pid, tls_index, thread_contexts

MV_SIZE = 0xD9BC
CFG_SIZE = 0x2BF30
CFG_MV = 0x1D308
CONFIGS = os.path.join(os.environ.get("LOCALAPPDATA", ""), "Artemis", "MatchConfigs")
SNAPS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "Ignore", "Temporal", "q98")
FIELDS = [("header", 0x0, 0x2B0), ("ver/quotas/map", 0x2B0, 0x2B8), ("2b8..2c8", 0x2B8, 0x2C8), ("bounds", 0x2C8, 0x2E0),
          ("budget/flags", 0x2E0, 0x2F4), ("strings", 0x2F4, 0x12F4), ("string count/idx", 0x12F4, 0x14FC),
          ("placements", 0x14FC, 0x14FC + 0x28B * 0x4C), ("quotas", 0xD640, 0xD940), ("d940", 0xD940, MV_SIZE)]

def name_of(mv):
    best = ""
    for start in range(0, 0x2B0, 2):
        s = mv[start:0x2B0].decode("utf-16le", "replace").split("\0")[0]
        if len(s) > len(best) and all(32 <= ord(c) < 127 for c in s): best = s
    return best

def info(mv):
    ver, quotas, mapid = struct.unpack_from("<HHI", mv, 0x2B0)
    used = sum(1 for i in range(0x28B) if mv[0x14FC + i * 0x4C] & 1)
    return f"ver {ver:#x} quotas {quotas} map {mapid} placements {used} \"{name_of(mv)}\""

def configs():
    out = {}
    for path in sorted(glob.glob(os.path.join(CONFIGS, "*.bin"))):
        b = open(path, "rb").read()
        if len(b) == CFG_SIZE: out[os.path.splitext(os.path.basename(path))[0]] = b[CFG_MV:CFG_MV + MV_SIZE]
    return out

def config(name):
    c = configs()
    if name not in c: raise SystemExit(f"no config '{name}': {', '.join(c)}")
    return c[name]

def ndiff(a, b): return sum(1 for x, y in zip(a, b) if x != y)

def fields(a, b):
    for n, s, e in FIELDS:
        d = ndiff(a[s:e], b[s:e])
        if d: print(f"  {n:18} {d} bytes")
    pa = [i for i in range(0x28B) if a[0x14FC + i * 0x4C] & 1]; pb = [i for i in range(0x28B) if b[0x14FC + i * 0x4C] & 1]
    print(f"  placements in use {len(pa)} vs {len(pb)}")

def resolve():
    pid = find_pid()
    if not pid: raise SystemExit("MCC not running")
    p = Proc(pid); idx = tls_index(p)
    for tid, tls in thread_contexts(p, idx):
        try:
            gg = p.u64(tls + 0x48); mode = p.read(gg + 0x10, 1)[0]; blk = p.u64(tls + 0x290)
        except IOError: continue
        if mode == 3 and blk:
            return p, (blk + 0x30B) & ~3, gg + 0x110F0, tid, p.u64(tls + 0x20)
    raise SystemExit("no engine thread in multiplayer (mode 3) with a map variant")

def read(p, a): return b"".join(p.read(a + o, min(0x1000, MV_SIZE - o)) for o in range(0, MV_SIZE, 0x1000))

def main(a):
    cmd = a[0] if a else "live"
    if cmd == "configs":
        for n, mv in configs().items(): print(f"{n}: {info(mv)}")
        return
    if cmd == "diff" and len(a) > 2:
        c = config(a[1]); r = open(os.path.join(SNAPS, a[2] + ".r.bin"), "rb").read(); print(f"{a[1]} vs snap {a[2]}"); fields(c, r); return
    p, R, O, tid, gee = resolve()
    r = read(p, R); o = read(p, O)
    state = lambda: struct.unpack_from("<hh", p.read(gee + 0xD8, 4))
    print(f"pid {p.pid} thread {tid} R {R:#x} O {O:#x} gee state/round {state()}")
    if cmd == "live":
        c = configs()
        for label, mv in (("R", r), ("O", o)):
            best = min(c, key=lambda n: ndiff(c[n], mv)) if c else None
            print(f"{label}: {info(mv)} | closest config '{best}' ({ndiff(c[best], mv) if best else '-'} bytes differ)")
    elif cmd == "snap":
        os.makedirs(SNAPS, exist_ok=True)
        open(os.path.join(SNAPS, a[1] + ".r.bin"), "wb").write(r); open(os.path.join(SNAPS, a[1] + ".o.bin"), "wb").write(o)
        print(f"snap {a[1]} -> {os.path.normpath(SNAPS)}")
    elif cmd == "diff":
        c = config(a[1]); print(f"{a[1]} vs R"); fields(c, r); print(f"{a[1]} vs O"); fields(c, o)
    elif cmd in ("load", "loadatend"):
        c = config(a[1])
        if cmd == "loadatend":
            start = state(); end = time.time() + (float(a[2]) if len(a) > 2 else 160)
            while state()[0] == start[0] and time.time() < end: time.sleep(0.005)
            print(f"{time.strftime('%H:%M:%S')} state {start} -> {state()}")
            if state()[0] == start[0]: raise SystemExit("timeout, nothing written")
        if struct.unpack_from("<I", c, 0x2B4)[0] != struct.unpack_from("<I", r, 0x2B4)[0]: raise SystemExit("different map id: refused")
        print(f"R before: {info(r)}\nnew     : {info(c)}")
        r = read(p, R)
        p.write(R, c[:0x2C8] + r[0x2C8:0x2E0] + c[0x2E0:])  # bounds stay the engine's (FUN_18006c290 recomputes them)
        if "--r-only" not in a: p.write(O, c)
        print(f"R after : {info(read(p, R))}" + ("" if "--r-only" in a else f"\nO after : {info(read(p, O))}"))
    else: print(__doc__)

if __name__ == "__main__":
    main(sys.argv[1:])
