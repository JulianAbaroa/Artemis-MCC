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
    python Tools/analyze_captures.py FILE havok [--label TEXT] [--class bipd] [--pairs] [--matrix]
        Havok data of the captures (A10, Ignore/Tareas/Transversal/A10_Colisiones.txt): layer table of the collision filter
        (symmetry check, rows of the layers in use, --matrix for all of them), and per object its component and rigid bodies
        (layer, motion type, system group). --pairs prints which captured objects collide with each other, with the
        isCollisionEnabled rule of the hkpGroupFilter (FUN_180882080). --label keeps the captures whose label contains TEXT.

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
        self.Havok = None  # dict of the hk-component line, {} when the object has no component
        self.Bodies = []   # dicts of the hk-body lines
        self.Phantoms = [] # dicts of the hk-phantom lines
        self.Phmo = None   # dict of the hk-phmo line, {} when the object has no physics model
        self.Rigids = []   # dicts of the hk-rigid lines, each with its "shapes" (dicts of the hk-shape lines)
        self.PhantomTypes = []  # dicts of the hk-phantomtype lines

    def ComponentAt(self, offset):
        for name, start, end in self.Ranges:
            if start <= offset < end:
                return name
        return "?"

class Capture:
    def __init__(self, index, generation, time, label):
        self.Index, self.Generation, self.Time, self.Label = index, generation, time, label
        self.Objects = {}
        self.Filter = None  # dict of the hk-filter line
        self.Lookup = []    # 32 rows of the Havok layer table

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
            elif line.startswith("hk-"):
                LoadHavokLine(line, cap, obj)
            elif line.startswith(".. "):
                name, start, end = line[3:].split()
                obj.Ranges.append((name, int(start, 16), int(end, 16)))
            elif re.match(r"^[0-9A-F]{4,}  ", line):
                offset, hexText = line.split("  ", 1)
                data = bytes.fromhex(hexText.replace(" ", ""))
                o = int(offset, 16)
                obj.Bytes[o:o + len(data)] = data
    return captures

def Pairs(text):
    words = text.split()
    return {words[i]: words[i + 1] for i in range(0, len(words) - 1, 2)}

def LoadHavokLine(line, cap, obj):
    kind, _, rest = line.partition(" ")
    if kind == "hk-filter":
        cap.Filter = None if rest == "none" else Pairs(rest)
    elif kind == "hk-lookup":
        layer, value = rest.split()
        cap.Lookup.append(int(value, 16))
    elif kind == "hk-component":
        words = rest.split()
        if words[0] == "none":
            obj.Havok = {}
        elif len(words) > 1 and words[1] == "unreadable":
            obj.Havok = {"handle": words[0], "unreadable": True}
        else:
            obj.Havok = {"handle": words[0], **Pairs(" ".join(words[1:]))}
    elif kind == "hk-phmo":
        words = rest.split()
        obj.Phmo = {} if words[0] == "none" else {"tag": words[0], **Pairs(" ".join(words[1:]))}
    elif kind == "hk-rigid":
        index, _, fields = rest.partition(" ")
        words = fields.split()
        root = words.index("root")
        rigid = Pairs(" ".join(words[:root] + words[root + 3:]))
        rigid.update({"index": int(index), "roottype": words[root + 1], "rootindex": words[root + 2], "shapes": []})
        obj.Rigids.append(rigid)
    elif kind == "hk-shape":
        body, shapeIndex, fields = rest.split(" ", 2)
        shape = Pairs(fields)
        shape["index_in_body"] = int(shapeIndex)
        obj.Rigids[int(body)]["shapes"].append(shape)
    elif kind == "hk-phantomtype":
        index, _, fields = rest.partition(" ")
        words = fields.split()
        entry = {"index": int(index)}
        i = 0
        while i < len(words):
            if words[i] in ("direction", "center", "axis", "orbit"):
                entry[words[i]] = (words[i + 1], words[i + 2]); i += 3
            else:
                entry[words[i]] = words[i + 1]; i += 2
        obj.PhantomTypes.append(entry)
    elif kind == "hk-phantom":
        slot, _, fields = rest.partition(" ")
        phantom = Pairs(fields)
        phantom["slot"] = slot
        obj.Phantoms.append(phantom)
    elif kind == "hk-body":
        index, _, fields = rest.partition(" ")
        body = Pairs(fields)
        body["index"] = int(index)
        obj.Bodies.append(body)

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

