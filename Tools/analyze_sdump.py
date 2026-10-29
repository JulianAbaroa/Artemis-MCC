"""
analyze_sdump.py
Reverse engineering over a .sdump (Snapshot recording, D202, D249): which bytes of a region change, when, and how, lined up with the
events of the engine ring. The first run parses the file once into a cache next to it (<file>.cache.npz); later runs read the cache.
Ticks are the engine clock (time globals +0xC).

    python Tools/analyze_sdump.py FILE events [--keys 0x79E0,0x78001B]
        Events of the ring (EventGlobals) in order: GameTick, sequence, key, cause and effect player index, the 0x1C bytes of data.
    python Tools/analyze_sdump.py FILE changes REGION [--type u8|u16|i16|u32|i32|f32] [--min N] [--max N] [--from T] [--to T]
                                                     [--near 0x79F2,0x78001B] [--window W] [--limit L]
        Aligned fields of REGION that change between --min and --max times (default 1..40) inside [--from, --to], with their value
        timeline. --near keeps only the fields whose every change falls within W ticks (default 3) of an event of those keys.
    python Tools/analyze_sdump.py FILE field REGION OFFSET [--type T] [--from T] [--to T]
        Timeline of one field (first value and every change).
    python Tools/analyze_sdump.py FILE timers REGION [--type T] [--from T] [--to T] [--limit L]
        Fields that count down or up by a steady step for at least 60 consecutive changes (timers, clocks).

    python Tools/analyze_sdump.py FILE objects [--from T] [--to T]
        Objects created and removed (handle, KindId, datum size) with their tick.
    python Tools/analyze_sdump.py FILE object HANDLE [--type T] [--min N] [--max N] [--from T] [--to T] [--near KEYS] [--window W]
        Fields of one object datum (same filters as changes). HANDLE in hex.

REGION is a name or index: PlayerArray PlayerMapping Interaction TimeGlobals GameEngineGlobals EventGlobals Placements GameGlobals
(short: gee = GameEngineGlobals, gg = GameGlobals, ev = EventGlobals, time = TimeGlobals, players = PlayerArray).
"""
import argparse, os, struct, sys
try:
    import numpy as np
except ImportError:
    sys.exit("numpy is required: pip install numpy")

REGIONS = ["PlayerArray", "PlayerMapping", "Interaction", "TimeGlobals", "GameEngineGlobals", "EventGlobals", "Placements", "GameGlobals"]
SHORT = {"gee": 4, "gg": 7, "ev": 5, "time": 3, "players": 0, "mapping": 1, "placements": 6, "interaction": 2}
TRACKED = (0, 3, 4, 5, 7)
TYPES = {"u8": "<u1", "i8": "<i1", "u16": "<u2", "i16": "<i2", "u32": "<u4", "i32": "<i4", "f32": "<f4"}
TIME, EVENTS = 3, 5
RING_OFFSET, RING_LAST, ENTRY = 0xDB28, 0x10B30, 0x30

def RegionIndex(name):
    low = name.lower()
    if low in SHORT: return SHORT[low]
    for i, r in enumerate(REGIONS):
        if r.lower() == low: return i
    if name.isdigit() and int(name) < len(REGIONS): return int(name)
    sys.exit(f"ERROR: unknown region {name}")

