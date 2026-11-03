"""Q97 probe: read and write the game variant of the running match from outside MCC (ctypes, no Artemis code).

Finds the engine thread context like the DLL does (signature TelemetryIdModifier -> TLS slot; TEB+0x58 of each thread of MCC),
then V = *(tls+0x48) + 0x14E8 (A19_Traits.txt sec. 3) and the players at *(*(tls+0x18)+0x50) + i*0x490 (effective traits +0x328).

    variant_poke.py dump                     general/respawn options, base traits of V and the effective traits of each player
    variant_poke.py get OFF [N]              N bytes (default 1) of V at OFF (hex)
    variant_poke.py set OFF B0 [B1 ...]      write bytes (hex) at V+OFF, then prints dump
    variant_poke.py watch SECONDS            prints the effective gravity/speed/jump of player 0 when they change
"""
import ctypes
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

def main(a):
    p, V, pl, tid, mode = resolve()
    print(f"pid {p.pid} thread {tid} mode {mode}")
    cmd = a[0] if a else "dump"
    if cmd == "dump": dump(p, V, pl)
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
