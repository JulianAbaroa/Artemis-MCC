"""
presets.py
Compiler of the gametype presets (Q97, D264, D265): reads the catalog of the writable fields of the game variant
(Data/Presets/Catalog.xml) and the presets (Data/Presets/<name>.xml), resolves the inheritance and the names of the enums, and writes
the text files that the DLL reads, so editing a preset needs no rebuild.

    python Tools/presets.py compile [--out DIR]   One <name>.txt per preset and _catalog.txt (default build/game/presets).
    python Tools/presets.py --check [--out DIR]   Exit 1 when the files of DIR differ from what the XML gives (nothing is written).
    python Tools/presets.py list                  Presets with their base, number of writes and doc.

Other options: --presets DIR and --catalog FILE (another source, used by the tests), --eol repo (CRLF and no final newline, the
format of the files committed to the repo, used for the fixture of the C++ tests).

Compiled <name>.txt (ASCII, one line per item, inheritance already resolved; bytes and mask are hex in memory order):
    preset <name>
    source <sha1 of the XML of the preset and of its chain of bases, line endings normalized>
    write <offset hex> <when> <bytes hex> <mask hex>
    check megalo_trait_set <i>
_catalog.txt: one line "field <offset hex> <size> <mask hex> <when>" for every field of the catalog.
An error is reported as file:line: what was expected, and invalidates the whole preset. Exit code 0 ok, 1 errors or differences.
"""
import argparse, hashlib, os, re, sys
from xml.parsers import expat
from xml.sax.saxutils import quoteattr

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PRESETS = os.path.join(ROOT, "Data", "Presets")
CATALOG = os.path.join(PRESETS, "Catalog.xml")
OUT = os.path.join(ROOT, "build", "game", "presets")
WHENS = ("tick", "spawn", "pickup", "round_start", "round_end", "load")
SIZES = {"u8": 1, "s8": 1, "u16": 2, "s16": 2, "u32": 4, "s32": 4, "bit": 1}
NAME = re.compile(r"^[a-z0-9_]{1,31}$")
IDENT = re.compile(r"^[a-z][a-z0-9_]*$")
CATALOG_FILE = "_catalog"


class PresetError(Exception):
    def __init__(self, path, line, message):
        super().__init__(f"{Relative(path)}:{line}: {message}")


def Relative(path):
    try:
        return os.path.relpath(path, ROOT).replace(os.sep, "/")
    except ValueError:
        return path


class Node:
    def __init__(self, tag, attrs, line):
        self.Tag, self.Attrs, self.Line, self.Children = tag, attrs, line, []


def ParseXml(path):
    """Tree of Nodes with the line of every element."""
    parser = expat.ParserCreate()
    stack, roots = [], []

    def Start(tag, attrs):
        node = Node(tag, attrs, parser.CurrentLineNumber)
        (stack[-1].Children if stack else roots).append(node)
        stack.append(node)

    parser.StartElementHandler = Start
    parser.EndElementHandler = lambda tag: stack.pop()
    try:
        with open(path, "rb") as handle:
            parser.Parse(handle.read(), True)
    except expat.ExpatError as error:
        raise PresetError(path, error.lineno, f"invalid XML ({expat.ErrorString(error.code)})")
    except OSError as error:
        raise PresetError(path, 0, f"cannot be read ({error.strerror})")
    if len(roots) != 1:
        raise PresetError(path, 1, "expected exactly one root element")
    return roots[0]


def Number(text):
    """Integer of a decimal or 0x literal, or None."""
    if re.fullmatch(r"[+-]?(0[xX][0-9a-fA-F]+|[0-9]+)", text.strip()):
        return int(text.strip(), 0)
    return None


def Range(kind):
    size = SIZES[kind] * 8
    return (-(1 << (size - 1)), (1 << (size - 1)) - 1) if kind.startswith("s") else (0, (1 << size) - 1)


