"""
analyze_captures.py
Reads a datum capture file of Artemis (UI Capture tab, F7; D81) and proposes hypotheses from it.
The file lives in <Storage Folder>/Captures/<date_time>.txt. Offsets are absolute datum offsets.
Field names come from Data/Datum/<component>.xml, so known fields show their name and unknown ones stand out.

    python Tools/analyze_captures.py FILE list
        Captures, labels and objects.
    python Tools/analyze_captures.py FILE diff A B [--class bipd] [--component unit]
        Byte runs that changed between capture A and capture B, with the field they fall in.
    python Tools/analyze_captures.py FILE field OFFSET [--type u32|i32|u16|u8|f32] [--self | --object HANDLE | --class bipd]
        Value of one offset in every capture.
    python Tools/analyze_captures.py FILE split TEXT_A TEXT_B [--class bipd] [--component unit] [--all]
        Captures whose label contains TEXT_A against those whose label contains TEXT_B (ignores case and accents).
        Lists offsets that hold one value in every A capture and another one in every B capture. Repeat each state
        two or three times when capturing: with one capture per side every changed byte qualifies.
        --all also lists offsets whose A and B values never overlap but are not constant (energy, timers).
    python Tools/analyze_captures.py FILE clocks [--class bipd]
        u32 offsets that grow in every capture: ticks and counters (noise for split, useful as clocks).

Objects are matched across captures by handle. --self follows the local biped even if it respawns with a new handle.
"""
import argparse, os, re, struct, sys, unicodedata
import xml.etree.ElementTree as ET

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
DATUM_XML = os.path.join(REPO, "Data", "Datum")

TYPE_SIZE = {"8": 1, "16": 2, "32": 4, "64": 8}
FIXED_SIZE = {"vector2": 8, "point2": 8, "vector3": 12, "point3": 12, "quaternion": 16, "float32": 4, "stringid": 4}

# --- Parsing ---------------------------------------------------------------------------------------------------------

class Obj:
    def __init__(self, handle, source, cls, size, address, tag):
        self.Handle, self.Source, self.Class, self.Address, self.Tag = handle, source, cls, address, tag
        self.Bytes = bytearray(size)
        self.Ranges = []  # (component, start, end)

    def ComponentAt(self, offset):
        for name, start, end in self.Ranges:
            if start <= offset < end:
                return name
        return "?"

class Capture:
    def __init__(self, index, generation, time, label):
        self.Index, self.Generation, self.Time, self.Label = index, generation, time, label
        self.Objects = {}

    def Self(self):
        return next((o for o in self.Objects.values() if o.Source == "self"), None)

def Load(path):
    captures, cap, obj = [], None, None
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\r\n")
            if line.startswith("== capture"):
                m = re.match(r'== capture (\d+) generation (\d+) time (\S+) label "(.*)"', line)
                cap = Capture(int(m[1]), int(m[2]), m[3], m[4])
                captures.append(cap)
            elif line.startswith("-- object"):
                m = re.match(r"-- object (\w+) source (\w+) class (\S+) size 0x(\w+) address 0x(\w+) tag ?(.*)", line)
                obj = Obj(m[1], m[2], m[3], int(m[4], 16), m[5], m[6])
                cap.Objects[obj.Handle] = obj
            elif line.startswith(".. "):
                name, start, end = line[3:].split()
                obj.Ranges.append((name, int(start, 16), int(end, 16)))
            elif re.match(r"^[0-9A-F]{4,}  ", line):
                offset, hexText = line.split("  ", 1)
                data = bytes.fromhex(hexText.replace(" ", ""))
                o = int(offset, 16)
                obj.Bytes[o:o + len(data)] = data
    return captures

# --- Field names -----------------------------------------------------------------------------------------------------

def FieldSize(tag):
    if tag in FIXED_SIZE:
        return FIXED_SIZE[tag]
    m = re.search(r"(8|16|32|64)$", tag)
    return TYPE_SIZE[m[1]] if m else 1