def Parse(path):
    f = open(path, "rb")
    head = f.read(160)
    if head[:6] != b"ASDUMP": sys.exit("ERROR: not a .sdump")
    version, headerSize = struct.unpack_from("<II", head, 8)
    regionCount = struct.unpack_from("<I", head, 24)[0]
    sizes = struct.unpack_from(f"<{regionCount}I", head, 28)
    f.seek(headerSize)
    cur = [bytearray(s) for s in sizes]
    initial = {}
    changes = {r: ([], [], []) for r in TRACKED}  # record index, byte offset, new byte
    ticks, events, lastSeq = [], [], 0
    rec = 0
    while True:
        marker = f.read(4)
        if len(marker) < 4: break
        if marker == b"SEND": break
        if marker != b"SNAP": sys.exit(f"ERROR: bad marker at record {rec}")
        h = f.read(28)
        if len(h) < 28: break
        dropped, gen, flags, nreg, nobj, nrem = struct.unpack_from("<IQIHHH", h, 0)
        if flags & 1: f.read(136)
        for _ in range(nreg):
            addr, size, nspan, rid = struct.unpack("<QIIB7x", f.read(24))
            if len(cur[rid]) != size: cur[rid] = bytearray(size)
            buf = cur[rid]
            for _ in range(nspan):
                off, ln = struct.unpack("<II", f.read(8))
                data = f.read(ln)
                if rid in TRACKED and rec > 0:
                    old = np.frombuffer(bytes(buf[off:off + ln]), np.uint8)
                    new = np.frombuffer(data, np.uint8)
                    if len(old) == len(new):
                        idx = np.nonzero(old != new)[0]
                        if len(idx):
                            c = changes[rid]
                            c[0].append(np.full(len(idx), rec, np.int32)); c[1].append((idx + off).astype(np.int32)); c[2].append(new[idx])
                buf[off:off + ln] = data
        for _ in range(nobj):
            addr, handle, size, nspan, kind = struct.unpack("<QIIIB3x", f.read(24))
            for _ in range(nspan):
                off, ln = struct.unpack("<II", f.read(8)); f.seek(ln, 1)
        f.seek(4 * nrem, 1)
        if rec == 0:
            for r in TRACKED: initial[r] = np.frombuffer(bytes(cur[r]), np.uint8).copy()
        t = cur[TIME]
        ticks.append(struct.unpack_from("<I", t, 0xC)[0] if len(t) >= 0x10 else 0)
        ev = cur[EVENTS]
        if len(ev) > RING_LAST + 4:
            last = struct.unpack_from("<i", ev, RING_LAST)[0]
            if last < lastSeq: lastSeq = 0
            for seq in range(max(lastSeq + 1, last - 255), last + 1):
                o = RING_OFFSET + (seq & 0xFF) * ENTRY
                key = struct.unpack_from("<I", ev, o)[0]
                es = struct.unpack_from("<I", ev, o + 0x24)[0]
                tick = struct.unpack_from("<I", ev, o + 0x28)[0]
                events.append((tick, es, key, bytes(ev[o + 8:o + 0x24]).hex()))
            lastSeq = max(lastSeq, last)
        rec += 1
    arrays = {"ticks": np.array(ticks, np.int64), "ev_tick": np.array([e[0] for e in events], np.int64),
              "ev_seq": np.array([e[1] for e in events], np.int64), "ev_key": np.array([e[2] for e in events], np.int64),
              "ev_data": np.array([e[3] for e in events])}
    for r in TRACKED:
        c = changes[r]
        arrays[f"r{r}_rec"] = np.concatenate(c[0]) if c[0] else np.zeros(0, np.int32)
        arrays[f"r{r}_off"] = np.concatenate(c[1]) if c[1] else np.zeros(0, np.int32)
        arrays[f"r{r}_val"] = np.concatenate(c[2]) if c[2] else np.zeros(0, np.uint8)
        arrays[f"r{r}_init"] = initial.get(r, np.zeros(0, np.uint8))
    print(f"parsed {rec} records, {len(events)} events", file=sys.stderr)
    return arrays