class Field:
    """A field of a group, with the offset relative to the group."""

    def __init__(self, node, path, group_when, enums, traits):
        a = node.Attrs
        self.Line = node.Line
        self.Name = self.Need(a, "name", path, node.Line)
        if not IDENT.match(self.Name):
            raise PresetError(path, node.Line, f"field name '{self.Name}' is not snake_case")
        self.Kind = self.Need(a, "type", path, node.Line)
        if self.Kind not in SIZES:
            raise PresetError(path, node.Line, f"type '{self.Kind}' of field '{self.Name}' is unknown (expected one of {', '.join(SIZES)})")
        offset = Number(self.Need(a, "offset", path, node.Line))
        if offset is None or offset < 0:
            raise PresetError(path, node.Line, f"offset of field '{self.Name}' is not a number")
        self.Offset = offset
        self.Size = SIZES[self.Kind]
        self.Bit = Number(a.get("bit", "0"))
        self.Width = Number(a.get("width", "1"))
        if self.Kind == "bit":
            if self.Bit is None or self.Width is None or self.Width < 1 or self.Bit < 0 or self.Bit + self.Width > 8:
                raise PresetError(path, node.Line, f"bit/width of field '{self.Name}' must fit one byte")
            self.Mask = bytes([((1 << self.Width) - 1) << self.Bit])
        else:
            if "bit" in a or "width" in a:
                raise PresetError(path, node.Line, f"field '{self.Name}' has bit/width but its type is not bit")
            self.Mask = b"\xff" * self.Size
        self.Enum = a.get("enum")
        if self.Enum is not None:
            if self.Enum not in enums:
                raise PresetError(path, node.Line, f"enum '{self.Enum}' of field '{self.Name}' is not defined")
            low, high = Range(self.Kind) if self.Kind != "bit" else (0, (1 << self.Width) - 1)
            for name, value in enums[self.Enum].items():
                if not low <= value <= high:
                    raise PresetError(path, node.Line, f"value {value} ({name}) of enum '{self.Enum}' is outside the range of {self.Kind} of '{self.Name}'")
        self.When = a.get("when", group_when)
        if self.When and self.When not in WHENS:
            raise PresetError(path, node.Line, f"field '{self.Name}' needs when in {{{', '.join(WHENS)}}}, got '{self.When}'")
        self.Doc = a.get("doc", "")
        self.Unit = a.get("unit", "")

    @staticmethod
    def Need(attrs, name, path, line):
        if name not in attrs:
            raise PresetError(path, line, f"missing attribute '{name}'")
        return attrs[name]


class Group:
    def __init__(self, node, path, enums, traits_fields):
        a = node.Attrs
        self.Line = node.Line
        self.Name = Field.Need(a, "name", path, node.Line)
        if not IDENT.match(self.Name):
            raise PresetError(path, node.Line, f"group name '{self.Name}' is not snake_case")
        offset = Number(a.get("offset", "0"))
        if offset is None:
            raise PresetError(path, node.Line, f"offset of group '{self.Name}' is not a number")
        self.Offset = offset
        self.When = a.get("when", "")
        self.Check = a.get("check", "")
        self.Traits = a.get("traits") == "true"
        self.Doc = a.get("doc", "")
        self.Indices = []
        for child in node.Children:
            if child.Tag != "index":
                continue
            count, stride = Number(child.Attrs.get("count", "")), Number(child.Attrs.get("stride", ""))
            if not child.Attrs.get("name") or count is None or count < 1 or stride is None or stride < 1:
                raise PresetError(path, child.Line, "an <index> needs name, count >= 1 and stride >= 1")
            self.Indices.append((child.Attrs["name"], count, stride))
        if a.get("traits") == "true":
            self.Fields = traits_fields
        else:
            if self.When and self.When not in WHENS:
                raise PresetError(path, node.Line, f"group '{self.Name}' needs when in {{{', '.join(WHENS)}}}, got '{self.When}'")
            self.Fields = [Field(c, path, self.When, enums, None) for c in node.Children if c.Tag == "field"]
            for field in self.Fields:
                if not field.When:
                    raise PresetError(path, field.Line, f"field '{field.Name}' needs when in {{{', '.join(WHENS)}}} (it or its group)")
        if a.get("traits") == "true" and self.When not in WHENS:
            raise PresetError(path, node.Line, f"traits group '{self.Name}' needs when in {{{', '.join(WHENS)}}}")
        self.ByName = {}
        for field in self.Fields:
            if field.Name in self.ByName:
                raise PresetError(path, field.Line, f"field '{field.Name}' is repeated in group '{self.Name}'")
            self.ByName[field.Name] = field

    def Instances(self):
        """Every combination of indices: (tuple of index values, byte offset of that instance)."""
        combos = [((), self.Offset)]
        for _, count, stride in self.Indices:
            combos = [(values + (i,), base + i * stride) for values, base in combos for i in range(count)]
        return combos