def LoadFields():
    fields = {}
    if not os.path.isdir(DATUM_XML):
        return fields
    for name in os.listdir(DATUM_XML):
        if not name.endswith(".xml"):
            continue
        root = ET.parse(os.path.join(DATUM_XML, name)).getroot()
        component = root.get("component", name[:-4])
        entries = []
        for el in root:
            if el.get("offset") is None:
                continue
            entries.append((int(el.get("offset"), 16), FieldSize(el.tag), el.get("name", "?"), el.tag, el.get("confidence", "")))
        fields[component] = sorted(entries)
    return fields

def FieldAt(fields, component, offset):
    for start, size, name, tag, confidence in fields.get(component, []):
        if start <= offset < start + size:
            suffix = "" if start == offset else f" +0x{offset - start:X}"
            return f"{name}{suffix} ({tag}, {confidence or '-'})"
    return "(no field)"

# --- Helpers ---------------------------------------------------------------------------------------------------------

FORMATS = {"u8": ("<B", 1), "u16": ("<H", 2), "u32": ("<I", 4), "i32": ("<i", 4), "f32": ("<f", 4)}

def Read(data, offset, kind):
    fmt, size = FORMATS[kind]
    if offset + size > len(data):
        return None
    return struct.unpack_from(fmt, data, offset)[0]

def Show(value, size):
    text = f"0x{value:0{size * 2}X}"
    if size == 4:
        f = struct.unpack("<f", struct.pack("<I", value))[0]
        if f != 0 and 1e-4 < abs(f) < 1e6:
            text += f" ({f:.4g})"
        if (value >> 16) not in (0, 0xFFFF) and (value & 0xFFFF) < 0x800:
            text += " handle?"
    return text

def Pick(capture, args):
    objs = list(capture.Objects.values())
    if getattr(args, "self", False):
        objs = [o for o in objs if o.Source == "self"]
    if getattr(args, "object", None):
        objs = [o for o in objs if o.Handle.upper() == args.object.upper()]
    if getattr(args, "cls", None):
        objs = [o for o in objs if o.Class == args.cls]
    return objs

def Key(obj, args):
    return "self" if getattr(args, "self", False) and obj.Source == "self" else obj.Handle

# --- Commands --------------------------------------------------------------------------------------------------------

def CmdList(captures, fields, args):
    for c in captures:
        print(f"capture {c.Index}  generation {c.Generation}  {c.Time}  \"{c.Label}\"")
        for o in c.Objects.values():
            print(f"    {o.Handle} {o.Source:<6} {o.Class} size 0x{len(o.Bytes):X} {o.Tag}")

def CmdDiff(captures, fields, args):
    a = next(c for c in captures if c.Index == args.a)
    b = next(c for c in captures if c.Index == args.b)
    for handle, ob in b.Objects.items():
        oa = a.Objects.get(handle)
        if oa is None or len(oa.Bytes) != len(ob.Bytes):
            continue
        if args.cls and ob.Class != args.cls:
            continue
        offset, size = 0, len(ob.Bytes)
        while offset < size:
            if oa.Bytes[offset] == ob.Bytes[offset]:
                offset += 1
                continue
            end = offset
            while end < size and oa.Bytes[end] != ob.Bytes[end]:
                end += 1
            component = ob.ComponentAt(offset)
            if not args.component or args.component == component:
                print(f"{ob.Class} {handle} {component:<6} 0x{offset:04X}  {oa.Bytes[offset:end].hex(' ').upper():<24} -> "
                      f"{ob.Bytes[offset:end].hex(' ').upper():<24} {FieldAt(fields, component, offset)}")
            offset = end

def CmdField(captures, fields, args):
    offset = int(args.offset, 16)
    for c in captures:
        for o in Pick(c, args):
            value = Read(o.Bytes, offset, args.type)
            shown = "-" if value is None else (f"{value:.6g}" if args.type == "f32" else Show(value, FORMATS[args.type][1]))
            print(f"capture {c.Index:<3} {o.Class} {o.Handle}  {shown:<28} \"{c.Label}\"")
    component = next((o.ComponentAt(offset) for c in captures for o in Pick(c, args)), "?")
    print(f"field: {component} {FieldAt(fields, component, offset)}")

def Fold(text):
    return "".join(ch for ch in unicodedata.normalize("NFKD", text.lower()) if not unicodedata.combining(ch))

