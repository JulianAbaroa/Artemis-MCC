"""Q97 probe: read and write the game variant of the running match from outside MCC (ctypes, no Artemis code).

Finds the engine thread context like the DLL does (signature TelemetryIdModifier -> TLS slot; TEB+0x58 of each thread of MCC),
then V = *(tls+0x48) + 0x14E8 (A19_Traits.txt sec. 3) and the players at *(*(tls+0x18)+0x50) + i*0x490 (effective traits +0x328).

    variant_poke.py dump                     general/respawn options, base traits of V and the effective traits of each player
    variant_poke.py get OFF [N]              N bytes (default 1) of V at OFF (hex)
    variant_poke.py set OFF B0 [B1 ...]      write bytes (hex) at V+OFF, then prints dump
    variant_poke.py watch SECONDS            prints the effective gravity/speed/jump of player 0 when they change
    variant_poke.py snap NAME                saves V[0:0x10000] and the 16 player datums to Ignore/Temporal/q97/NAME.*.bin
    variant_poke.py diff A B                 bytes that differ between two snaps (V and players), labelled by field
    variant_poke.py scan [NAME]              copies of the variant options (V+0x2BC..0x3BC, from the live match or from snap NAME)
                                             anywhere in the writable memory of MCC (finds the lobby copy, Q97 b)
    variant_poke.py tick                     Match and the traits of the players from the Tick of Artemis (bridge), Q97 f
    variant_poke.py peek ADDR [N] | poke ADDR B0 [B1 ...]   raw bytes at an absolute address (hex)
"""
import ctypes
import os
import ctypes.wintypes as wt
import re
import struct
import sys
import time

k32 = ctypes.WinDLL("kernel32", use_last_error=True)
ntdll = ctypes.WinDLL("ntdll")
psapi = ctypes.WinDLL("psapi", use_last_error=True)

PROCESS_ALL = 0x0010 | 0x0020 | 0x0008 | 0x0400 | 0x1000
THREAD_QUERY = 0x0040
TH32CS_SNAPTHREAD = 0x4
TH32CS_SNAPPROCESS = 0x2

class THREADENTRY32(ctypes.Structure):
    _fields_ = [("dwSize", wt.DWORD), ("cntUsage", wt.DWORD), ("th32ThreadID", wt.DWORD), ("th32OwnerProcessID", wt.DWORD),
                ("tpBasePri", ctypes.c_long), ("tpDeltaPri", ctypes.c_long), ("dwFlags", wt.DWORD)]

class PROCESSENTRY32W(ctypes.Structure):
    _fields_ = [("dwSize", wt.DWORD), ("cntUsage", wt.DWORD), ("th32ProcessID", wt.DWORD), ("th32DefaultHeapID", ctypes.c_size_t),
                ("th32ModuleID", wt.DWORD), ("cntThreads", wt.DWORD), ("th32ParentProcessID", wt.DWORD),
                ("pcPriClassBase", ctypes.c_long), ("dwFlags", wt.DWORD), ("szExeFile", ctypes.c_wchar * 260)]

class THREAD_BASIC_INFORMATION(ctypes.Structure):
    _fields_ = [("ExitStatus", ctypes.c_long), ("TebBaseAddress", ctypes.c_void_p), ("UniqueProcess", ctypes.c_void_p),
                ("UniqueThread", ctypes.c_void_p), ("AffinityMask", ctypes.c_size_t), ("Priority", ctypes.c_long),
                ("BasePriority", ctypes.c_long)]

class MODULEINFO(ctypes.Structure):
    _fields_ = [("lpBaseOfDll", ctypes.c_void_p), ("SizeOfImage", wt.DWORD), ("EntryPoint", ctypes.c_void_p)]

k32.OpenProcess.restype = wt.HANDLE
k32.OpenThread.restype = wt.HANDLE
k32.CreateToolhelp32Snapshot.restype = wt.HANDLE

