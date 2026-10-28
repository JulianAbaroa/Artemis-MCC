"""
GenerateDatum.py
Generates the runtime ("dynamic") datum layouts as C++23 module partitions of
Datum.Object.Type from the public XML files in Data/Datum/.

    python Tools/TagTranspiler/GenerateDatum.py                    # every Data/Datum/*.xml
    python Tools/TagTranspiler/GenerateDatum.py Data/Datum/unit.xml
    python Tools/TagTranspiler/GenerateDatum.py --out DIR          # write under DIR instead of the repo
    python Tools/TagTranspiler/GenerateDatum.py --check            # compare with the repo, write nothing there (exit 0 equal, 1 not)
    python Tools/TagTranspiler/GenerateDatum.py --check --out DIR  # compare with DIR

The files are always written with CRLF line endings and no final newline, whatever the OS (D176).

Per XML (obje.xml -> prefix Obje) it writes, under
Artemis/2_Blam/Layout/Datum/Type/:
    Obje/DatumObjeOffset.ixx     export module Datum.Object.Type:Obje.Offset;
    Obje/DatumObjeStructure.ixx  export module Datum.Object.Type:Obje.Structure;
    Datum.Object.Type.Obje.ixx   aggregate (partition :Obje)
File names carry the "Datum" prefix so they never collide with the Map* files of
layer 1 (same .ixx name twice in one project overwrites the .obj); module partition
names and namespaces are unaffected.

XML specifics (on top of the Assembly format):
  start     first datum byte of the component. Offsets stay ABSOLUTE in the Offset
            partition; the Structure partition is relative to `start` (the struct
            is the component only, so UnitData is 0x820 bytes, not 0x9D8).
  baseSize  END of the component (absolute).
  confidence / exposed / default  documentation attributes (ignored here).
It reuses GenerateOffset.py / GenerateStructure.py and rewrites module + namespace.
"""
import os, sys, glob, shutil, tempfile, argparse, io, contextlib
import xml.etree.ElementTree as ET

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
import GenerateOffset, GenerateStructure
from Common import XmlBaseName, ParseHex, CompareTrees, ReportCheck

REPO = os.path.normpath(os.path.join(SCRIPT_DIR, "..", ".."))
DATUM_XML = os.path.join(REPO, "Data", "Datum")
OUT = os.path.join(REPO, "Artemis", "2_Blam", "Layout", "Datum", "Type")

def Finish(text: str, xmlName: str) -> str:
    text = text.replace("export module Map.Tag.Type:", "export module Datum.Object.Type:")
    text = text.replace("Map::Tag::Type::", "Datum::Object::Type::")
    text = text.replace("(GenerateOffset.py)", "(GenerateDatum.py)")
    text = text.replace("(GenerateStructure.py)", "(GenerateDatum.py)")
    # rules.txt: CRLF line endings, no final newline.
    return text.rstrip("\r\n").replace("\r\n", "\n").replace("\n", "\r\n")

def Write(path: str, text: str) -> None:
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(text)
    print(f"  Generated: {path}")

def ProcessXml(xmlPath: str, out: str) -> None:
    prefix = XmlBaseName(xmlPath)
    xmlName = os.path.basename(xmlPath)
    os.makedirs(out, exist_ok=True)
    print(f"[{prefix}] {xmlName}")

    tree = ET.parse(xmlPath)
    root = tree.getroot()
    start = ParseHex(root.get("start", "0x0"))
    end = ParseHex(root.get("baseSize", "0x0"))
    if end <= start:
        raise SystemExit(f"{xmlName}: baseSize must be greater than start")

    tmp = tempfile.mkdtemp(prefix="tpl_")
    try:
        # Offset partition: absolute offsets, as written in the XML.
        tmpOff = os.path.join(tmp, "off.ixx")
        GenerateOffset.Generate(xmlPath, tmpOff)

        # Structure partition: offsets relative to the component start.
        rel = ET.fromstring(ET.tostring(root))
        for el in rel.iter():
            if el is not rel and el.get("offset"):
                el.set("offset", "0x%X" % (ParseHex(el.get("offset")) - start))
        rel.set("baseSize", "0x%X" % (end - start))
        relXml = os.path.join(tmp, xmlName)
        ET.ElementTree(rel).write(relXml, encoding="utf-8")
        tmpStr = os.path.join(tmp, "str.ixx")
        GenerateStructure.Generate(relXml, tmpStr)

        with open(tmpOff, encoding="utf-8") as f:
            off = f.read()
        with open(tmpStr, encoding="utf-8") as f:
            st = f.read()
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    off = off.replace(f"// Base size: 0x{end:X}.", f"// Component range: 0x{start:X}..0x{end:X} (absolute datum offsets).")
    # Range of the component, so tools can cut a datum by component without repeating the XML.
    marker = "::Offset\n{\n"
    if marker not in off.replace("\r\n", "\n"):
        raise SystemExit(f"{xmlName}: Offset namespace not found")
    off = off.replace("\r\n", "\n").replace(marker, marker +
        "    // First datum byte of the component and the byte after its last one.\n"
        f"    constexpr std::size_t k_ComponentStart = 0x{start:X};\n"
        f"    constexpr std::size_t k_ComponentEnd = 0x{end:X};\n\n", 1)
    st = st.replace("// Root struct size:", f"// Component starts at datum offset 0x{start:X}; field offsets in comments are relative to it.\n// Root struct size:")
    compDir = os.path.join(out, prefix)
    os.makedirs(compDir, exist_ok=True)
    Write(os.path.join(compDir, f"Datum{prefix}Offset.ixx"), Finish(off, xmlName))
    Write(os.path.join(compDir, f"Datum{prefix}Structure.ixx"), Finish(st, xmlName))

    agg = "\n".join([
        "// Auto-generated by TagTranspiler (GenerateDatum.py)",
        f"// Source XML: {xmlName}",
        "",
        f"export module Datum.Object.Type:{prefix};",
        "",
        f"export import :{prefix}.Offset;",
        f"export import :{prefix}.Structure;",
    ])
    Write(os.path.join(out, f"Datum.Object.Type.{prefix}.ixx"), Finish(agg, xmlName))

def Check(files: list, target: str) -> int:
    """Generates into a temporary folder and compares it, byte by byte, with target. Writes nothing in target."""
    tmp = tempfile.mkdtemp(prefix="gendatum_")
    try:
        with contextlib.redirect_stdout(io.StringIO()):
            for p in files:
                ProcessXml(p, tmp)
        different, missing = CompareTrees(tmp, target)
        total = sum(len(names) for _, _, names in os.walk(tmp))
        return ReportCheck(different, missing, target, total)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

def main() -> None:
    parser = argparse.ArgumentParser(description="Generates the datum layouts (Datum.Object.Type) from Data/Datum/*.xml.")
    parser.add_argument("xml", nargs="*", help="XML files (default: every Data/Datum/*.xml)")
    parser.add_argument("--out", metavar="DIR", default=OUT, help="output root (default: the repo)")
    parser.add_argument("--check", action="store_true", help="compare with the output root and write nothing there")
    args = parser.parse_args()
    files = args.xml or sorted(glob.glob(os.path.join(DATUM_XML, "*.xml")))
    if not files:
        print("No XML found.")
        sys.exit(1)
    out = os.path.abspath(args.out)
    if args.check:
        sys.exit(Check(files, out))
    for p in files:
        ProcessXml(p, out)

if __name__ == "__main__":
    main()
