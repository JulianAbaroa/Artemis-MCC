"""
build.py
Compiles Artemis through the build watcher (Tools/build_watcher.bat, open on the Windows side) and prints the errors and warnings
in compact form: one line per diagnostic, relative paths, no duplicates, errors first.

    python Tools/build.py                       Build Release|x64 and wait (up to --timeout seconds).
    python Tools/build.py --config Release|x64  Other configuration.
    python Tools/build.py --rebuild             Rebuild instead of an incremental build.
    python Tools/build.py --detail LEVEL        Also write build/watch/detail.log (normal, detailed, diagnostic) with the MSBuild
                                                performance summary (time per project).
    python Tools/build.py --project PATH        Build another project or solution (path relative to the repo, e.g.
                                                Artemis/Tests/Artemis.Tests.vcxproj). Default: Artemis/Artemis.vcxproj.
    python Tools/build.py --wait ID             Keep waiting for a build that outlived --timeout (the id is printed).
    python Tools/build.py --last                Print the result of the last build again, without building.
    python Tools/build.py --warnings            Every warning (default: only the ones in files changed according to git, plus counts).

    -n N   cap on diagnostic lines printed (default 40; 0 = no cap)

Exit code: 0 build ok, 1 build failed, 2 still building (use --wait), 3 the watcher is not running, 4 a LiveChat
message arrived (Tools/livechat.py; the build keeps going, resume with --wait).
"""
import argparse, collections, json, os, re, subprocess, sys, time, uuid
import livechat

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WATCH = os.path.join(ROOT, "build", "watch")
DIAGNOSTIC = re.compile(r"^(?P<origin>.+?)(?:\((?P<line>\d+)(?:,(?P<col>\d+))?\))?\s*:\s*(?P<kind>fatal error|error|warning)\s+"
                        r"(?P<code>[A-Z]+\d+)\s*:\s*(?P<message>.*?)(?:\s*\[[^\]]*\])?\s*$")


def ReadJson(name):
    try:
        with open(os.path.join(WATCH, name), encoding="utf-8-sig") as handle:
            return json.load(handle)
    except (OSError, ValueError):
        return None


def Relative(origin):
    path = origin.strip().replace("\\", "/")
    root = ROOT.replace("\\", "/")
    index = path.lower().find("/artemis-mcc/")
    if index >= 0:
        path = path[index + len("/artemis-mcc/"):]
    elif path.lower().startswith(root.lower() + "/"):
        path = path[len(root) + 1:]
    if path.startswith("Artemis/Core/"):
        return path[len("Artemis/Core/"):]
    return path[len("Artemis/"):] if path.startswith("Artemis/") else path


def Parse(name):
    items, seen = [], set()
    try:
        with open(os.path.join(WATCH, name), encoding="utf-8-sig", errors="replace") as handle:
            lines = handle.read().splitlines()
    except OSError:
        return items
    for line in lines:
        match = DIAGNOSTIC.match(line.strip())
        if not match:
            continue
        item = (Relative(match.group("origin")), int(match.group("line") or 0), match.group("kind"), match.group("code"),
                " ".join(match.group("message").split()))
        key = (item[0].lower(), item[1], item[3], item[4])
        if key not in seen:
            seen.add(key)
            items.append(item)
    return items


def ChangedFiles():
    out = subprocess.run(["git", "status", "--porcelain", "--untracked-files=all"], cwd=ROOT, capture_output=True, text=True).stdout
    return {Relative(line[3:].split(" -> ")[-1].strip('"')).lower() for line in out.splitlines()}