def find_pid():
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    e = PROCESSENTRY32W(); e.dwSize = ctypes.sizeof(e)
    ok = k32.Process32FirstW(snap, ctypes.byref(e)); pid = None
    while ok:
        if e.szExeFile.lower().startswith("mcc-win64"): pid = e.th32ProcessID
        ok = k32.Process32NextW(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    return pid

class Proc:
    def __init__(self, pid):
        self.pid = pid
        self.h = k32.OpenProcess(PROCESS_ALL, False, pid)
        if not self.h: raise SystemExit(f"OpenProcess failed {ctypes.get_last_error()}")
    def read(self, addr, n):
        buf = ctypes.create_string_buffer(n); got = ctypes.c_size_t()
        if not k32.ReadProcessMemory(self.h, ctypes.c_void_p(addr), buf, n, ctypes.byref(got)) or got.value != n:
            raise IOError(f"read {addr:#x} {n}")
        return buf.raw
    def write(self, addr, data):
        got = ctypes.c_size_t()
        if not k32.WriteProcessMemory(self.h, ctypes.c_void_p(addr), data, len(data), ctypes.byref(got)) or got.value != len(data):
            raise IOError(f"write {addr:#x} err {ctypes.get_last_error()}")
    def u64(self, a): return struct.unpack("<Q", self.read(a, 8))[0]
    def u32(self, a): return struct.unpack("<I", self.read(a, 4))[0]
    def modules(self):
        mods = (ctypes.c_void_p * 1024)(); need = wt.DWORD(); out = []
        psapi.EnumProcessModulesEx(self.h, mods, ctypes.sizeof(mods), ctypes.byref(need), 3)
        for m in mods[: need.value // 8]:
            nb = ctypes.create_unicode_buffer(260)
            psapi.GetModuleBaseNameW(self.h, ctypes.c_void_p(m), nb, 260)
            out.append((m, 0, nb.value))
        return out
    def module(self, name):
        mods = (ctypes.c_void_p * 1024)(); need = wt.DWORD()
        psapi.EnumProcessModulesEx(self.h, mods, ctypes.sizeof(mods), ctypes.byref(need), 3)
        for m in mods[: need.value // 8]:
            nb = ctypes.create_unicode_buffer(260)
            psapi.GetModuleBaseNameW(self.h, ctypes.c_void_p(m), nb, 260)
            if nb.value.lower() == name:
                mi = MODULEINFO(); psapi.GetModuleInformation(self.h, ctypes.c_void_p(m), ctypes.byref(mi), ctypes.sizeof(mi))
                return mi.lpBaseOfDll, mi.SizeOfImage
        raise SystemExit(f"{name} not loaded")

def tls_index(p):
    base, size = p.module("haloreach.dll")
    img = b""
    for off in range(0, size, 0x100000):
        try: img += p.read(base + off, min(0x100000, size - off))
        except IOError: img += b"\0" * min(0x100000, size - off)
    m = re.search(re.escape(b"\x8B\x0D") + b"(....)" + re.escape(bytes.fromhex("65 48 8B 04 25 58 00 00 00 48 8B 04 C8")), img, re.S)
    if not m: raise SystemExit("signature TelemetryIdModifier not found")
    rel = struct.unpack("<i", m.group(1))[0]
    return p.u32(base + m.start() + 6 + rel)

def thread_contexts(p, idx):
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)
    e = THREADENTRY32(); e.dwSize = ctypes.sizeof(e); out = []
    ok = k32.Thread32First(snap, ctypes.byref(e))
    while ok:
        if e.th32OwnerProcessID == p.pid:
            th = k32.OpenThread(THREAD_QUERY, False, e.th32ThreadID)
            if th:
                tbi = THREAD_BASIC_INFORMATION()
                if ntdll.NtQueryInformationThread(wt.HANDLE(th), 0, ctypes.byref(tbi), ctypes.sizeof(tbi), None) == 0 and tbi.TebBaseAddress:
                    try:
                        arr = p.u64(tbi.TebBaseAddress + 0x58)
                        tls = p.u64(arr + idx * 8) if arr else 0
                        if tls: out.append((e.th32ThreadID, tls))
                    except IOError: pass
                k32.CloseHandle(th)
        ok = k32.Thread32Next(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    return out

def resolve():
    pid = find_pid()
    if not pid: raise SystemExit("MCC not running")
    p = Proc(pid); idx = tls_index(p)
    for tid, tls in thread_contexts(p, idx):
        try:
            gg = p.u64(tls + 0x48)
            mode = p.read(gg + 0x10, 1)[0]
            pl = p.u64(p.u64(tls + 0x18) + 0x50)
        except IOError: continue
        if mode in (1, 2, 3):
            return p, gg + 0x14E8, pl, tid, mode
    raise SystemExit("no engine thread with a game mode 1..3 (not in a match?)")

TRAIT = {"dmgres": 0x00, "health": 0x01, "shield": 0x03, "wdmg": 0x0C, "melee": 0x0D, "nades": 0x10, "infammo": 0x11,
         "prim": 0x15, "sec": 0x16, "aa": 0x17, "speed": 0x18, "grav": 0x19, "veh": 0x1A, "camo": 0x20, "radar": 0x28, "range": 0x29}

def fmt_traits(b):
    jump = struct.unpack_from("<i", b, 0x1C)[0]
    s = " ".join(f"{k}={struct.unpack_from('<b', b, o)[0] if k in ('prim','sec','aa') else b[o]}" for k, o in TRAIT.items())
    return s + f" jump={jump}"

def dump(p, V, pl):
    g = p.read(V + 0x2BC, 0x14)
    print(f"V={V:#x} rounds_to_win={g[1]} time_min={g[2]} round_limit={g[3]} sudden={struct.unpack_from('<h',g,4)[0]}"
          f" grace={struct.unpack_from('<H',g,6)[0]} lives={g[9]} respawn_s={g[0xB]} loadout_cam={g[0xF]} resp_traits_s={g[0x10]}")
    print("base   " + fmt_traits(p.read(V + 0x304, 0x2C)))
    for i in range(16):
        try:
            d = p.read(pl + i * 0x490, 0x360)
        except IOError: break
        if d[:2] == b"\0\0": continue
        print(f"p{i:<2} {d[:2].hex()} " + fmt_traits(d[0x328:0x354]))

SNAPS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "Ignore", "Temporal", "q97")
V_SIZE = 0x10000

# (start, end, label) offsets from V (A19_Traits.txt sec. 3); trait blocks are 0x2C long
LABELS = [(0x2BC, 0x2BD, "general flags"), (0x2BD, 0x2BE, "rounds to win"), (0x2BE, 0x2BF, "time limit min"),
          (0x2BF, 0x2C0, "round limit"), (0x2C0, 0x2C2, "sudden death s"), (0x2C2, 0x2C4, "grace s"),
          (0x2C4, 0x2C5, "respawn flags"), (0x2C5, 0x2C6, "lives"), (0x2C6, 0x2C7, "team lives"), (0x2C7, 0x2C8, "respawn s"),
          (0x2C8, 0x2C9, "suicide pen"), (0x2C9, 0x2CA, "betrayal pen"), (0x2CA, 0x2CB, "respawn growth"),
          (0x2CB, 0x2CC, "loadout cam s"), (0x2CC, 0x2CD, "resp traits s"), (0x2D0, 0x2FC, "respawn traits"),
          (0x2FC, 0x2FE, "social flags"), (0x2FE, 0x300, "team changes"), (0x300, 0x304, "map flags"), (0x304, 0x330, "base traits"),
          (0x330, 0x332, "weapon set"), (0x332, 0x334, "vehicle set"), (0x334, 0x360, "red traits"), (0x360, 0x38C, "blue traits"),
          (0x38C, 0x3B8, "yellow traits"), (0x3B8, 0x3BB, "powerup durations"), (0x3BC, 0x3BD, "species"),
          (0x3BD, 0x3BE, "designator switch")] + \
         [(0x3C0 + t * 0x58, 0x418 + t * 0x58, f"team {t}") for t in range(8)] + \
         [(0x680, 0x681, "loadout flags")] + [(0x681 + i * 30, 0x681 + (i + 1) * 30, f"loadout palette {i}") for i in range(6)] + \
         [(0x738, 0x73A, "team scoring"), (0x4 + 0x5DC4, 0x4 + 0x5DC8, "megalo trait set count")] + \
         [(0x5DD0 + i * 0x30, 0x5E00 + i * 0x30, f"megalo trait set {i}") for i in range(16)] + \
         [(0x4 + 0xE700, 0xF840, "megalo user options?"), (0xF840, 0xF842, "score to win")]

def label(off):
    for a, b, n in LABELS:
        if a <= off < b: return f"{n}+{off - a:#x}"
    return "?"

def read_chunked(p, addr, n, chunk=0x1000):
    out = bytearray()
    for o in range(0, n, chunk):
        try: out += p.read(addr + o, min(chunk, n - o))
        except IOError: out += b"\xEE" * min(chunk, n - o)
    return bytes(out)

def snap(p, V, pl, name):
    os.makedirs(SNAPS, exist_ok=True)
    v = read_chunked(p, V, V_SIZE); players = read_chunked(p, pl, 16 * 0x490)
    open(os.path.join(SNAPS, name + ".v.bin"), "wb").write(v)
    open(os.path.join(SNAPS, name + ".players.bin"), "wb").write(players)
    print(f"snap {name}: V {len(v):#x} bytes at {V:#x}, players at {pl:#x} -> {os.path.normpath(SNAPS)}")

def load(name, kind):
    return open(os.path.join(SNAPS, f"{name}.{kind}.bin"), "rb").read()

def runs(a, b):
    i, n = 0, min(len(a), len(b))
    while i < n:
        if a[i] != b[i]:
            j = i
            while j < n and a[j] != b[j] and j - i < 32: j += 1
            yield i, j
            i = j
        else: i += 1

def diff(an, bn):
    for kind in ("v", "players"):
        a, b = load(an, kind), load(bn, kind); shown = 0; total = 0
        for i, j in runs(a, b):
            total += j - i
            if shown < 120:
                where = label(i) if kind == "v" else f"player {i // 0x490} +{i % 0x490:#x}"
                print(f"{kind:7} +{i:#06x} {where:28} {a[i:j].hex(' ')} -> {b[i:j].hex(' ')}"); shown += 1
        print(f"{kind}: {total} bytes differ" + (" (first 120 runs shown)" if shown >= 120 else ""))

class MBI(ctypes.Structure):
    _fields_ = [("BaseAddress", ctypes.c_void_p), ("AllocationBase", ctypes.c_void_p), ("AllocationProtect", wt.DWORD),
                ("PartitionId", wt.WORD), ("RegionSize", ctypes.c_size_t), ("State", wt.DWORD), ("Protect", wt.DWORD),
                ("Type", wt.DWORD)]

def regions(p):
    addr, mbi = 0, MBI()
    while k32.VirtualQueryEx(p.h, ctypes.c_void_p(addr), ctypes.byref(mbi), ctypes.sizeof(mbi)):
        base, size = mbi.BaseAddress or 0, mbi.RegionSize
        if mbi.State == 0x1000 and mbi.Protect & 0xCC and not mbi.Protect & 0x100 and size <= 0x100000000:
            yield base, size, mbi.Type, mbi.AllocationBase or 0
        addr = base + size
        if addr >= 0x7FFFFFFFFFFF: break

def scan(p, sig_v, V):
    pats = [("options 0x2BC..0x3BC", 0x2BC, sig_v[0x2BC:0x3BC]), ("base traits 0x304", 0x304, sig_v[0x304:0x330]),
            ("loadouts 0x681", 0x681, sig_v[0x681:0x681 + 60]), ("megalo sets 0x5DD0", 0x5DD0, sig_v[0x5DD0:0x5DD0 + 0x60])]
    hits = {n: [] for n, _, _ in pats}; scanned = 0; t0 = time.time(); mods = {}
    for m in p.modules(): mods[m[0]] = m
    for base, size, typ, ab in regions(p):
        for o in range(0, size, 0x1000000):
            n = min(0x1000000 + 0x200, size - o)
            try: buf = p.read(base + o, n)
            except IOError: continue
            scanned += n
            for name, off, pat in pats:
                if pat.count(0) == len(pat): continue
                for m in re.finditer(re.escape(pat), buf):
                    a = base + o + m.start()
                    if a not in [h[0] for h in hits[name]]: hits[name].append((a, a - off, typ, ab))
    print(f"scanned {scanned / 2**20:.0f} MB in {time.time() - t0:.1f} s; live V = {V:#x}" if V else f"scanned {scanned / 2**20:.0f} MB")
    for name, _, _ in pats:
        print(f"{name}: {len(hits[name])} hits")
        for a, vb, typ, ab in hits[name][:20]:
            kind = {0x20000: "private", 0x40000: "mapped", 0x1000000: "image"}.get(typ, hex(typ))
            if ab in mods: kind = mods[ab][2]
            print(f"  {a:#x}  as V {vb:#x}{'  <- live V' if V and vb == V else ''}  {kind} alloc {ab:#x} (+{a - ab:#x})")

def tick():
    sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
    import numpy as np
    from artemis.bridge import Bridge, discover
    b = Bridge(int(discover()[0]["pid"])); t = b.wait_tick(3.0)
    if t is None: raise SystemExit("no tick")
    m = t.match
    print("match " + " ".join(f"{k}={m[k] if k != 'PowerupDurationTicks' else list(m[k])}" for k in m.dtype.names
                              if k not in ("PlayerScores", "TeamTotalScores", "TeamRoundScores")))
    for pl in t.players:
        tr = pl["Traits"]
        print(f"player {int(pl['Handle']):#x} team {pl['Team']} local {pl['LocalIndex']} alive {pl['IsAlive']} " + " ".join(
            f"{k}={float(tr[k]):.3g}" if isinstance(tr[k], np.floating) else f"{k}={tr[k]}" for k in tr.dtype.names))

def main(a):
    cmd = a[0] if a else "dump"
    if cmd == "tick": return tick()
    if cmd in ("scan", "peek", "poke", "diff") and (cmd != "scan" or len(a) > 1):
        if cmd == "diff": return diff(a[1], a[2])
        p = Proc(find_pid() or sys.exit("MCC not running"))
        if cmd == "scan": return scan(p, load(a[1], "v"), None)
        addr = int(a[1], 16)
        if cmd == "peek": print(f"{addr:#x}: {p.read(addr, int(a[2]) if len(a) > 2 else 16).hex(' ')}"); return
        data = bytes(int(x, 16) for x in a[2:])
        print(f"{addr:#x}: {p.read(addr, len(data)).hex(' ')} -> {data.hex(' ')}"); p.write(addr, data); return
    p, V, pl, tid, mode = resolve()
    print(f"pid {p.pid} thread {tid} mode {mode}")
    if cmd == "dump": dump(p, V, pl)
    elif cmd == "snap": snap(p, V, pl, a[1])
    elif cmd == "scan": scan(p, read_chunked(p, V, V_SIZE), V)
    elif cmd == "get":
        off = int(a[1], 16); n = int(a[2]) if len(a) > 2 else 1
        print(f"V+{off:#x}: {p.read(V + off, n).hex(' ')}")
    elif cmd == "set":
        off = int(a[1], 16); data = bytes(int(x, 16) for x in a[2:])
        print(f"V+{off:#x}: {p.read(V + off, len(data)).hex(' ')} -> {data.hex(' ')}")
        p.write(V + off, data); dump(p, V, pl)
    elif cmd == "watch":
        end = time.time() + float(a[1]); last = None
        while time.time() < end:
            b = p.read(pl + 0x328, 0x2C); cur = (b[0x18], b[0x19], struct.unpack_from("<i", b, 0x1C)[0])
            if cur != last: print(f"{time.strftime('%H:%M:%S')} p0 speed={cur[0]} grav={cur[1]} jump={cur[2]}", flush=True); last = cur
            time.sleep(0.05)
    else: print(__doc__)

if __name__ == "__main__":
    main(sys.argv[1:])
