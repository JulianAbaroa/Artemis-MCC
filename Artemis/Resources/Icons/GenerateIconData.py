import os
import sys

LICENSE_HEADER = (
    "// ============================================================\n"
    "// Icon data embedded from Font Awesome Free (https://fontawesome.com)\n"
    "// Icons: cube, person-rifle, gear, square-binary, file-lines, map\n"
    "// License: CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)\n"
    "// Changes: downloaded as PNG, embedded as byte arrays.\n"
    "// This attribution applies to the icon byte data below, not to\n"
    "// the surrounding source code (which is GPL-3.0).\n"
    "// ============================================================\n\n"
)

ICONS = {
    "cube-solid.png":          "Objects",
    "person-rifle-solid.png":  "Players",
    "map-solid.png":           "Map",
    "gear-solid.png":          "Settings",
    "square-binary-solid.png": "Scanner",
    "file-lines-solid.png":    "Logs",
}

def to_cpp_array(name: str, data: bytes) -> tuple[str, str]:
    """Generate the extern declaration and the array definition."""
    hex_bytes = ", ".join(f"0x{b:02x}" for b in data)
    
    decl = (
        f"    extern const unsigned char {name}[];\n"
        f"    extern const unsigned int  {name}Size;\n"
    )
    
    defn = (
        f"const unsigned char Icons::Data::{name}[] = {{\n    "
        + ",\n    ".join(
            ", ".join(f"0x{b:02x}" for b in data[i:i+16])
            for i in range(0, len(data), 16)
        )
        + f"\n}};\n"
        f"const unsigned int Icons::Data::{name}Size = {len(data)}u;\n\n"
    )
    
    return decl, defn

def generate(png_dir: str):
    header_lines = [
        LICENSE_HEADER,
        "#pragma once\n\n",
        "namespace Icons::Data\n{\n",
    ]
    cpp_lines = [
        LICENSE_HEADER,
        '#include "pch.h"\n',
        '#include "IconData.h"\n\n',
    ]

    for filename, varname in ICONS.items():
        path = os.path.join(png_dir, filename)
        if not os.path.exists(path):
            print(f"  [WARN] Not found: {path}.")
            continue

        with open(path, "rb") as f:
            data = f.read()

        decl, defn = to_cpp_array(varname, data)
        header_lines.append(decl)
        cpp_lines.append(defn)
        print(f"[OK] {filename} → Icons::Data::{varname} ({len(data)} bytes)")

    header_lines.append("}\n")

    with open(os.path.join(png_dir, "IconData.h"), "w") as f:
        f.writelines(header_lines)

    with open(os.path.join(png_dir, "IconData.cpp"), "w") as f:
        f.writelines(cpp_lines)

    print("\nGenerated: IconData.h")
    print("Generated: IconData.cpp")

if __name__ == "__main__":
    default_dir = os.path.dirname(os.path.abspath(__file__))
    png_dir = sys.argv[1] if len(sys.argv) > 1 else default_dir
    print(f"\nLooking for PNGs in: {os.path.abspath(png_dir)}\n")
    generate(png_dir)