def ParseObjects(path, want=None):
    """Second pass over the file for the object table: creations/removals, and the byte history of the handle `want`."""
    f = open(path, "rb")
    head = f.read(160)
    f.seek(struct.unpack_from("<I", head, 12)[0])
    alive, life, ticks = {}, [], []
    cur, init, hist, rec, tick = None, None, ([], [], []), 0, 0
    timeBuf = bytearray(0x3C)
    while True:
        marker = f.read(4)
        if marker != b"SNAP": break
        h = f.read(28)
        if len(h) < 28: break
        dropped, gen, flags, nreg, nobj, nrem = struct.unpack_from("<IQIHHH", h, 0)
        if flags & 1: f.read(136)
        for _ in range(nreg):
            addr, size, nspan, rid = struct.unpack("<QIIB7x", f.read(24))
            for _ in range(nspan):
                off, ln = struct.unpack("<II", f.read(8)); data = f.read(ln)
                if rid == TIME: timeBuf[off:off + ln] = data
        tick = struct.unpack_from("<I", timeBuf, 0xC)[0]
        ticks.append(tick)
        changed = set()
        for _ in range(nobj):
            addr, handle, size, nspan, kind = struct.unpack("<QIIIB3x", f.read(24))
            changed.add(handle)
            if handle not in alive:
                life.append((tick, "+", handle, kind, size)); alive[handle] = (kind, size)
            mine = handle == want
            if mine and (cur is None or len(cur) != size): cur = bytearray(size)
            for _ in range(nspan):
                off, ln = struct.unpack("<II", f.read(8)); data = f.read(ln)
                if mine:
                    if init is not None:
                        old = np.frombuffer(bytes(cur[off:off + ln]), np.uint8); new = np.frombuffer(data, np.uint8)
                        idx = np.nonzero(old != new)[0]
                        if len(idx):
                            hist[0].append(np.full(len(idx), rec, np.int32)); hist[1].append((idx + off).astype(np.int32)); hist[2].append(new[idx])
                    cur[off:off + ln] = data
            if mine and init is None: init = np.frombuffer(bytes(cur), np.uint8).copy()
        for _ in range(nrem):
            handle = struct.unpack("<I", f.read(4))[0]
            if handle in alive:
                kind, size = alive.pop(handle); life.append((tick, "-", handle, kind, size))
        rec += 1
    d = {"ticks": np.array(ticks, np.int64)}
    if want is not None:
        d["r99_init"] = init if init is not None else np.zeros(0, np.uint8)
        d["r99_rec"] = np.concatenate(hist[0]) if hist[0] else np.zeros(0, np.int32)
        d["r99_off"] = np.concatenate(hist[1]) if hist[1] else np.zeros(0, np.int32)
        d["r99_val"] = np.concatenate(hist[2]) if hist[2] else np.zeros(0, np.uint8)
    return d, life

def Load(path):
    cache = path + ".cache.npz"
    if not os.path.exists(cache) or os.path.getmtime(cache) < os.path.getmtime(path):
        np.savez(cache, **Parse(path))
    return dict(np.load(cache))