class Catalog:
    def __init__(self, path=CATALOG):
        self.Path = path
        root = ParseXml(path)
        if root.Tag != "catalog":
            raise PresetError(path, root.Line, "expected <catalog>")
        self.Enums = {}
        for node in root.Children:
            if node.Tag != "enum":
                continue
            name = Field.Need(node.Attrs, "name", path, node.Line)
            values = {}
            for child in node.Children:
                key, value = child.Attrs.get("name"), Number(child.Attrs.get("value", ""))
                if not key or value is None:
                    raise PresetError(path, child.Line, "a <value> needs name and a numeric value")
                if key in values:
                    raise PresetError(path, child.Line, f"value '{key}' is repeated in enum '{name}'")
                values[key] = value
            if name in self.Enums:
                raise PresetError(path, node.Line, f"enum '{name}' is defined twice")
            self.Enums[name] = values
        traits_nodes = [n for n in root.Children if n.Tag == "traits"]
        if len(traits_nodes) != 1:
            raise PresetError(path, root.Line, "expected exactly one <traits> block")
        traits = [Field(c, path, "", self.Enums, None) for c in traits_nodes[0].Children if c.Tag == "field"]
        self.Groups = {}
        for node in root.Children:
            if node.Tag != "group":
                continue
            group = Group(node, path, self.Enums, traits)
            if group.Name in self.Groups:
                raise PresetError(path, node.Line, f"group '{group.Name}' is defined twice")
            self.Groups[group.Name] = group
        self.Expanded = list(self.Expand())
        self.CheckOverlaps()

    def Expand(self):
        """(group, index values, field, absolute offset, when) for every field of every instance."""
        for group in self.Groups.values():
            for values, base in group.Instances():
                for field in group.Fields:
                    yield group, values, field, base + field.Offset, field.When or group.When

    def CheckOverlaps(self):
        owner = {}
        for group, values, field, offset, _ in self.Expanded:
            for i in range(field.Size):
                bits = field.Mask[i]
                if owner.get(offset + i, (0, ""))[0] & bits:
                    raise PresetError(self.Path, field.Line, f"field '{group.Name}.{field.Name}' overlaps another field at offset 0x{offset + i:X}")
                previous = owner.get(offset + i, (0, ""))
                owner[offset + i] = (previous[0] | bits, group.Name)

    def Text(self):
        lines = sorted((offset, field.Mask.hex(), field.Size, when) for _, _, field, offset, when in self.Expanded)
        return "".join(f"field {offset:x} {size} {mask} {when}\n" for offset, mask, size, when in lines)


def Encode(field, text, path, line, enums):
    """Bytes of the value written in the XML for a field (name of the enum or a number)."""
    names = enums.get(field.Enum, {}) if field.Enum else {}
    if text in names:
        value = names[text]
    else:
        value = {"true": 1, "false": 0}.get(text) if field.Kind == "bit" else None
        if value is None:
            value = Number(text)
        if value is None:
            hint = f" or one of: {', '.join(names)}" if names else ""
            raise PresetError(path, line, f"'{text}' is not a valid value for '{field.Name}' (expected a number{hint})")
    low, high = (0, (1 << field.Width) - 1) if field.Kind == "bit" else Range(field.Kind)
    if field.Kind == "u32":
        low = -(1 << 31)
    if not low <= value <= high:
        raise PresetError(path, line, f"{value} is outside the range of '{field.Name}' ({low} to {high})")
    if field.Kind == "bit":
        return bytes([value << field.Bit])
    return (value & ((1 << (8 * field.Size)) - 1)).to_bytes(field.Size, "little")


