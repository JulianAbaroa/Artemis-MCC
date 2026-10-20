"""
deps.py
Module graph of the C++ code (export module / module / import), built from the sources in a fraction of a second.
Answers "who depends on X" and "what does a change in X touch" without reading the files.

    python Tools/deps.py users NAME          Files that import NAME directly (NAME = module, Module:Partition, or a file path).
    python Tools/deps.py users NAME -t       Same, transitive: every file that depends on NAME through any chain.
    python Tools/deps.py uses NAME [-t]      What NAME imports (direct or transitive). External imports (std, <...>) are left out.
    python Tools/deps.py impact FILE...      Files affected by a change in these files (transitive users), grouped by module.
    python Tools/deps.py where NAME          Which file declares a module or partition. A trailing '*' matches by prefix.
    python Tools/deps.py top [-n 20]         Modules with the most direct users (fan-in): the risky ones to change.
    python Tools/deps.py cycles              Import cycles between files (should be none).

    --files   list every file instead of grouping by module     -n N   cap on lines printed (default 40; 0 = no cap)

A unit is "Module" (primary interface) or "Module:Partition". 'import :P' inside module M means M:P. An implementation unit
('module M;' in a .cpp) depends on M. Generated files count like any other. Artemis/External/ is not scanned.
"""
import argparse, collections, os, re, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from check_rules import ROOT, Strip

DECL = re.compile(r"^\s*(export\s+)?module\s+([\w.]+)(:[\w.]+)?\s*;")
IMPORT = re.compile(r"^\s*(?:export\s+)?import\s+([\w.]+)?(:[\w.]+)?\s*;")


def ListSources():
    out = subprocess.run(["git", "ls-files", "--cached", "--others", "--exclude-standard", "--", "Artemis/"],
                         cwd=ROOT, capture_output=True, text=True, check=True).stdout
    # A file moved or deleted since the last commit is still in the index: only the ones on disk count.
    return sorted(p for p in out.splitlines() if p.endswith((".ixx", ".cpp")) and not p.startswith("Artemis/External/")
                  and os.path.isfile(os.path.join(ROOT, p)))


class Graph:
    def __init__(self):
        self.Unit = {}                               # file -> unit it declares (None for implementation units)
        self.Owner = {}                              # file -> module it belongs to
        self.Imports = collections.defaultdict(set)  # file -> units it imports
        self.Providers = collections.defaultdict(set)  # unit -> files that declare it
        for path in ListSources():
            with open(os.path.join(ROOT, path), "rb") as handle:
                lines = handle.read().decode("utf-8", errors="replace").lstrip("﻿").replace("\r\n", "\n").split("\n")
            code = Strip(lines)[0]
            module = None
            for line in code:
                match = DECL.match(line)
                if match and module is None:
                    module = match.group(2)
                    self.Owner[path] = module
                    if match.group(1):
                        unit = module + (match.group(3) or "")
                        self.Unit[path] = unit
                        self.Providers[unit].add(path)
                    elif match.group(3):                 # internal partition: module M:P;
                        unit = module + match.group(3)
                        self.Unit[path] = unit
                        self.Providers[unit].add(path)
                    else:                                # implementation unit: depends on its module
                        self.Unit[path] = None
                        self.Imports[path].add(module)
                    continue
                match = IMPORT.match(line)
                if match:
                    name, part = match.group(1), match.group(2)
                    if part and not name:
                        if module:
                            self.Imports[path].add(module + part)
                    elif name and name != "std" and not name.startswith("std."):
                        self.Imports[path].add(name + (part or ""))
        self.Users = collections.defaultdict(set)    # file -> files that import a unit it declares
        for path, units in self.Imports.items():
            for unit in units:
                for provider in self.Providers.get(unit, ()):
                    if provider != path:
                        self.Users[provider].add(path)

    def Resolve(self, name):
        """A name, a 'Prefix*' or a file path -> set of files."""
        path = name.replace("\\", "/")
        if path in self.Owner:
            return {path}
        matches = [p for p in self.Owner if p.endswith("/" + path) or p.endswith(path)] if "/" in path or path.endswith((".ixx", ".cpp")) else []
        if matches:
            return set(matches)
        if name.endswith("*"):
            prefix = name[:-1]
            return {f for unit, files in self.Providers.items() if unit.startswith(prefix) for f in files}
        files = set(self.Providers.get(name, ()))
        if ":" not in name and files:
            return files
        return files

    def Closure(self, start, edges):
        seen, stack = set(), list(start)
        while stack:
            for nxt in edges(stack.pop()):
                if nxt not in seen and nxt not in start:
                    seen.add(nxt)
                    stack.append(nxt)
        return seen

    def Dependencies(self, path):
        return {p for unit in self.Imports.get(path, ()) for p in self.Providers.get(unit, ()) if p != path}


