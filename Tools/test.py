"""
test.py
Builds a test project through the build watcher (Tools/build_watcher.bat, open on the Windows side), runs its exe with GoogleTest
JSON output and prints a compact summary: how many passed, failed (file:line and message) and were skipped (reason).

    python Tools/test.py                              Release|x64 build + run of Artemis/Tests/Artemis.Tests.
    python Tools/test.py --config Debug|x64           Other configuration.
    python Tools/test.py --project PATH --exe PATH    Another test project and its exe (paths relative to the repo).
    python Tools/test.py --filter "Suite.*"           Passed to the exe as --gtest_filter.
    python Tools/test.py --wait ID                    Keep waiting for a build that outlived --timeout (the id is printed).

    -n N   cap on lines printed (default 40; 0 = no cap)

Exit code: 0 every test passed (skipped ones do not count), 1 a test failed, 2 the project did not compile (or the exe did not
run), 3 the watcher is not running, 4 a LiveChat message arrived (the build keeps going, resume with --wait), 5 still building.
"""
import argparse, json, os, re, sys, time, uuid
import build, livechat

DEFAULT_PROJECT = "Artemis/Tests/Artemis.Tests.vcxproj"
DEFAULT_EXE = "build/{platform}/{configuration}/Artemis.Tests.exe"
RESULTS = "build/watch/tests.json"
RUN_LOG = "run.log"


def ReadText(name):
    try:
        with open(os.path.join(build.WATCH, name), encoding="utf-8-sig", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def SkipReasons(log):
    """Reason of each skipped test, from the run log: the text between 'file(line): Skipped' and '[  SKIPPED ] Suite.Test'."""
    reasons = {}
    log = log.replace("\r", "")
    for match in re.finditer(r"^(?P<where>[^\n]*?(?:\(\d+\)|:\d+)): Skipped[ \t]*\n(?P<reason>(?s:.*?))\n?\[  SKIPPED \] (?P<name>[\w./]+)", log, re.M):
        reasons[match.group("name")] = " ".join(match.group("reason").split())
    return reasons


def Summarize(args, started):
    try:
        mtime = os.path.getmtime(os.path.join(build.ROOT, RESULTS))
        with open(os.path.join(build.ROOT, RESULTS), encoding="utf-8-sig") as handle:
            data = json.load(handle)
    except (OSError, ValueError):
        print("The exe ran but wrote no readable " + RESULTS + ". Last lines of the run log:")
        for line in [l for l in ReadText(RUN_LOG).splitlines() if l.strip()][-10:]:
            print("    " + line[:200])
        return 2
    if mtime < started - 2:
        print(f"{RESULTS} is older than this run: the exe did not write it.")
        return 2
    reasons = SkipReasons(ReadText(RUN_LOG))
    passed, failed, skipped = 0, [], []
    for suite in data.get("testsuites", []):
        for case in suite.get("testsuite", []):
            name = f"{suite.get('name')}.{case.get('name')}"
            if case.get("failures"):
                failed.append((name, case["failures"]))
            elif case.get("result") == "SKIPPED" or case.get("status") == "NOTRUN":
                skipped.append((name, reasons.get(name, "")))
            else:
                passed += 1
    lines = [f"Tests: {passed} passed, {len(failed)} failed, {len(skipped)} skipped ({data.get('time', '?')})."]
    for name, failures in failed:
        for failure in failures:
            text = failure.get("failure", "").replace("\r", "")
            first, _, rest = text.partition("\n")
            lines.append(f"  FAILED {name}: {build.Relative(first)}: {' '.join(rest.split())[:200]}")
    for name, reason in skipped:
        lines.append(f"  SKIPPED {name}: {reason or '(no reason in the run log)'}")
    for line in lines[:args.n] if args.n else lines:
        print(line)
    if args.n and len(lines) > args.n:
        print(f"... {len(lines) - args.n} more lines (-n 0 for all)")
    return 1 if failed else 0


def Wait(id, timeout):
    start = time.time()
    while time.time() - start < timeout:
        result = build.ReadJson("result.json")
        if result and result.get("id") == id:
            return result
        if time.time() - start > 12:
            status = build.ReadJson("status.json")
            if not status or status.get("id") != id:
                print("The build watcher did not pick up the request: ask the user to open Tools\\build_watcher.bat.")
                print(f"The request stays queued: once it is open, run python Tools/test.py --wait {id}")
                sys.exit(3)
        livechat.Interrupt(f"python Tools/test.py --wait {id}")
        time.sleep(2)
    print(f"Still building after {timeout} s. Run: python Tools/test.py --wait {id}")
    sys.exit(5)


def Finish(result, args, started):
    if result.get("exitCode") != 0:
        build.Report(result, args)
        print("The test project did not compile.")
        return 2
    if "runExitCode" not in result:
        print("The build is ok but the watcher did not run the exe (is it an old watcher? close and reopen build_watcher.bat).")
        return 2
    code = Summarize(args, started)
    print(f"(build {result.get('seconds')} s, run {result.get('runSeconds')} s, exe exit code {result.get('runExitCode')})")
    return code


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--config", default="Release|x64")
    parser.add_argument("--project", default=DEFAULT_PROJECT)
    parser.add_argument("--exe")
    parser.add_argument("--filter")
    parser.add_argument("--wait")
    parser.add_argument("--timeout", type=int, default=170)
    parser.add_argument("-n", type=int, default=40)
    args = parser.parse_args()
    args.warnings = False
    os.makedirs(build.WATCH, exist_ok=True)
    started = time.time()

    if args.wait:
        return Finish(Wait(args.wait, args.timeout), args, started - args.timeout)
    configuration, _, platform = args.config.partition("|")
    platform = platform or "x64"
    exe = args.exe or DEFAULT_EXE.format(platform=platform, configuration=configuration)
    runArguments = ["--gtest_output=json:" + RESULTS]
    if args.filter:
        runArguments.append("--gtest_filter=" + args.filter)
    id = time.strftime("%Y%m%d-%H%M%S-") + uuid.uuid4().hex[:6]
    request = {"id": id, "configuration": configuration, "platform": platform, "target": "Build",
               "project": args.project.replace("\\", "/"), "run": {"exe": exe.replace("\\", "/"), "args": runArguments}}
    with open(os.path.join(build.WATCH, "request.json"), "w", encoding="utf-8") as handle:
        json.dump(request, handle)
    return Finish(Wait(id, args.timeout), args, started)


if __name__ == "__main__":
    sys.exit(Main())
