"""
outline.py
Skeleton of C++ files: namespaces, classes, methods and functions with their line ranges, without the bodies.
Read the outline first, then only the line range the change needs.

    python Tools/outline.py FILE [FILE...]     Paths relative to the repo, to Artemis/Core/, or a unique file name (OpenMapDetour.cpp).
    python Tools/outline.py FILE --members     Also data members, usings and constants.
    python Tools/outline.py FILE --find NAME   Only the entries whose line mentions NAME (and their parents).

'+' public, '-' private/protected. A range (12-40) is a body. A single line is a declaration.
"""
import argparse, os, re, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from check_rules import ROOT, Strip
from refs import Classify

ACCESS = re.compile(r"^\s*(public|private|protected)\s*:(?!:)\s*")


def Find(name):
    path = name.replace("\\", "/")
    for candidate in (path, "Artemis/Core/" + path, "Artemis/" + path):
        if os.path.isfile(os.path.join(ROOT, candidate)):
            return candidate
    out = subprocess.run(["git", "ls-files", "--cached", "--others", "--exclude-standard", "--", f"*{os.path.basename(path)}"],
                         cwd=ROOT, capture_output=True, text=True).stdout.split()
    matches = [p for p in out if p.endswith("/" + path) or p == path]
    return matches[0] if len(matches) == 1 else (matches or None)


def Outline(path, members):
    with open(os.path.join(ROOT, path), "rb") as handle:
        lines = handle.read().decode("utf-8", errors="replace").lstrip("﻿").replace("\r\n", "\n").split("\n")
    code = Strip(lines)[0]
    entries, stack, header, start = [], [], [], None
    access = []                                   # current access per open class

    def Named():
        return [s for s in stack if s[0] in ("type", "ns", "func")]

    for index, line in enumerate(code):
        if line.lstrip().startswith("#"):
            continue
        for ch in line:
            if ch == "{":
                text = " ".join("".join(header).split())
                text = ACCESS.sub("", text)
                kind, name = Classify(text)
                inClass = stack and stack[-1][0] == "type"
                stack.append((kind, name, text, start if start is not None else index, len(Named()),
                              access[-1] if inClass and access else None))
                if kind == "type":
                    access.append("private" if re.match(r"(?:template\s*<.*>\s*)?class\b", text) else "public")
                header, start = [], None
            elif ch == "}":
                if not stack:
                    continue
                kind, name, text, begin, depth, acc = stack.pop()
                if kind == "type":
                    access.pop()
                if kind in ("type", "ns", "func"):
                    entries.append((begin, index, depth, kind, text, acc))
                    header, start = [], None
                elif text and not text.endswith(")") and not text.endswith("="):
                    header = list(text + "{}")
                else:
                    header, start = [], None
            elif ch == ";":
                text = " ".join("".join(header).split())
                label = ACCESS.match(text)
                if label and access:
                    access[-1] = label.group(1)
                text = ACCESS.sub("", text)
                if text and stack and stack[-1][0] in ("type", "ns"):
                    isMethod = "(" in text and not re.match(r"using\b", text)
                    if isMethod or members:
                        entries.append((start if start is not None else index, None, len(Named()), "decl", text,
                                        access[-1] if stack[-1][0] == "type" and access else None))
                elif text and not stack and (members or re.match(r"(?:export\s+)?(?:module|import)\b", text)):
                    entries.append((start if start is not None else index, None, 0, "top", text, None))
                header, start = [], None
            else:
                if start is None and not ch.isspace():
                    start = index
                header.append(ch)
                label = ACCESS.match("".join(header))
                if label and access and stack and stack[-1][0] == "type" and "".join(header).rstrip().endswith(":"):
                    access[-1] = label.group(1)
        header.append(" ")
    entries.sort(key=lambda e: (e[0], -(e[1] or e[0])))
    return lines, entries


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("files", nargs="+")
    parser.add_argument("--members", action="store_true")
    parser.add_argument("--find")
    args = parser.parse_args()
    for name in args.files:
        path = Find(name)
        if not isinstance(path, str):
            print(f"{name}: " + ("not found" if not path else "ambiguous: " + ", ".join(path[:5])))
            continue
        lines, entries = Outline(path, args.members)
        imports = sum(1 for e in entries if e[3] == "top" and re.match(r"(?:export\s+)?import\b", e[4]))
        module = next((e[4] for e in entries if e[3] == "top" and re.match(r"(?:export\s+)?module\s+\w", e[4])), "")
        short = path[len("Artemis/Core/"):] if path.startswith("Artemis/Core/") else path
        print(f"{short}  ({len(lines)} lines)  {module}  imports: {imports}")
        rows = []
        for begin, end, depth, kind, text, acc in entries:
            if kind == "top" and not args.members:
                continue
            span = f"{begin + 1}-{end + 1}" if end is not None and end > begin else f"{begin + 1}"
            mark = "+ " if acc == "public" else ("- " if acc in ("private", "protected") else "")
            label = re.sub(r"\)\s*:(?!:)\s*\w.*$", ")", text) if kind == "func" else text
            rows.append((text, f"  {span:>9}  {'  ' * depth}{mark}{label[:130]}"))
        if args.find:
            rows = [r for r in rows if re.search(rf"\b{re.escape(args.find)}\b", r[0]) or not re.search(r"\(", r[0])]
        for _, row in rows:
            print(row)
    return 0


if __name__ == "__main__":
    sys.exit(Main())