def ReadPreset(path, catalog):
    """Own settings of one XML: ({(offset, mask hex): (offset, size, when, bytes, mask)}, set of trait set checks, base name, doc, root line)."""
    root = ParseXml(path)
    if root.Tag != "preset":
        raise PresetError(path, root.Line, "expected <preset>")
    for name in root.Attrs:
        if name not in ("base", "doc"):
            raise PresetError(path, root.Line, f"unknown attribute '{name}' in <preset> (expected base or doc)")
    writes, checks, seen = {}, set(), set()
    for node in root.Children:
        group = catalog.Groups.get(node.Tag)
        if group is None:
            raise PresetError(path, node.Line, f"unknown element <{node.Tag}> (expected one of: {', '.join(catalog.Groups)})")
        attrs = dict(node.Attrs)
        values = []
        for name, count, _ in group.Indices:
            if name not in attrs:
                raise PresetError(path, node.Line, f"<{node.Tag}> needs the attribute '{name}' (0 to {count - 1})")
            number = Number(attrs.pop(name))
            if number is None or not 0 <= number < count:
                raise PresetError(path, node.Line, f"'{name}' of <{node.Tag}> must be a number from 0 to {count - 1}")
            values.append(number)
        if (node.Tag, tuple(values)) in seen:
            raise PresetError(path, node.Line, f"<{node.Tag}> {tuple(values) or ''} is written twice in the same preset")
        seen.add((node.Tag, tuple(values)))
        base = group.Offset + sum(v * s for v, (_, _, s) in zip(values, group.Indices))
        for name, text in attrs.items():
            field = group.ByName.get(name)
            if field is None:
                raise PresetError(path, node.Line, f"unknown attribute '{name}' in <{node.Tag}> (expected one of: {', '.join(group.ByName)})")
            offset = base + field.Offset
            writes[(offset, field.Mask.hex())] = (offset, field.Size, field.When or group.When, Encode(field, text, path, node.Line, catalog.Enums), field.Mask)
        if group.Check:
            checks.add((group.Check, values[0]))
    return writes, checks, root.Attrs.get("base", ""), root.Attrs.get("doc", ""), root.Line


def Normalized(path):
    with open(path, "rb") as handle:
        return handle.read().replace(b"\r\n", b"\n").rstrip(b"\n")


def CompilePreset(name, folder, catalog, cache=None, paths=None):
    """Compiled text of one preset, or raises PresetError. Returns (text, base, doc, number of writes). paths = {preset name: file} reads
    that file instead of <folder>/<name>.xml (the web builder compiles a candidate before it replaces the preset)."""
    chain, writes, checks = [], {}, set()
    current, trail = name, []
    first_line = 1
    while current:
        path = (paths or {}).get(current) or os.path.join(folder, current + ".xml")
        if current in trail:
            raise PresetError(os.path.join(folder, trail[-1] + ".xml"), first_line, f"the chain of bases has a cycle ({' -> '.join(trail + [current])})")
        if not os.path.isfile(path):
            raise PresetError(os.path.join(folder, trail[-1] + ".xml"), first_line, f"base '{current}' does not exist (expected {Relative(path)})")
        trail.append(current)
        own, own_checks, base, doc, first_line = ReadPreset(path, catalog)
        chain.append((current, path, own, own_checks))
        if len(chain) == 1:
            top = (base, doc)
        current = base
        if current and not NAME.match(current):
            raise PresetError(path, first_line, f"base '{current}' is not a valid preset name ([a-z0-9_], up to 31 characters)")
    for _, _, own, own_checks in reversed(chain):
        writes.update(own)
        checks |= own_checks
    digest = hashlib.sha1()
    for link, path, _, _ in chain:
        digest.update(link.encode() + b"\n" + Normalized(path))
    lines = [f"preset {name}", f"source {digest.hexdigest()}"]
    for offset, size, when, data, mask in sorted(writes.values(), key=lambda w: (w[0], w[4])):
        lines.append(f"write {offset:x} {when} {data.hex()} {mask.hex()}")
    lines += [f"check {kind} {index}" for kind, index in sorted(checks)]
    return "\n".join(lines) + "\n", top[0], top[1], len(writes)


