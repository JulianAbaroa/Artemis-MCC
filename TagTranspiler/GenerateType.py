"""
GenerateType.py
Generates {Group}Types.h from an Assembly XML plugin.
This file contains the raw data structures (structs) and bitfields
that represent the flat memory layout of th tag blocks.

Usage:
    python GenerateType.py input.xml output.h
"""


import xml.etree.ElementTree as ET
import re, sys, os
from dataclasses import dataclass, field as dc_field
from typing import Optional
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from Common import ToPascal, ToFieldName, ParseHex, IGNORED_TAGS, XmlBaseName

TYPE_INFO: dict[str, tuple[str, int]] = {
    "int8":     ("int8_t",       1),
    "int16":    ("int16_t",      2),
    "int32":    ("int32_t",      4),
    "int64":    ("int64_t",      8),
    "uint8":    ("uint8_t",      1),
    "uint16":   ("uint16_t",     2),
    "uint32":   ("uint32_t",     4),
    "uint64":   ("uint64_t",     8),
    "float32":  ("float",        4),
    "float64":  ("double",       8),
    "degree":   ("float",        4),
    "flags8":   ("uint8_t",      1),
    "flags16":  ("uint16_t",     2),
    "flags32":  ("uint32_t",     4),
    "flags64":  ("uint64_t",     8),
    "enum8":    ("uint8_t",      1),
    "enum16":   ("uint16_t",     2),
    "enum32":   ("uint32_t",     4),
    "stringid": ("uint32_t",     4),
    "vector3":  ("Vec3",        12),
    "point3":   ("Vec3",        12),
    "degree3":  ("Vec3",        12),
    "vector4":  ("Vec4",        16),
    "point2":   ("Vec2",         8),
    "colorf":   ("ColorRGB",    12),
    "rangef":   ("RangeF",       8),
    "undefined":("_pad_",        4),
    "tagblock": ("_TagBlock_",  12),
    "tagref":   ("Map_TagRef",  16),
    "dataref":  ("Map_DataRef", 20),
}

@dataclass
class Field:
    Offset:      int
    Size:        int
    CType:       str
    Name:        str
    Comment:     str = ""
    EntryStruct: Optional[str] = None

@dataclass
class Struct:
    Name:      str
    TotalSize: int
    Fields:    list = dc_field(default_factory=list)

def BuildComment(child: ET.Element, tag: str) -> str:
    tooltip = child.get("tooltip", "")
    parts   = [f"{tag}."]
    options = []
    for sub in child:
        stag = sub.tag.lower()
        if stag == "bit":
            options.append(f"{sub.get('name','?')}={sub.get('index','?')}")
        elif stag == "option":
            options.append(f"{sub.get('name','?')}={sub.get('value','?')}")
    if options:
        shown  = options[:16]
        suffix = f", ...({len(options)} total)" if len(options) > 16 else ""
        parts.append(f" [{', '.join(shown)}{suffix}]")
    if tooltip:
        parts.append(f" | {tooltip}")
    return "".join(parts)