# --- Havok (A10) -----------------------------------------------------------------------------------------------------

LAYERS = ["Everything", "Environment Default", "Environment Only", "Small Crate", "Crate", "Huge Crate", "Item", "Projectile",
          "Machine", "Early Mover Machine", "Creature", "Biped", "Dead Biped", "Super Collidable Ragdoll", "Ragdoll", "Vehicle",
          "Decal", "Forge Dynamic Scenery", "Small Expensive Plant", "Tech Art Custom"]
MOTIONS = {0: "invalid", 1: "dynamic", 2: "sphere inertia", 3: "box inertia", 4: "keyframed", 5: "fixed", 6: "thin box", 7: "character"}

def LayerName(layer):
    return f"{layer} {LAYERS[layer]}" if layer < len(LAYERS) else f"{layer} (unnamed)"

def IsCollisionEnabled(lookup, a, b):
    # Same logic as FUN_180882080 (hkpGroupFilter::isCollisionEnabled).
    if ((a ^ b) & 0xFFFF0000) != 0 or (a & 0xFFFF0000) == 0:
        return (lookup[a & 0x1F] >> (b & 0x1F)) & 1 != 0
    return ((b >> 5 ^ a) & 0x3E0) != 0 and ((a >> 5 ^ b) & 0x3E0) != 0

# Component flags (+0x18): bit 2 set while the bodies are in the Havok world (FUN_18017fd78 removes them and clears it),
# bit 0 set when some body is not fixed (FUN_18017d680).
FLAG_IN_WORLD = 0x4

def IsInWorld(obj):
    return bool(obj.Havok) and "flags" in obj.Havok and (int(obj.Havok["flags"], 16) & FLAG_IN_WORLD) != 0

def Infos(obj):
    if not IsInWorld(obj):
        return []
    return [int(b["info"], 16) for b in obj.Bodies if b.get("address", "0x0") != "0x0"]

SHAPES = {0: "sphere", 1: "pill", 2: "box", 3: "triangle", 4: "polyhedron", 5: "multi sphere", 6: "phantom", 14: "list", 15: "mopp"}
NO_GROUP = 255

def EffectiveGroup(shape):
    # H7 (A10): the shape override wins, then the material override; 255 keeps the layer of the rigid body.
    group, material = int(shape["group"]), int(shape["materialgroup"])
    if group != NO_GROUP:
        return group, "shape"
    if material != NO_GROUP:
        return material, "material"
    return None, "body"

def PrintPhmo(o):
    if o.Phmo is None:
        return
    if not o.Phmo:
        print("      phmo: none")
        return
    print(f"      phmo {o.Phmo['tag']}  rigid bodies {o.Phmo['rigidbodies']}  materials {o.Phmo['materials']}  phantom types {o.Phmo['phantomtypes']}")
    for r in o.Rigids:
        root = SHAPES.get(int(r["roottype"]), r["roottype"])
        print(f"        rigid {r['index']}: tag motion {r['motion']} size {r['size']} flags {r['flags']} mask {r['mask']} root {root} {r['rootindex']}, {len(r['shapes'])} shapes")
        groups = {}
        for s in r["shapes"]:
            group, source = EffectiveGroup(s)
            key = ("body layer" if group is None else f"{LayerName(group)} (from {source})") + (" PHANTOM" if s["phantom"] == "1" else "")
            groups.setdefault(key, []).append(f"{SHAPES.get(int(s['type']), s['type'])} {s['index']}")
        for key, shapes in groups.items():
            shown = ", ".join(shapes[:6]) + (f" ... +{len(shapes) - 6}" if len(shapes) > 6 else "")
            print(f"          {key:<44} {len(shapes):>3} shapes: {shown}")
    for p in o.PhantomTypes:
        print(f"        phantom type {p['index']}: flags {p['flags']} marker {p['marker']} direction {p['direction']} center {p['center']} axis {p['axis']} orbit {p['orbit']}")

