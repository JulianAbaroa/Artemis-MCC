"""
refs.py
Where a symbol is used, one line per use with the function or class it is in, without reading the files.

    python Tools/refs.py NAME [NAME...]       Uses of each name (whole word), grouped by file, with the enclosing scope.
    python Tools/refs.py NAME --defs          Only lines that look like the definition or declaration.
    python Tools/refs.py NAME --count         Only the number of uses per file.
    python Tools/refs.py NAME --in Domain/4   Only files whose path contains this text.
    python Tools/refs.py NAME --comments      Also hits that are only in comments or strings (left out by default, counted).
    python Tools/refs.py NAME --aliases       Also the 'using X = A::B::X;' / 'using A::B::X;' lines (left out by default, counted).

    -n N   cap on lines printed (default 60; 0 = no cap)

Searches tracked and untracked .ixx/.cpp under Artemis/ (not Artemis/External/) with git grep -w. The scope is found by walking
the braces of the file with comments and strings removed (check_rules.Strip). 'def' is a heuristic: type, namespace or function
definition, or a member/constant declared with an initializer.
"""
import argparse, collections, os, re, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from check_rules import ROOT, Strip

CONTROL = {"if", "for", "while", "switch", "catch", "return", "sizeof", "decltype", "alignof", "static_assert", "noexcept",
           "requires", "alignas", "defined", "__declspec", "else", "do", "try"}
TYPE_HEAD = re.compile(r"^(?:template\s*<.*>\s*)?(?:export\s+)?(namespace|class|struct|union|enum(?:\s+class|\s+struct)?)\b\s*([\w:]*)")
FUNC_TAIL = re.compile(r"^\s*(?:(?:const|volatile|noexcept(?:\s*\(.*\))?|override|final|mutable|&&|&)\s*)*(?:->\s*[^{;]+)?$")


def MatchParen(text, open):
    depth = 0
    for i in range(open, len(text)):
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0:
                return i
    return -1


def Classify(header):
    """Header text before a '{' -> (kind, name). kind: type, func, block."""
    h = " ".join(header.split())
    match = TYPE_HEAD.match(h)
    if match:
        if match.group(1) == "namespace":
            return ("ns", match.group(2) or "(anonymous)")
        return ("type", match.group(2))
    for m in re.finditer(r"([~\w][\w:~]*)\s*\(", h):
        name = m.group(1)
        if name.split("::")[-1] in CONTROL:
            return ("block", "")
        prefix = h[:m.start()]
        if "=" in prefix or "(" in prefix or "[" in prefix or "{" in prefix:
            return ("block", "")
        close = MatchParen(h, m.end() - 1)
        if close < 0:
            return ("block", "")
        rest = h[close + 1:]
        if FUNC_TAIL.match(rest) or re.match(r"^\s*(?:noexcept\s*)?:(?!:)", rest):
            return ("func", name)
        return ("block", "")
    return ("block", "")


def Scopes(code):
    """Per line, the scope stack at the start of that line: list of (kind, name)."""
    result, stack, header, popped = [], [], [], None
    for line in code:
        result.append([s for s in stack])
        if line.lstrip().startswith("#"):
            continue
        for ch in line:
            if ch == "{":
                text = "".join(header).strip()
                kind, name = Classify(text)
                stack.append((kind, name, text))
                header = []
            elif ch == "}":
                if stack:
                    kind, name, text = stack.pop()
                    header = list(text + "{}") if kind == "block" and text and not text.endswith(")") and not text.endswith("=") else []
                else:
                    header = []
            elif ch == ";":
                header = []
            else:
                header.append(ch)
        header.append(" ")
    return result


def Context(stack):
    types = [n for k, n, _ in stack if k == "type" and n]
    funcs = [n for k, n, _ in stack if k == "func"]
    spaces = [n for k, n, _ in stack if k == "ns"]
    if funcs:
        func = funcs[0]
        return func if "::" in func or not types else types[-1] + "::" + func
    return types[-1] if types else (spaces[-1] if spaces else "(file)")


def IsAlias(code, name):
    w = re.escape(name)
    return bool(re.match(rf"^\s*using\s+(?:{w}\s*=|[\w:]*::{w}\s*;)", code))


