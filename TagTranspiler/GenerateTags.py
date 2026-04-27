"""
GenerateTags.py
Processes one or more Assembly XML files and generates
all necessary C++ headers in organized folders.

Usage, a single XML:
    python GenerateTags.py phmo.xml

Usage, full directory of plugins:
    python GenerateTags.py ".../Assembly/Plugins/ReachMCC"

Output structure (relative to the script directory):
    Generated/
        Phmo/
            PhmoOffsets.h      <- GenerateOffset.py
            PhmoTypes.h        <- GenerateType.py
            PhmoObject.h       <- GenerateObject.py
            PhmoDescriptor.h   <- GenerateObject.py

From C++ include: #include "../Generated/Phmo/PhmoTypes.h"
"""

import sys, os, re

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)

from Common import XmlBaseName
import GenerateOffset
import GenerateType
import GenerateObject

OUTPUT_ROOT = os.path.join(SCRIPT_DIR, "Generated")

def ProcessXml(xmlPath: str) -> None:
    prefix = XmlBaseName(xmlPath)
    outDir = os.path.join(OUTPUT_ROOT, prefix)
    os.makedirs(outDir, exist_ok=True)

    print(f"[{prefix}] {os.path.basename(xmlPath)}")
    try:
        GenerateOffset.Generate(xmlPath, os.path.join(outDir, f"{prefix}Offsets.h"))
        GenerateType.Generate( xmlPath, os.path.join(outDir, f"{prefix}Types.h"))
        GenerateObject.Generate(xmlPath, outDir)
        
    except Exception as e:
        import traceback
        print(f"  [ERROR] {e}")
        traceback.print_exc()

def ProcessDirectory(xmlDir: str) -> None:
    xmlFiles = sorted([
        os.path.join(xmlDir, f)
        for f in os.listdir(xmlDir)
        if f.lower().endswith(".xml")
    ])
    print(f"Processing {len(xmlFiles)} XML in: {xmlDir}")
    print(f"Output: {OUTPUT_ROOT}")
    print()
    for xmlPath in xmlFiles:
        ProcessXml(xmlPath)
    print(f"\nDone. {len(xmlFiles)} groups generated in {OUTPUT_ROOT}")

def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    target = sys.argv[1]

    if os.path.isdir(target):
        ProcessDirectory(target)
    elif os.path.isfile(target) and target.lower().endswith(".xml"):
        print(f"Output: {OUTPUT_ROOT}")
        ProcessXml(target)
    else:
        print(f"Error: '{target}' is not a valid XML file or directory.")
        sys.exit(1)

if __name__ == "__main__":
    main()