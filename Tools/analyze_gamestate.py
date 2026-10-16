"""
analyze_gamestate.py
Searches the game state copies that Capture writes with the "Engine globals" option (A5): one .bin per capture next to the session
file, named in its gs-dump line. Values are found by label, like a memory scanner that keeps the history of every capture.
Every offset is also shown as tc+0xSLOT+0xDELTA: the global of the thread context it falls in (from the "context" engine block).

    python Tools/analyze_gamestate.py FILE list
        Captures, labels and game state copies.
    python Tools/analyze_gamestate.py FILE search "LABEL=VALUE; LABEL=VALUE; ..." [--types u8,i16,u16,i32,u32,f32] [--max 60]
        Offsets (aligned to their size) whose value is VALUE in every capture of each LABEL (label match ignores case, contains). VALUE can be a number,
        "*" (any), or a relation to the previous condition: "+1", "-1", ">", "<", "!" (different), "=" (same). The values of the
        offsets found are printed for every capture.
    python Tools/analyze_gamestate.py FILE clocks [--types u32,i32,f32] [--max 40]
        Offsets that grow in every capture (ticks, timers).
    python Tools/analyze_gamestate.py FILE field OFFSET [--types u8,u32,...]
        Value of one game state offset (hex) in every capture.
"""
import argparse, os, re, struct, sys, unicodedata
try:
    import numpy as np
except ImportError:
    sys.exit("numpy is required: pip install numpy")

TYPES = {"u8": "<u1", "i8": "<i1", "u16": "<u2", "i16": "<i2", "u32": "<u4", "i32": "<i4", "f32": "<f4"}

def Fold(text):
    return "".join(c for c in unicodedata.normalize("NFD", text.lower()) if unicodedata.category(c) != "Mn")

def Load(path):
    caps, cap, inContext = [], None, False
    folder = os.path.dirname(os.path.abspath(path))
    for line in open(path, encoding="utf-8", errors="replace"):
        line = line.rstrip("\r\n")
        if line.startswith("== capture"):
            m = re.match(r'== capture (\d+) generation (\d+) time (\S+) label "(.*)"', line)
            cap = {"index": int(m[1]), "label": m[4], "file": None, "context": bytearray(0x400)}
            caps.append(cap)
            inContext = False
        elif line.startswith("gs-dump"):
            w = line.split()
            cap["file"], cap["base"], cap["size"] = os.path.join(folder, w[1]), int(w[3], 16), int(w[5], 16)
        elif line.startswith("-- object"):
            inContext = " class context " in line
        elif inContext and re.match(r"^[0-9A-F]{4,}  ", line):
            off, hexText = line.split("  ", 1)
            data = bytes.fromhex(hexText.replace(" ", ""))
            cap["context"][int(off, 16):int(off, 16) + len(data)] = data
    caps = [c for c in caps if c["file"] and os.path.exists(c["file"])]
    if not caps: sys.exit("No capture with a game state copy (gs-dump) found next to the file.")
    size = min(c["size"] for c in caps)
    for c in caps:
        c["data"] = np.fromfile(c["file"], dtype=np.uint8)[:size]
    return caps, size

def Slots(cap):
    slots = []
    for i in range(0, 0x400, 8):
        p = struct.unpack_from("<Q", cap["context"], i)[0]
        if cap["base"] <= p < cap["base"] + cap["size"]: slots.append((p - cap["base"], i))
    return sorted(slots)

def Where(offset, slots):
    best = None
    for start, slot in slots:
        if start <= offset: best = (start, slot)
        else: break
    return f"tc+0x{best[1]:03X}+0x{offset - best[0]:X}" if best else "-"

def Values(caps, dtype, offset):
    size = np.dtype(dtype).itemsize
    return [np.frombuffer(c["data"][offset:offset + size].tobytes(), dtype=dtype)[0].item() for c in caps]

def Show(v):
    return f"{v:.4g}" if isinstance(v, float) else str(v)