def IsDefinition(code, name):
    w = re.escape(name)
    return bool(re.search(rf"\b(?:namespace|class|struct|union|enum(?:\s+class)?|concept)\s+{w}\b(?!\s*;)", code)
                or re.search(rf"^\s*(?:template\s*<.*>\s*)?(?:[\w:]+\s+)*auto\s+(?:__\w+\s+)?(?:[\w:]+::)?{w}\s*\(", code)
                or re.search(rf"^\s*(?!(?:return|delete|co_return|throw|case|goto|else)\b)(?:[\w:]+\s+)*[\w:<>,]+[\s*&]+{w}\s*(?:\{{|=[^=]|;|\[)", code)
                or re.search(rf"^\s*(?:[\w:]+::)?{w}\s*\([^;]*\)\s*(?::(?!:)|$)", code))


def Short(path):
    if path.startswith("Artemis/Core/"):
        return path[len("Artemis/Core/"):]
    return path[len("Artemis/"):] if path.startswith("Artemis/") else path


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("names", nargs="+")
    parser.add_argument("--defs", action="store_true")
    parser.add_argument("--count", action="store_true")
    parser.add_argument("--comments", action="store_true")
    parser.add_argument("--aliases", action="store_true")
    parser.add_argument("--in", dest="within", default="")
    parser.add_argument("-n", type=int, default=60)
    args = parser.parse_args()

    command = ["git", "grep", "--untracked", "-n", "-w"]
    for name in args.names:
        command += ["-e", name]
    command += ["--", "Artemis/*.ixx", "Artemis/*.cpp", ":!Artemis/External/"]
    out = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, errors="replace").stdout
    hits = collections.defaultdict(list)
    for row in out.splitlines():
        path, line, _ = row.split(":", 2)
        if args.within.replace("\\", "/") in path:
            hits[path].append(int(line))

    words = [re.compile(rf"\b{re.escape(n)}\b") for n in args.names]
    rows, hidden, aliases, total, perFile = [], 0, 0, 0, collections.Counter()
    for path in sorted(hits):
        with open(os.path.join(ROOT, path), "rb") as handle:
            lines = handle.read().decode("utf-8", errors="replace").lstrip("﻿").replace("\r\n", "\n").split("\n")
        code = Strip(lines)[0]
        scopes = None
        fileRows = []
        for number in hits[path]:
            index = number - 1
            if index >= len(code):
                continue
            inCode = any(w.search(code[index]) for w in words)
            if not inCode and not args.comments:
                hidden += 1
                continue
            if inCode and not args.aliases and any(IsAlias(code[index], n) for n in args.names):
                aliases += 1
                continue
            isDef = inCode and any(IsDefinition(code[index], n) for n in args.names if re.search(rf"\b{re.escape(n)}\b", code[index]))
            if args.defs and not isDef:
                continue
            if scopes is None:
                scopes = Scopes(code)
            tag = "def" if isDef else ("   " if inCode else "cmt")
            text = lines[index].strip()
            fileRows.append(f"  {number:5} {tag} [{Context(scopes[index])}]  {text[:110]}")
        if fileRows:
            perFile[path] = len(fileRows)
            total += len(fileRows)
            if not args.count:
                rows.append(f"{Short(path)} ({len(fileRows)})")
                rows.extend(fileRows)

    print(f"{' '.join(args.names)}: {total} {'definitions' if args.defs else 'uses'} in {len(perFile)} files"
          + (f"; hidden: {hidden} in comments/strings" if hidden else "") + (f", {aliases} using-aliases" if aliases else ""))
    if args.count:
        rows = [f"  {count:4}  {Short(path)}" for path, count in sorted(perFile.items(), key=lambda kv: (-kv[1], kv[0]))]
    limit = args.n
    for row in rows[:limit] if limit else rows:
        print(row)
    if limit and len(rows) > limit:
        print(f"... {len(rows) - limit} more lines (-n 0 for all, or --count / --in to narrow)")
    return 0


if __name__ == "__main__":
    sys.exit(Main())