KEY = re.compile(r"^([a-z][a-z0-9_]*)(?:\[(\d+(?:,\d+)*)\])?\.([a-z][a-z0-9_]*)$")


def ValueKey(group, values, field):
    """Name of one value of a preset: 'general.round_limit', 'team[0].primary_color', 'loadout[1,2].primary_weapon'."""
    return f"{group}[{','.join(str(v) for v in values)}].{field}" if values else f"{group}.{field}"


def ParseKey(catalog, key):
    """(group, index values, field) of a ValueKey. Raises ValueError when the group, the indices or the field are not in the catalog."""
    found = KEY.fullmatch(key) if isinstance(key, str) else None
    group = catalog.Groups.get(found.group(1)) if found else None
    if group is None:
        raise ValueError(f"'{key}' is not a value of a preset (expected group.field or group[index].field of the catalog)")
    values = tuple(int(v) for v in found.group(2).split(",")) if found.group(2) else ()
    if len(values) != len(group.Indices) or any(not 0 <= v < count for v, (_, count, _) in zip(values, group.Indices)):
        raise ValueError(f"'{key}': group '{group.Name}' needs the indices {', '.join(f'{n} (0 to {c - 1})' for n, c, _ in group.Indices) or 'none'}")
    if found.group(3) not in group.ByName:
        raise ValueError(f"'{key}': group '{group.Name}' has no field '{found.group(3)}' (expected one of: {', '.join(group.ByName)})")
    return group, values, found.group(3)


def ReadOwn(path, catalog):
    """The values one XML sets as they are written (enum name or number): ({ValueKey: text}, base name, doc)."""
    root = ParseXml(path)
    if root.Tag != "preset":
        raise PresetError(path, root.Line, "expected <preset>")
    own = {}
    for node in root.Children:
        group = catalog.Groups.get(node.Tag)
        if group is None:
            raise PresetError(path, node.Line, f"unknown element <{node.Tag}> (expected one of: {', '.join(catalog.Groups)})")
        attrs, values = dict(node.Attrs), []
        for name, count, _ in group.Indices:
            number = Number(attrs.pop(name, ""))
            if number is None or not 0 <= number < count:
                raise PresetError(path, node.Line, f"'{name}' of <{node.Tag}> must be a number from 0 to {count - 1}")
            values.append(number)
        for name, text in attrs.items():
            if name not in group.ByName:
                raise PresetError(path, node.Line, f"unknown attribute '{name}' in <{node.Tag}> (expected one of: {', '.join(group.ByName)})")
            own[ValueKey(node.Tag, values, name)] = text
    return own, root.Attrs.get("base", ""), root.Attrs.get("doc", "")


def Resolve(name, folder, catalog, paths=None):
    """The values a preset ends up with and where each comes from: {ValueKey: (text, name of the preset of the chain that sets it)}. The
    nearest preset of the chain of bases wins, as in CompilePreset. The chain has to compile (a cycle just stops)."""
    chain, current = [], name
    while current and current not in [link for link, _ in chain]:
        own, base, _ = ReadOwn((paths or {}).get(current) or os.path.join(folder, current + ".xml"), catalog)
        chain.append((current, own))
        current = base
    resolved = {}
    for link, own in reversed(chain):
        resolved.update({key: (text, link) for key, text in own.items()})
    return resolved


def PresetXml(base, doc, own, catalog, comment=""):
    """XML of a preset from its own values {ValueKey: text}, in the format of the files of the repo: one element per group and index
    (catalog order, the indices first, then the fields in the order they come in `own`, so a preset read with ReadOwn is written as it was), LF lines and no final newline. comment is kept as written.
    Raises ValueError for a key that is not in the catalog."""
    order = list(catalog.Groups)
    elements = {}
    for key, text in own.items():
        group, values, field = ParseKey(catalog, key)
        elements.setdefault((order.index(group.Name), values), {})[field] = str(text)
    root = "".join(f" {name}={quoteattr(value)}" for name, value in (("base", base), ("doc", doc)) if value)
    lines = ['<?xml version="1.0" encoding="utf-8"?>'] + ([comment] if comment else []) + [f"<preset{root}>"]
    for (index, values), fields in sorted(elements.items()):
        group = catalog.Groups[order[index]]
        parts = [f"{name}={quoteattr(str(v))}" for (name, _, _), v in zip(group.Indices, values)]
        parts += [f"{name}={quoteattr(text)}" for name, text in fields.items()]
        lines.append(f"  <{group.Name} {' '.join(parts)} />")
    return "\n".join(lines + ["</preset>"])