def Groups(captures, text):
    return [c for c in captures if Fold(text) in Fold(c.Label)]

def CmdSplit(captures, fields, args):
    groupA, groupB = Groups(captures, args.a), Groups(captures, args.b)
    if not groupA or not groupB:
        sys.exit(f"No captures match: A {len(groupA)}, B {len(groupB)}.")
    print(f"A \"{args.a}\": captures {[c.Index for c in groupA]}   B \"{args.b}\": captures {[c.Index for c in groupB]}")
    if len(groupA) < 2 or len(groupB) < 2:
        print("note: one capture on a side, every changed byte qualifies. Repeat each state to filter noise.")

    def Objects(group):
        result = []
        for c in group:
            result.append({Key(o, args): o for o in Pick(c, args)})
        return result

    objsA, objsB = Objects(groupA), Objects(groupB)
    keys = set.intersection(*[set(d) for d in objsA + objsB])
    for key in sorted(keys):
        listA = [d[key] for d in objsA]
        listB = [d[key] for d in objsB]
        size = min(len(o.Bytes) for o in listA + listB)
        covered = set()
        sample = listB[0]
        for width, kind in ((4, "u32"), (2, "u16"), (1, "u8")):
            for offset in range(0, size - width + 1, width):
                if any(offset + i in covered for i in range(width)):
                    continue
                valuesA = {Read(o.Bytes, offset, kind) for o in listA}
                valuesB = {Read(o.Bytes, offset, kind) for o in listB}
                if valuesA & valuesB:
                    continue
                isConstant = len(valuesA) == 1 and len(valuesB) == 1
                if not isConstant and not args.all:
                    continue
                component = sample.ComponentAt(offset)
                if args.component and args.component != component:
                    continue
                covered.update(range(offset, offset + width))
                tagA = " / ".join(Show(v, width) for v in sorted(valuesA))
                tagB = " / ".join(Show(v, width) for v in sorted(valuesB))
                mark = "" if isConstant else "  [varies]"
                print(f"{sample.Class} {key} {component:<6} 0x{offset:04X} {kind:<3}  A {tagA}   B {tagB}{mark}   {FieldAt(fields, component, offset)}")

def CmdClocks(captures, fields, args):
    if len(captures) < 3:
        sys.exit("clocks needs three captures or more.")
    perKey = {}
    for c in captures:
        for o in Pick(c, args):
            perKey.setdefault(o.Handle, []).append(o)
    for handle, objs in perKey.items():
        if len(objs) < 3:
            continue
        size = min(len(o.Bytes) for o in objs)
        for offset in range(0, size - 3, 4):
            values = [Read(o.Bytes, offset, "u32") for o in objs]
            if all(b > a for a, b in zip(values, values[1:])) and values[-1] - values[0] < 0x10000000:
                component = objs[0].ComponentAt(offset)
                print(f"{objs[0].Class} {handle} {component:<6} 0x{offset:04X}  {values[0]} .. {values[-1]}   {FieldAt(fields, component, offset)}")

def main():
    parser = argparse.ArgumentParser(description="Analyze an Artemis datum capture file (D81).")
    parser.add_argument("file")
    sub = parser.add_subparsers(dest="command", required=True)

    def Filters(p, single=False):
        p.add_argument("--class", dest="cls")
        if single:
            p.add_argument("--self", action="store_true")
            p.add_argument("--object")

    sub.add_parser("list")
    p = sub.add_parser("diff"); p.add_argument("a", type=int); p.add_argument("b", type=int); p.add_argument("--component"); Filters(p)
    p = sub.add_parser("field"); p.add_argument("offset"); p.add_argument("--type", default="u32", choices=FORMATS); Filters(p, True)
    p = sub.add_parser("split"); p.add_argument("a"); p.add_argument("b"); p.add_argument("--component"); p.add_argument("--all", action="store_true"); Filters(p, True)
    p = sub.add_parser("clocks"); Filters(p, True)

    args = parser.parse_args()
    captures = Load(args.file)
    fields = LoadFields()
    {"list": CmdList, "diff": CmdDiff, "field": CmdField, "split": CmdSplit, "clocks": CmdClocks}[args.command](captures, fields, args)

if __name__ == "__main__":
    main()
