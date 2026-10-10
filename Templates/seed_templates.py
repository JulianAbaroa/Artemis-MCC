#!/usr/bin/env python3
"""
seed_templates.py - create / update the runtime template XML files.

The XML in Templates/Object/ is the SOURCE OF TRUTH: edit it by hand, commit it.
This script only brings in what the automatic analysis (analyze_templates.py ->
fields_<comp>.txt) discovered:

  * XML does not exist yet : creates it with every candidate as "Unknown XXX"
                             (confidence="unknown").
  * XML already exists     : MERGE. Adds candidates that do not overlap any existing
                             field and refreshes the tooltip / type guess of fields that
                             are still confidence="unknown". Fields whose confidence is
                             anything else (your labels) are never modified, renamed or
                             removed. Unknown fields that disappeared from the analysis
                             are kept (nothing is deleted).

Usage (from anywhere; defaults resolve relative to the repo):
  python3 Templates/seed_templates.py                  # all known components
  python3 Templates/seed_templates.py unit bipd        # only these
  python3 Templates/seed_templates.py --fields DIR --out DIR --dry-run

Notes: XML comments are not preserved (use tooltip / <comment> children instead).
"""
import argparse, os, re, sys
import xml.etree.ElementTree as ET

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

# Component ranges inside the datum (start, end), measured from the type definition table.
RANGES = {
    "obje": (0x000, 0x1A8), "anim": (0x1A8, 0x1B8), "unit": (0x1B8, 0x9D8),
    "bipd": (0x9D8, 0xE00), "vehi": (0x9D8, 0xDE8), "gint": (0x9D8, 0x3BE0),
    "item": (0x1A8, 0x1C4), "weap": (0x1C4, 0x338), "eqip": (0x1C4, 0x210),
    "proj": (0x1A8, 0x258), "crea": (0x1B8, 0x294), "devi": (0x1A8, 0x2C4),
    "term": (0x2C4, 0x2C8), "mach": (0x2C4, 0x2D4), "ctrl": (0x2C4, 0x2CC),
    "scen": (0x1A8, 0x1E4), "bloc": (0x1A8, 0x1B0), "ssce": (0x1A8, 0x1DC),
    "efsc": (0x1A8, 0x1AC),
}

SIZE = dict(int8=1, uint8=1, flags8=1, enum8=1, int16=2, uint16=2, flags16=2, enum16=2,
            int32=4, uint32=4, flags32=4, enum32=4, float32=4, stringid=4, degree=4,
            int64=8, uint64=8, flags64=8, float64=8, point2=8, vector2=8,
            point3=12, vector3=12, degree3=12, vector4=16, quaternion=16, undefined=4)
AUTO_TAG = {1: "uint8", 2: "uint16", 4: "uint32", 8: "uint64"}
LINE = re.compile(r"^0x([0-9a-f]+)\s+(u8|u16|u32|u64)\s+R(\d+)\s+W(\d+)\s*(.*)$")

def candidates(path, start, end):
    out = []
    for ln in open(path, encoding="utf-8"):
        m = LINE.match(ln.rstrip())
        if not m:
            continue
        off = int(m.group(1), 16)
        sz = {"u8": 1, "u16": 2, "u32": 4, "u64": 8}[m.group(2)]
        if off < start or off + sz > end:
            continue
        rest = m.group(5).strip()
        tag = "float32" if sz == 4 and "FLOAT" in rest else AUTO_TAG[sz]
        tip = "R%s W%s %s" % (m.group(3), m.group(4), re.sub(r"\s+", " ", rest)[:150])
        out.append(dict(off=off, sz=sz, tag=tag, tip=tip.strip(),
                        score=int(m.group(3)) + int(m.group(4))))
    return out

def elem_size(el):
    tag = el.tag
    if tag not in SIZE:
        raise SystemExit("unsupported tag <%s> at %s: add it to SIZE" % (tag, el.get("offset")))
    return SIZE[tag]

def overlaps(placed, off, sz):
    return any(off < po + ps and po < off + sz for po, ps in placed)

def process(comp, fields_dir, out_dir, dry):
    start, end = RANGES[comp]
    fpath = os.path.join(fields_dir, "fields_%s.txt" % comp)
    if not os.path.exists(fpath):
        print("%s: %s not found, skipped" % (comp, fpath))
        return
    xpath = os.path.join(out_dir, comp + ".xml")
    cands = candidates(fpath, start, end)

    if os.path.exists(xpath):
        root = ET.parse(xpath).getroot()
        mode = "merge"
    else:
        root = ET.Element("plugin", game="Halo: Reach (MCC)", runtime="true", component=comp,
                          start="0x%X" % start, baseSize="0x%X" % end)
        mode = "create"
    rs, re_ = int(root.get("start", "0"), 16), int(root.get("baseSize", "0"), 16)
    if (rs, re_) != (start, end):
        print("%s: WARNING XML range 0x%X..0x%X differs from RANGES 0x%X..0x%X (XML wins)" % (comp, rs, re_, start, end))
        start, end = rs, re_

    placed, byoff = [], {}
    for el in root:
        off = int(el.get("offset"), 16)
        placed.append((off, elem_size(el)))
        byoff[off] = el

    refreshed = added = 0
    # refresh still-unknown fields (same offset + size => update guess and stats)
    for c in cands:
        el = byoff.get(c["off"])
        if el is not None and el.get("confidence") == "unknown" and elem_size(el) == c["sz"]:
            if el.get("tooltip") != c["tip"] or el.tag != c["tag"]:
                el.set("tooltip", c["tip"])
                el.tag = c["tag"]
                refreshed += 1
    # add new, most-used first so frequent shapes win overlaps
    for c in sorted(cands, key=lambda c: (-c["score"], c["off"], -c["sz"])):
        if overlaps(placed, c["off"], c["sz"]):
            continue
        placed.append((c["off"], c["sz"]))
        el = ET.SubElement(root, c["tag"], {"name": "Unknown %X" % c["off"], "offset": "0x%X" % c["off"],
                                            "confidence": "unknown", "tooltip": c["tip"]})
        byoff[c["off"]] = el
        added += 1

    items = sorted(list(root), key=lambda e: int(e.get("offset"), 16))
    for e in list(root):
        root.remove(e)
    for e in items:
        e.tail = None
        root.append(e)
    labeled = sum(1 for e in items if e.get("confidence") != "unknown")
    print("%s [%s]: %d fields (%d labeled), +%d new, %d refreshed -> %s" %
          (comp, mode, len(items), labeled, added, refreshed, xpath))
    if dry:
        return
    os.makedirs(out_dir, exist_ok=True)
    ET.indent(root, space="\t")
    ET.ElementTree(root).write(xpath, encoding="utf-8", xml_declaration=True)
    raw = open(xpath, "rb").read().replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")   # repo uses CRLF
    open(xpath, "wb").write(raw)

def main():
    ap = argparse.ArgumentParser(description="Create / merge runtime template XML files.")
    ap.add_argument("components", nargs="*", help="obje unit bipd ... (default: every component that has a fields file)")
    ap.add_argument("--fields", default=os.path.join(REPO, "Ignore", "Ghidra", "templates"))
    ap.add_argument("--out", default=os.path.join(REPO, "Templates", "Object"))
    ap.add_argument("--dry-run", action="store_true")
    a = ap.parse_args()
    comps = a.components or list(RANGES)
    for c in comps:
        if c not in RANGES:
            raise SystemExit("unknown component: " + c)
        process(c, a.fields, a.out, a.dry_run)

if __name__ == "__main__":
    main()
