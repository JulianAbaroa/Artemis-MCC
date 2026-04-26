import xml.etree.ElementTree as ET
import re
import os
from dataclasses import dataclass, field as dc_field

IGNORED_TAGS = {"revisions", "revision", "comment"}

CPP_KEYWORDS = {
    "alignas","alignof","and","and_eq","asm","auto","bitand","bitor","bool",
    "break","case","catch","char","char8_t","char16_t","char32_t","class",
    "compl","concept","const","consteval","constexpr","constinit","const_cast",
    "continue","co_await","co_return","co_yield","decltype","default","delete",
    "do","double","dynamic_cast","else","enum","explicit","export","extern",
    "false","float","for","friend","goto","if","inline","int","long","mutable",
    "namespace","new","noexcept","not","not_eq","nullptr","operator","or",
    "or_eq","private","protected","public","register","reinterpret_cast",
    "requires","return","short","signed","sizeof","static","static_assert",
    "static_cast","struct","switch","template","this","thread_local","throw",
    "true","try","typedef","typeid","typename","union","unsigned","using",
    "virtual","void","volatile","wchar_t","while","xor","xor_eq",
}

# Naming, single source of truth.

def Clean(name: str) -> str:
    return re.sub(r"[\u2018\u2019\u201c\u201d'\"´`]", "", name)

def ToPascal(name: str) -> str:
    s = Clean(name)
    words = re.sub(r"[^a-zA-Z0-9]+", " ", s).split()
    result = "".join(w.capitalize() for w in words)
    if result and result[0].isdigit():
        result = "_" + result
    return result

def ToOffsetIdent(name: str) -> str:
    pascal = ToPascal(name)
    if pascal in CPP_KEYWORDS:
        pascal += "_"
    return f"k_{pascal}"

def ToFieldName(name: str) -> str:
    pascal = ToPascal(name)
    if pascal in CPP_KEYWORDS:
        pascal += "_"
    return pascal

def ParseHex(s: str) -> int:
    s = s.strip()
    try:
        return int(s, 16) if "0x" in s.lower() else int(s)
    except Exception:
        return 0

def XmlBaseName(xmlPath: str) -> str:
    base = os.path.splitext(os.path.basename(xmlPath))[0]
    base = re.sub(r"^ReachMCC_", "", base, flags=re.IGNORECASE)
    return ToPascal(base)

# Block tree, shared between GenerateType and GenerateObject.

@dataclass
class BlockNode:
    XmlName:      str
    Pascal:       str
    FieldName:    str
    Offset:       int
    ElementSize:  int
    DiskStruct:   str
    ObjectStruct: str
    Children:     list = dc_field(default_factory=list)

    @property
    def HasChildren(self) -> bool:
        return bool(self.Children)

    @property
    def VectorElementType(self) -> str:
        return self.ObjectStruct if self.HasChildren else self.DiskStruct


def BuildBlockTree(element: ET.Element, diskPrefix: str) -> list:
    nodes = []
    allFieldCounter: dict[str, int] = {}
    structCounter: dict[str, int] = {}

    totalSize = ParseHex(element.get("baseSize", element.get("elementSize", "0x0")))

    childrenInfo = []
    for child in element:
        tag = child.tag.lower()
        if tag in IGNORED_TAGS:
            continue
        childrenInfo.append((tag, child, ParseHex(child.get("offset", "0x0"))))

    occupiedOffsets = set()

    for i, (tag, child, offset) in enumerate(childrenInfo):
        xmlName = child.get("name", "unknown")
        base    = ToFieldName(xmlName)

        allFieldCounter[base] = allFieldCounter.get(base, 0) + 1
        n = allFieldCounter[base]
        fieldName = base if n == 1 else f"{base}_{n}"

        # We avoid overlays.
        if offset in occupiedOffsets:
            continue
        occupiedOffsets.add(offset)

        if tag != "tagblock":
            continue

        # Calculate actual available space by skipping overlays (same offset).
        available = 0
        for j in range(i + 1, len(childrenInfo)):
            nxt = childrenInfo[j][2]
            if nxt > offset:
                available = nxt - offset
                break
        else:
            available = (totalSize - offset) if totalSize > offset else 0

        # If the tagblock is less than 12 bytes, it is truncated. 
        # It should not be included in the tree.
        if 0 < available < 12:
            continue

        pascal      = ToPascal(xmlName)
        elementSize = ParseHex(child.get("elementSize", "0x0"))

        structCounter[pascal] = structCounter.get(pascal, 0) + 1
        m = structCounter[pascal]
        structSuffix = "" if m == 1 else f"_{m}"

        diskStruct   = f"{diskPrefix}_{pascal}Entry{structSuffix}"
        children     = BuildBlockTree(child, f"{diskPrefix}_{pascal}{structSuffix}")
        objectStruct = f"{diskPrefix}_{pascal}Object{structSuffix}" if children else ""

        nodes.append(BlockNode(
            XmlName=xmlName,
            Pascal=pascal,
            FieldName=fieldName,
            Offset=offset,
            ElementSize=elementSize,
            DiskStruct=diskStruct,
            ObjectStruct=objectStruct,
            Children=children,
        ))

    return nodes