class Parser:
    def __init__(self):
        self.Structs:    list[Struct] = []
        self._NameCount: dict[str, int] = {}

    def _UniqueName(self, candidate: str) -> str:
        n = self._NameCount.get(candidate, 0)
        self._NameCount[candidate] = n + 1
        return candidate if n == 0 else f"{candidate}_{n + 1}"

    def _ProcessChildren(self, element: ET.Element, structName: str, totalSize: int, childPrefix: Optional[str] = None) -> Struct:
        struct = Struct(Name=structName, TotalSize=totalSize)
        nameCounter: dict[str, int] = {}

        childrenInfo = []
        for child in element:
            tag = child.tag.lower()
            if tag in IGNORED_TAGS:
                continue
            childrenInfo.append((tag, child, ParseHex(child.get("offset", "0x0"))))

        occupiedOffsets = set()

        for i, (tag, child, offset) in enumerate(childrenInfo):
            rawName    = child.get("name", "unknown")
            base       = ToFieldName(rawName)
            nameCounter[base] = nameCounter.get(base, 0) + 1
            n          = nameCounter[base]
            fieldName  = base if n == 1 else f"{base}_{n}"

            if offset in occupiedOffsets:
                struct.Fields.append(Field(
                    Offset=offset, Size=0, CType="_overlay_comment_",
                    Name=fieldName,
                    Comment=f"overlay (same offset as previous field, skipped)"
                ))
                continue
            occupiedOffsets.add(offset)

            available = 0
            for j in range(i + 1, len(childrenInfo)):
                nxt = childrenInfo[j][2]
                if nxt > offset:
                    available = nxt - offset
                    break
            else:
                available = (totalSize - offset) if totalSize > offset else 0

            cType = "uint32_t"
            size = 4
            is_tagblock = False

            if tag == "tagblock":
                is_tagblock = True
                cType = "_TagBlock_"
                size = 12
            elif tag == "undefined":
                cType = "_pad_"
                size = 4
            elif tag == "ranged":
                if available >= 16 or available == 0:
                    cType, size = "RangeD", 16
                else:
                    cType, size = "RangeF", 8
            elif tag == "colorf":
                hasAlpha = child.get("alpha", "false").lower() == "true"
                cType, size = ("ColorRGBA", 16) if hasAlpha else ("ColorRGB", 12)
            elif tag in TYPE_INFO:
                cType, size = TYPE_INFO[tag]
            else:
                print(f"  [WARN] Unknown tag '{tag}' in {structName}.{fieldName} @ 0x{offset:X}", file=sys.stderr)

            # --- Dynamic truncation logic ---
            is_truncated = False
            comment = BuildComment(child, tag)
            if available > 0 and size > available:
                is_truncated = True
                comment += f" [Truncated to {available} bytes]"
                if available >= 8:   cType, size = "uint64_t", 8
                elif available >= 4: cType, size = "uint32_t", 4
                elif available >= 2: cType, size = "uint16_t", 2
                elif available >= 1: cType, size = "uint8_t", 1
                else:                cType, size = "_pad_", 0

            entryName = None
            if is_tagblock and not is_truncated:
                elementSize = ParseHex(child.get("elementSize", "0x0"))
                parentRoot = re.sub(r"(Entry(_\d+)?|Data)$", "", structName)
                childPascal = ToPascal(rawName)
                
                # Prefix resolution depending on the mode.
                # (ParseBlock vs _ParseBlockWithPrefix).
                if childPrefix is None:
                    childBase = f"{parentRoot}_{childPascal}Entry"
                    existingCount = self._NameCount.get(childBase, 0)
                    entryName = self._UniqueName(childBase)
                    nextPrefix = f"{parentRoot}_{childPascal}" if existingCount == 0 else f"{parentRoot}_{childPascal}_{existingCount + 1}"
                else:
                    childBase = f"{childPrefix}_{childPascal}Entry"
                    existingCount = self._NameCount.get(childBase, 0)
                    entryName = self._UniqueName(childBase)
                    nextPrefix = f"{childPrefix}_{childPascal}" if existingCount == 0 else f"{childPrefix}_{childPascal}_{existingCount + 1}"
                
                self.Structs.append(self._ProcessChildren(child, entryName, elementSize, nextPrefix))

            struct.Fields.append(Field(
                Offset=offset, Size=size, CType=cType,
                Name=fieldName, Comment=comment,
                EntryStruct=entryName
            ))

        return struct

    def _ParseBlockWithPrefix(self, element: ET.Element, structName: str, totalSize: int, childPrefix: str) -> Struct:
        return self._ProcessChildren(element, structName, totalSize, childPrefix)

    def ParseBlock(self, element: ET.Element, structName: str, totalSize: int) -> Struct:
        return self._ProcessChildren(element, structName, totalSize, None)

    def ParseRoot(self, root: ET.Element, rootName: str) -> Struct:
        totalSize = ParseHex(root.get("baseSize", "0x0"))
        self._NameCount[rootName] = 1
        rootStruct = self.ParseBlock(root, rootName, totalSize)
        self.Structs.append(rootStruct)
        return rootStruct

def EmitField(f: Field, cursor: int, lines: list[str]) -> int:
    if f.CType == "_overlay_comment_":
        lines.append(f"    // [overlay] {f.Name}: {f.Comment}")
        return cursor
    if f.Offset > cursor:
        lines.append(f"    uint8_t  _gap_0x{cursor:X}[0x{f.Offset - cursor:X}];")
        cursor = f.Offset
    if f.CType == "_pad_":
        lines.append(f"    uint8_t  _pad_0x{f.Offset:X}[0x4];  // {f.Comment}")
        return cursor + 4
    if f.CType == "_TagBlock_":
        lines.append(f"    // -> {f.EntryStruct}[]")
        lines.append(f"    Map_TagBlock   {f.Name};  // 0x{f.Offset:X}  {f.Comment}")
        return cursor + 12
    lines.append(f"    {f.CType:<12} {f.Name};  // 0x{f.Offset:X}  {f.Comment}")
    return cursor + f.Size

def EmitStruct(s: Struct, lines: list[str]) -> None:
    lines.append(f"// Entry size: 0x{s.TotalSize:X}")
    lines.append(f"struct {s.Name}")
    lines.append("{")
    cursor = 0
    for f in sorted(s.Fields, key=lambda x: x.Offset):
        cursor = EmitField(f, cursor, lines)
    if s.TotalSize > 0 and cursor < s.TotalSize:
        lines.append(f"    uint8_t  _tail_0x{cursor:X}[0x{s.TotalSize - cursor:X}];  // trailing pad")
    lines.append("};")
    if s.TotalSize > 0:
        lines.append(f"static_assert(sizeof({s.Name}) == 0x{s.TotalSize:X}, \"{s.Name} size mismatch\");")
    lines.append("")

def Generate(xmlPath: str, outPath: str) -> None:
    tree     = ET.parse(xmlPath)
    root     = tree.getroot()
    prefix   = XmlBaseName(xmlPath)
    rootName = prefix + "Data"
    total    = ParseHex(root.get("baseSize", "0x0"))

    parser = Parser()
    parser.ParseRoot(root, rootName)

    lines = []
    lines.append(f"// Auto-generated by GenerateType.py")
    lines.append(f"// Source: {os.path.basename(xmlPath)}")
    lines.append(f"// Root struct size: {total:#x}")
    lines.append(f"//")
    lines.append(f"")
    lines.append(f"#pragma once")
    lines.append(f"")
    lines.append(f"#include \"Core/Types/Domain/Map/MapTypes.h\"")
    lines.append(f"#include <cstdint>")
    lines.append(f"")
    lines.append(f"#pragma pack(push, 1)")
    lines.append(f"")
    for s in parser.Structs:
        EmitStruct(s, lines)
    lines.append(f"#pragma pack(pop)")
    lines.append(f"")

    with open(outPath, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print(f"Generated: {outPath}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python GenerateType.py input.xml output.h")
        sys.exit(1)
    Generate(sys.argv[1], sys.argv[2])