def Label(graph, path):
    unit = graph.Unit.get(path)
    return unit if unit else (graph.Owner.get(path, "?") + " (impl)")


def Short(path):
    return path[len("Artemis/Core/"):] if path.startswith("Artemis/Core/") else path


def Print(lines, limit):
    for line in lines[:limit] if limit else lines:
        print(line)
    if limit and len(lines) > limit:
        print(f"... {len(lines) - limit} more (-n 0 for all)")


def Report(graph, files, header, listFiles, limit):
    print(f"{header}: {len(files)} files")
    if not files:
        return
    if listFiles:
        Print([f"  {Short(p)}  [{Label(graph, p)}]" for p in sorted(files)], limit)
        return
    groups = collections.Counter(graph.Owner.get(p, "?") for p in files)
    Print([f"  {count:3}  {module}" for module, count in sorted(groups.items(), key=lambda kv: (-kv[1], kv[0]))], limit)


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", choices=["users", "uses", "impact", "where", "top", "cycles"])
    parser.add_argument("names", nargs="*")
    parser.add_argument("-t", "--transitive", action="store_true")
    parser.add_argument("--files", action="store_true")
    parser.add_argument("-n", type=int, default=40)
    args = parser.parse_args()
    graph = Graph()

    if args.command in ("users", "uses", "where", "impact") and not args.names:
        parser.error(f"{args.command} needs a NAME")

    if args.command == "where":
        for name in args.names:
            files = graph.Resolve(name)
            print(f"{name}: " + ("not found" if not files else ""))
            Print([f"  {Short(p)}  [{Label(graph, p)}]" for p in sorted(files)], args.n)
        return 0

    if args.command in ("users", "uses", "impact"):
        start = set()
        for name in args.names:
            found = graph.Resolve(name)
            if not found:
                print(f"{name}: not found (try: deps.py where '{name}*')")
                return 1
            start |= found
        if args.command == "uses":
            files = graph.Closure(start, graph.Dependencies) if args.transitive else {d for p in start for d in graph.Dependencies(p)}
            Report(graph, files, "uses" + (" (transitive)" if args.transitive else ""), args.files, args.n)
        else:
            transitive = args.transitive or args.command == "impact"
            files = graph.Closure(start, lambda p: graph.Users.get(p, ())) if transitive else {u for p in start for u in graph.Users.get(p, ())}
            files -= start
            Report(graph, files, "users" + (" (transitive)" if transitive else ""), args.files, args.n)
        return 0

    if args.command == "top":
        byModule = collections.defaultdict(set)
        for path, users in graph.Users.items():
            byModule[graph.Owner.get(path, "?")] |= {u for u in users if graph.Owner.get(u) != graph.Owner.get(path)}
        rows = sorted(((len(u), m) for m, u in byModule.items()), reverse=True)
        Print([f"  {count:3}  {module}" for count, module in rows], args.n or 20)
        return 0

    if args.command == "cycles":
        index, low, onStack, stack, cycles, counter = {}, {}, set(), [], [], [0]
        sys.setrecursionlimit(10000)

        def Visit(v):
            index[v] = low[v] = counter[0]
            counter[0] += 1
            stack.append(v)
            onStack.add(v)
            for w in graph.Dependencies(v):
                if w not in index:
                    Visit(w)
                    low[v] = min(low[v], low[w])
                elif w in onStack:
                    low[v] = min(low[v], index[w])
            if low[v] == index[v]:
                component = []
                while True:
                    w = stack.pop()
                    onStack.discard(w)
                    component.append(w)
                    if w == v:
                        break
                if len(component) > 1:
                    cycles.append(component)

        for v in sorted(graph.Owner):
            if v not in index:
                Visit(v)
        print(f"cycles: {len(cycles)}")
        for component in cycles:
            Print(["  " + " | ".join(Short(p) for p in sorted(component))], args.n)
        return 1 if cycles else 0


if __name__ == "__main__":
    sys.exit(Main())