def CmdHavok(captures, fields, args):
    selected = Groups(captures, args.label) if args.label else captures
    if not selected:
        sys.exit("No captures match.")
    withTable = [c for c in selected if len(c.Lookup) == 32]
    if not withTable:
        print("No Havok layer table in the selected captures (hk-filter none or an old capture file).")
    else:
        lookup = withTable[-1].Lookup
        print(f"Layer table of capture {withTable[-1].Index}: world {withTable[-1].Filter.get('world')} filter {withTable[-1].Filter.get('filter')}")
        changed = [c.Index for c in withTable if c.Lookup != lookup]
        if changed:
            print(f"warning: the table differs in captures {changed}.")
        broken = [(i, j) for i in range(32) for j in range(32) if ((lookup[i] >> j) & 1) != ((lookup[j] >> i) & 1)]
        print("symmetric: yes" if not broken else f"symmetric: NO, {len(broken)} pairs differ, first {broken[:5]} (offset of the table may be wrong)")
        used = sorted({info & 0x1F for c in selected for o in Pick(c, args) for info in Infos(o)})
        rows = range(32) if args.matrix else used
        for layer in rows:
            hits = [j for j in range(32) if (lookup[layer] >> j) & 1]
            print(f"  {LayerName(layer):<28} {lookup[layer]:08X}  collides with {', '.join(str(j) for j in hits) or '-'}")
    for c in selected:
        print(f"\ncapture {c.Index}  \"{c.Label}\"")
        for o in Pick(c, args):
            print(f"  {o.Class} {o.Handle} {o.Source:<6} {o.Tag}")
            if o.Havok is None:
                print("      no hk-component line")
                continue
            if not o.Havok:
                print("      no Havok component (obje 0xCC = -1)")
                continue
            if o.Havok.get("unreadable"):
                print(f"      component {o.Havok['handle']} unreadable")
                continue
            handle = int(o.Havok["handle"], 16)
            checks = []
            checks.append("salt ok" if int(o.Havok["salt"], 16) == handle >> 16 else f"salt {o.Havok['salt']} != {handle >> 16:04X}")
            checks.append("owner ok" if o.Havok["owner"].upper() == o.Handle.upper() else f"owner {o.Havok['owner']}")
            checks.append("in world" if IsInWorld(o) else "OUT of world")
            print(f"      component {o.Havok['handle']} flags {o.Havok['flags']} size {o.Havok['size']} bodies {o.Havok['bodies']}  ({', '.join(checks)})")
            for b in o.Bodies:
                info = int(b["info"], 16)
                motion = int(b["motion"])
                print(f"      body {b['index']}: layer {LayerName(info & 0x1F):<26} motion {motion} {MOTIONS.get(motion, '?'):<14} "
                      f"group {b['group']} subsystem {b['subsystem']} ignores {b['ignores']}")
            PrintPhmo(o)
            for ph in o.Phantoms:
                info = int(ph["info"], 16)
                print(f"      phantom {ph['slot']}: layer {LayerName(info & 0x1F):<26} group {ph['group']} subsystem {ph['subsystem']} ignores {ph['ignores']}")
    if args.pairs and withTable:
        lookup = withTable[-1].Lookup
        objs = []
        for c in selected:
            for o in Pick(c, args):
                if Infos(o):
                    objs.append((f"c{c.Index} {o.Class} {o.Handle[-4:]}", Infos(o), c.Label, o.Tag))
        print("\nPairs (Y = some body of the row collides with some body of the column; objects out of the world are left out).")
        print("note: Havok makes no contact between two non-dynamic bodies (motion 4 keyframed or 5 fixed), even when the layers collide.")
        for i, (name, _, label, tag) in enumerate(objs):
            print(f"  [{i}] {name}  \"{label}\"  {tag}")
        print("      " + " ".join(f"{i:>3}" for i in range(len(objs))))
        for i, (_, infosA, _, _) in enumerate(objs):
            cells = []
            for j, (_, infosB, _, _) in enumerate(objs):
                cells.append("  ." if i == j else ("  Y" if any(IsCollisionEnabled(lookup, a, b) for a in infosA for b in infosB) else "  -"))
            print(f"  {i:>3} " + " ".join(cells))

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
    p = sub.add_parser("havok"); p.add_argument("--label"); p.add_argument("--pairs", action="store_true"); p.add_argument("--matrix", action="store_true"); Filters(p)

    args = parser.parse_args()
    captures = Load(args.file)
    fields = LoadFields()
    {"list": CmdList, "diff": CmdDiff, "field": CmdField, "split": CmdSplit, "clocks": CmdClocks, "havok": CmdHavok}[args.command](captures, fields, args)

if __name__ == "__main__":
    main()