def Sources(folder):
    """Preset names (files except the catalog) of a folder, and the invalid ones."""
    names, errors = [], []
    for file in sorted(os.listdir(folder)):
        if not file.endswith(".xml") or file == "Catalog.xml":
            continue
        name = file[:-4]
        if NAME.match(name):
            names.append(name)
        else:
            errors.append(f"{Relative(os.path.join(folder, file))}:1: the file name '{name}' is not a valid preset name ([a-z0-9_], up to 31 characters)")
    return names, errors


def Compile(folder=PRESETS, catalog_path=None):
    """{file name: text} of every preset that compiles and of the catalog, plus the error lines and the info of each preset."""
    catalog = Catalog(catalog_path or os.path.join(folder, "Catalog.xml"))
    names, errors = Sources(folder)
    files, info = {CATALOG_FILE + ".txt": catalog.Text()}, {}
    for name in names:
        try:
            text, base, doc, count = CompilePreset(name, folder, catalog)
        except PresetError as error:
            errors.append(str(error))
            continue
        files[name + ".txt"] = text
        info[name] = (base, doc, count)
    return files, errors, info


def Write(path, text, eol):
    if eol == "repo":
        data = text.rstrip("\n").replace("\n", "\r\n").encode("ascii")
    else:
        data = text.encode("ascii")
    with open(path + ".tmp", "wb") as handle:
        handle.write(data)
    os.replace(path + ".tmp", path)


def Plain(data):
    return data.replace(b"\r\n", b"\n").rstrip(b"\n")


def Run(command, out, folder, catalog_path, check, eol):
    try:
        files, errors, info = Compile(folder, catalog_path)
    except PresetError as error:
        print(error)
        return 1
    for line in errors:
        print(line)
    if command == "list":
        for name, (base, doc, count) in info.items():
            print(f"{name}: {count} writes" + (f", base {base}" if base else "") + (f" - {doc}" if doc else ""))
        return 1 if errors else 0
    if check:
        differences = []
        existing = sorted(f for f in os.listdir(out) if f.endswith(".txt")) if os.path.isdir(out) else []
        for file, text in files.items():
            path = os.path.join(out, file)
            if not os.path.isfile(path):
                differences.append(f"{file}: missing")
            else:
                with open(path, "rb") as handle:
                    if Plain(handle.read()) != Plain(text.encode("ascii")):
                        differences.append(f"{file}: differs from the XML")
        differences += [f"{file}: has no XML" for file in existing if file not in files]
        for line in differences:
            print(line)
        print(f"presets --check: {'OK' if not differences and not errors else 'DIFFERENT'} ({len(files)} files)")
        return 1 if differences or errors else 0
    os.makedirs(out, exist_ok=True)
    for file, text in files.items():
        Write(os.path.join(out, file), text, eol)
    for file in sorted(f for f in os.listdir(out) if f.endswith(".txt") and f not in files):
        try:
            os.remove(os.path.join(out, file))
        except OSError:
            print(f"{file}: has no XML and could not be removed")
    print(f"presets: {len(files) - 1} compiled -> {Relative(out)}" + (f", {len(errors)} errors" if errors else ""))
    return 1 if errors else 0


def Export():
    """Compiles Data/Presets into build/game/presets. Called by game.py deploy and run, like Export of the scenarios."""
    return Run("compile", OUT, PRESETS, None, False, "lf")


def Main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", nargs="?", choices=["compile", "list"], default="compile")
    parser.add_argument("--out", default=OUT)
    parser.add_argument("--presets", default=PRESETS)
    parser.add_argument("--catalog")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--eol", choices=["lf", "repo"], default="lf")
    args = parser.parse_args(argv)
    return Run(args.command, args.out, args.presets, args.catalog, args.check, args.eol)


if __name__ == "__main__":
    sys.exit(Main())