def Report(caps, slots, dtype, name, offsets, limit):
    for off in offsets[:limit]:
        vals = Values(caps, dtype, int(off))
        print(f"  0x{int(off):07X} {Where(int(off), slots):<24} {name:<4} " + " ".join(Show(v) for v in vals))
    if len(offsets) > limit: print(f"  ... {len(offsets) - limit} more")

def Views(caps, size, dtype):
    item = np.dtype(dtype).itemsize
    # note: Values are read at their natural alignment, like the engine stores them.
    for shift in [0]:
        n = (size - shift) // item
        yield shift, item, [np.frombuffer(c["data"][shift:shift + n * item].tobytes(), dtype=dtype) for c in caps]

def CmdSearch(caps, size, args):
    conds = []
    for part in [p.strip() for p in args.expr.split(";") if p.strip()]:
        label, value = part.rsplit("=", 1) if not part.endswith("==") else (part[:-2], "=")
        idx = [i for i, c in enumerate(caps) if Fold(label.strip()) in Fold(c["label"])]
        if not idx: sys.exit(f"No capture matches label '{label.strip()}'.")
        conds.append((idx, value.strip()))
    slots = Slots(caps[0])
    for name in args.types.split(","):
        dtype = TYPES[name]
        found = []
        for shift, item, arrs in Views(caps, size, dtype):
            mask = np.ones(len(arrs[0]), dtype=bool)
            previous = None
            for idx, value in conds:
                group = [arrs[i] for i in idx]
                same = np.ones_like(mask)
                for g in group[1:]: same &= (g == group[0])
                mask &= same
                cur = group[0]
                if value == "*": pass
                elif value in ("+1", "-1") and previous is not None: mask &= (cur == previous + int(value))
                elif value == ">" and previous is not None: mask &= (cur > previous)
                elif value == "<" and previous is not None: mask &= (cur < previous)
                elif value == "!" and previous is not None: mask &= (cur != previous)
                elif value == "=" and previous is not None: mask &= (cur == previous)
                else: mask &= (cur == (float(value) if dtype == "<f4" else int(value, 0)))
                previous = cur
            found += [shift + i * item for i in np.nonzero(mask)[0]]
        found.sort()
        print(f"{name}: {len(found)} offsets")
        Report(caps, slots, dtype, name, found, args.max)

def CmdClocks(caps, size, args):
    slots = Slots(caps[0])
    for name in args.types.split(","):
        dtype = TYPES[name]
        found = []
        for shift, item, arrs in Views(caps, size, dtype):
            mask = np.ones(len(arrs[0]), dtype=bool)
            for a, b in zip(arrs, arrs[1:]): mask &= (b > a)
            found += [shift + i * item for i in np.nonzero(mask)[0]]
        found.sort()
        print(f"{name}: {len(found)} growing offsets")
        Report(caps, slots, dtype, name, found, args.max)

def main():
    p = argparse.ArgumentParser()
    p.add_argument("file"); p.add_argument("command", choices=["list", "search", "clocks", "field"]); p.add_argument("expr", nargs="?")
    p.add_argument("--types", default=None); p.add_argument("--max", type=int, default=60)
    args = p.parse_args()
    caps, size = Load(args.file)
    if args.command == "list":
        for c in caps: print(f"capture {c['index']:>3} \"{c['label']}\"  {os.path.basename(c['file'])}  base 0x{c['base']:X} size 0x{c['size']:X}")
        print("globals of the context inside the game state:", ", ".join(f"tc+0x{s:03X}@0x{o:X}" for o, s in Slots(caps[0])))
    elif args.command == "search":
        args.types = args.types or "u8,i16,i32,f32"; CmdSearch(caps, size, args)
    elif args.command == "clocks":
        args.types = args.types or "u32,f32"; CmdClocks(caps, size, args)
    else:
        off = int(args.expr, 16); slots = Slots(caps[0])
        for name in (args.types or "u8,u16,u32,f32").split(","):
            print(f"0x{off:X} {Where(off, slots)} {name}: " + " ".join(Show(v) for v in Values(caps, TYPES[name], off)))

if __name__ == "__main__":
    main()