def Report(result, args):
    errors, warnings = Parse("errors.log"), Parse("warnings.log")
    state = "OK" if result.get("exitCode") == 0 else f"FAILED (exit {result.get('exitCode')})"
    files = len({e[0] for e in errors})
    project = f" {result['project']}" if result.get("project") else ""
    print(f"Build{project} {result.get('configuration')}|{result.get('platform')} {result.get('target')}: {state} in {result.get('seconds')} s. "
          f"{len(errors)} errors in {files} files, {len(warnings)} warnings.")
    if result.get("error"):
        print(f"  watcher error: {result['error']}")
    if result.get("exitCode") != 0 and not errors:
        print(f"  no compiler errors: MSBuild failed before compiling ({result.get('msbuild')}). Last lines of the console:")
        try:
            with open(os.path.join(WATCH, "console.log"), "rb") as handle:
                raw = handle.read()
            text = raw.decode("utf-16", errors="replace") if raw[:2] in (b"\xff\xfe", b"\xfe\xff") else raw.decode("utf-8-sig", errors="replace")
            tail = [l.rstrip() for l in text.splitlines() if l.strip()][-12:]
        except OSError:
            tail = ["(no console.log)"]
        for line in tail:
            print("    " + line[:200])
    lines = []
    for origin, line, kind, code, message in sorted(errors, key=lambda e: (e[0].lower(), e[1])):
        lines.append(f"  {origin}:{line}: {code} {message[:160]}")
        if code == "LNK1104" and ".dll" in message.lower():
            lines.append("    (the DLL is locked: probably injected in the game. Close the game or eject it and build again.)")
    if warnings:
        changed = ChangedFiles()
        shown = warnings if args.warnings else [w for w in warnings if w[0].lower() in changed]
        codes = collections.Counter(w[3] for w in warnings)
        lines.append(f"warnings by code: " + ", ".join(f"{c} x{n}" for c, n in codes.most_common(8))
                     + ("" if args.warnings else f"  ({len(shown)} in changed files, listed below; --warnings for all)"))
        for origin, line, kind, code, message in sorted(shown, key=lambda w: (w[0].lower(), w[1])):
            lines.append(f"  {origin}:{line}: {code} {message[:160]}")
    for line in lines[:args.n] if args.n else lines:
        print(line)
    if args.n and len(lines) > args.n:
        print(f"... {len(lines) - args.n} more lines (-n 0 for all)")
    return 0 if result.get("exitCode") == 0 else 1


def Wait(id, timeout):
    start = time.time()
    while time.time() - start < timeout:
        result = ReadJson("result.json")
        if result and result.get("id") == id:
            return result
        if time.time() - start > 12:
            status = ReadJson("status.json")
            if not status or status.get("id") != id:
                print("The build watcher did not pick up the request: ask the user to open Tools\\build_watcher.bat.")
                print(f"The request stays queued: once it is open, run python Tools/build.py --wait {id}")
                sys.exit(3)
        livechat.Interrupt(f"python Tools/build.py --wait {id}")
        time.sleep(2)
    print(f"Still building after {timeout} s. Run: python Tools/build.py --wait {id}")
    sys.exit(2)


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--config", default="Release|x64")
    parser.add_argument("--rebuild", action="store_true")
    parser.add_argument("--detail", choices=["normal", "detailed", "diagnostic"], help="also write build/watch/detail.log with that verbosity")
    parser.add_argument("--project", help="project or solution to build, relative to the repo (default Artemis/Artemis.vcxproj)")
    parser.add_argument("--wait")
    parser.add_argument("--last", action="store_true")
    parser.add_argument("--warnings", action="store_true")
    parser.add_argument("--timeout", type=int, default=170)
    parser.add_argument("-n", type=int, default=40)
    args = parser.parse_args()
    os.makedirs(WATCH, exist_ok=True)

    if args.last:
        result = ReadJson("result.json")
        if not result:
            print("No build yet.")
            return 3
        return Report(result, args)
    if args.wait:
        return Report(Wait(args.wait, args.timeout), args)

    configuration, _, platform = args.config.partition("|")
    id = time.strftime("%Y%m%d-%H%M%S-") + uuid.uuid4().hex[:6]
    request = {"id": id, "configuration": configuration, "platform": platform or "x64", "target": "Rebuild" if args.rebuild else "Build"}
    if args.detail:
        request["detail"] = args.detail
    if args.project:
        request["project"] = args.project.replace("\\", "/")
    with open(os.path.join(WATCH, "request.json"), "w", encoding="utf-8") as handle:
        json.dump(request, handle)
    return Report(Wait(id, args.timeout), args)


if __name__ == "__main__":
    sys.exit(Main())