class Region:
    """Byte history of one region: rebuilds the value of any field at every record where it changed."""
    def __init__(self, d, r):
        self.ticks = d["ticks"]; self.init = d[f"r{r}_init"]
        rec, off, val = d[f"r{r}_rec"], d[f"r{r}_off"], d[f"r{r}_val"]
        order = np.lexsort((rec, off))
        self.rec, self.off, self.val = rec[order], off[order], val[order]
        self.starts = np.searchsorted(self.off, np.arange(len(self.init) + 1))

    def Timeline(self, offset, size, dtype):
        """List of (record, value): the first record and every record where the field changed."""
        recs = set()
        for b in range(offset, offset + size):
            recs.update(self.rec[self.starts[b]:self.starts[b + 1]].tolist())
        cur = bytearray(self.init[offset:offset + size].tobytes())
        out = [(0, np.frombuffer(bytes(cur), dtype)[0])]
        per = {b: (self.rec[self.starts[b]:self.starts[b + 1]], self.val[self.starts[b]:self.starts[b + 1]]) for b in range(offset, offset + size)}
        pos = {b: 0 for b in per}
        for rc in sorted(recs):
            for b, (rs, vs) in per.items():
                while pos[b] < len(rs) and rs[pos[b]] <= rc:
                    cur[b - offset] = vs[pos[b]]; pos[b] += 1
            v = np.frombuffer(bytes(cur), dtype)[0]
            if v != out[-1][1] or (isinstance(v, np.floating) and np.isnan(v)): out.append((rc, v))
        return out

    def ChangedFields(self, size):
        """Aligned field offsets with at least one changed byte."""
        offs = np.unique(self.off // size * size)
        return offs.tolist()

def Fmt(v):
    if isinstance(v, np.floating): return f"{float(v):.4g}"
    return str(int(v))

def Window(tl, ticks, lo, hi):
    return [(rc, v) for rc, v in tl if lo <= ticks[rc] <= hi]

def Main():
    p = argparse.ArgumentParser(add_help=True, description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("file"); p.add_argument("cmd"); p.add_argument("args", nargs="*")
    p.add_argument("--type", default="u32"); p.add_argument("--min", type=int, default=1); p.add_argument("--max", type=int, default=40)
    p.add_argument("--from", dest="lo", type=int, default=0); p.add_argument("--to", dest="hi", type=int, default=1 << 40)
    p.add_argument("--near", default=""); p.add_argument("--window", type=int, default=3); p.add_argument("--limit", type=int, default=60)
    p.add_argument("--keys", default="")
    a = p.parse_args()
    d = Load(a.file)
    ticks = d["ticks"]
    if a.cmd == "events":
        keys = {int(k, 16) for k in a.keys.split(",") if k}
        n = 0
        for t, s, k, data in zip(d["ev_tick"], d["ev_seq"], d["ev_key"], d["ev_data"]):
            if keys and int(k) not in keys: continue
            if not (a.lo <= t <= a.hi): continue
            raw = bytes.fromhex(str(data))
            print(f"{int(t):7d} #{int(s):4d} {int(k):#09x} cause={raw[0x10]:3d} effect={raw[0x11]:3d} {raw.hex()}")
            n += 1
            if n >= a.limit * 10: print("..."); break
        return
    if a.cmd == "objects":
        _, life = ParseObjects(a.file)
        for t, sign, handle, kind, size in life:
            if a.lo <= t <= a.hi: print(f"{t:7d} {sign} {handle:#010x} kind {kind} size {size:#x}")
        return
    dtype = np.dtype(TYPES[a.type]); size = dtype.itemsize
    if a.cmd == "object":
        od, _ = ParseObjects(a.file, int(a.args[0], 16))
        od["ticks"] = od["ticks"]
        reg = Region(od, 99); a.cmd = "changes"; ticks = od["ticks"]
    else:
        reg = Region(d, RegionIndex(a.args[0]))
    near = [int(k, 16) for k in a.near.split(",") if k]
    nearTicks = np.sort(d["ev_tick"][np.isin(d["ev_key"], near)]) if near else None
    def Show(off, tl):
        vals = " ".join(f"{int(ticks[rc])}:{Fmt(v)}" for rc, v in tl[:24])
        more = f" ... ({len(tl)} values)" if len(tl) > 24 else ""
        print(f"+{off:#07x} {vals}{more}")
    if a.cmd == "field":
        off = int(a.args[1], 16)
        Show(off, [x for x in reg.Timeline(off, size, dtype) if x[0] == 0 or a.lo <= ticks[x[0]] <= a.hi])
        return
    shown = 0
    for off in reg.ChangedFields(size):
        tl = reg.Timeline(off, size, dtype)
        inside = [x for x in tl[1:] if a.lo <= ticks[x[0]] <= a.hi]
        if a.cmd == "changes":
            if not (a.min <= len(inside) <= a.max): continue
            if nearTicks is not None:
                ts = np.array([ticks[rc] for rc, _ in inside])
                i = np.searchsorted(nearTicks, ts)
                dist = np.minimum(np.abs(ts - nearTicks[np.clip(i, 0, len(nearTicks) - 1)]), np.abs(ts - nearTicks[np.clip(i - 1, 0, len(nearTicks) - 1)]))
                if dist.max() > a.window: continue
            Show(off, tl[:1] + inside)
        elif a.cmd == "timers":
            if len(inside) < 60: continue
            vs = np.array([float(v) for _, v in inside]); dv = np.diff(vs)
            best, run, cur = 0, 0, None
            for x in dv:
                if x != 0 and x == cur: run += 1
                else: run, cur = 1, x
                best = max(best, run)
            if best < 60: continue
            print(f"+{off:#07x} changes {len(inside)} longest steady run {best}, range {Fmt(vs.min())}..{Fmt(vs.max())}")
        else:
            sys.exit(f"ERROR: unknown command {a.cmd}")
        shown += 1
        if shown >= a.limit: print(f"... (limit {a.limit})"); break

if __name__ == "__main__":
    Main()
