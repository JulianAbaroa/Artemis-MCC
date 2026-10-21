"""
game.py
Drives the in-game test bench (Diseno/11, D129) from the device shell: deploys the DLL and launches MCC through the game watcher
(Tools/game_watcher.bat, open on the Windows side), queues scenarios for Artemis (BenchService) and prints their reports in a few
lines. Every wait is capped at --timeout seconds (default 170, below the shell limit); when it runs out the command says how to
keep waiting.

    python Tools/game.py status                  Watcher, MCC and Artemis state in one line each.
    python Tools/game.py deploy [--launch]       Close MCC, copy build/x64/Release/wtsapi32.dll (+pdb) and optionally launch.
    python Tools/game.py launch | kill           Start MCC (Steam, no EAC) / close it.
    python Tools/game.py info                    config.ini of Artemis and the recorded match configs on disk.
    python Tools/game.py wait loaded|lobby|exit  Wait until Artemis is loaded, the Reach custom games lobby is open, or MCC is gone.
    python Tools/game.py run FILE [--no-wait]    Queue a scenario (JSON, Diseno/11 sec. 9) and wait for its report.
    python Tools/game.py report [ID]             Print a report again (default: the last one).
    python Tools/game.py log [-n N] [--all] [--grep RE]   Last WARNING/ERROR lines of the Artemis log (--all: every line).
    python Tools/game.py crashes                 Crashes saved by the watcher.
    python Tools/game.py export                  Compile Tools/scenarios/*.json for the bench panel of the Simulation tab.

Exit code: 0 ok, 1 failed (request or scenario), 2 still waiting (run the same wait again), 3 the watcher is not running,
4 a LiveChat message arrived (Tools/livechat.py; it is printed with the command that resumes the wait).
"""
import argparse, glob, json, os, re, sys, time, uuid
from datetime import datetime, timezone
import livechat

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME = os.path.join(ROOT, "build", "game")


def ReadJson(path):
    try:
        with open(path if os.path.isabs(path) else os.path.join(GAME, path), encoding="utf-8-sig") as handle:
            return json.load(handle)
    except (OSError, ValueError):
        return None


def WriteJson(name, value):
    path = os.path.join(GAME, name)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path + ".tmp", "w", encoding="utf-8") as handle:
        json.dump(value, handle, indent=1)
    os.replace(path + ".tmp", path)


def Age(stamp):
    if not stamp:
        return None
    try:
        moment = datetime.fromisoformat(re.sub(r"(\.\d{6})\d+", r"\1", stamp).replace("Z", "+00:00"))
    except ValueError:
        return None
    now = datetime.now(moment.tzinfo) if moment.tzinfo else datetime.now()
    return (now - moment).total_seconds()


def Watcher():
    state = ReadJson("watcher.json")
    if not state:
        return None
    age = Age(state.get("heartbeat"))
    return state if age is not None and age < 10 else None


def NeedWatcher():
    state = Watcher()
    if not state:
        print("The game watcher is not running: ask the user to open Tools\\game_watcher.bat.")
        sys.exit(3)
    return state


def Bench():
    state = ReadJson("state.json")
    if not state:
        return None
    age = Age(state.get("heartbeat"))
    state["_alive"] = age is not None and age < 10
    return state


def Request(kind, **extra):
    NeedWatcher()
    id = time.strftime("%Y%m%d-%H%M%S-", time.gmtime()) + uuid.uuid4().hex[:6]
    WriteJson("request.json", dict(id=id, kind=kind, **extra))
    start = time.time()
    while time.time() - start < 60:
        result = ReadJson("result.json")
        if result and result.get("id") == id:
            print(f"{kind}: {'ok' if result.get('ok') else 'FAILED'}: {result.get('message')}")
            return 0 if result.get("ok") else 1
        time.sleep(1)
    print(f"{kind}: no answer from the watcher in 60 s.")
    return 1


def Status():
    watcher = Watcher()
    if not watcher:
        print("watcher: NOT RUNNING (Tools\\game_watcher.bat)")
    else:
        print(f"watcher: ok | MCC {'running pid ' + str(watcher.get('pid')) if watcher.get('mccRunning') else 'not running'}"
              f" | artemis loaded since launch: {watcher.get('artemisLoaded')} | crashes: {watcher.get('crashes')}"
              f" (last {watcher.get('lastCrash')})")
    bench = Bench()
    if not bench:
        print("bench: no state.json (Artemis with BenchService not loaded yet)")
    else:
        keys = ["phase", "lobbyReady", "scenario", "match", "matchIndex", "matchCount", "tick", "engineStarts", "engineDestroys"]
        print(f"bench: {'alive' if bench['_alive'] else 'STALE'} | " + " | ".join(f"{k} {bench.get(k)}" for k in keys if k in bench))
        print(f"  last launch: {bench.get('lastLaunch') or '-'} | last leave: {bench.get('lastLeave') or '-'}")
        print(f"  configs: {', '.join(bench.get('configs') or []) or 'none'}")
    return 0 if watcher else 3


def Condition(what):
    watcher, bench = Watcher(), Bench()
    if what == "loaded":
        return bool(bench and bench["_alive"]) or bool(watcher and watcher.get("artemisLoaded"))
    if what == "lobby":
        return bool(bench and bench["_alive"] and bench.get("lobbyReady"))
    if what == "exit":
        return bool(watcher and not watcher.get("mccRunning"))
    raise SystemExit(f"unknown condition {what}")


def Wait(what, timeout):
    NeedWatcher()
    start = time.time()
    while time.time() - start < timeout:
        if Condition(what):
            print(f"wait {what}: ok after {round(time.time() - start)} s")
            return 0
        if what != "exit" and not (Watcher() or {}).get("mccRunning") and time.time() - start > 20:
            print(f"wait {what}: MCC is not running")
            return 1
        livechat.Interrupt(f"python Tools/game.py wait {what}")
        time.sleep(2)
    print(f"wait {what}: still waiting after {timeout} s (run the same command again)")
    return 2


def PrintReport(report):
    print(f"report {report.get('id')} ({report.get('scenario')}): {str(report.get('result', '?')).upper()}"
          f" | {report.get('seconds')} s | {report.get('message', '')}")
    for match in report.get("matches", []):
        timing = match.get("timing", {})
        print(f"  match {match.get('index')} {match.get('name')} [{match.get('config')}]: {str(match.get('result')).upper()}"
              f" end={match.get('end')} ticks={match.get('ticks')} "
              + " ".join(f"{k}={v}" for k, v in timing.items())
              + (f" ms/tick={1000 * timing['run'] / match['ticks']:.2f}" if match.get("ticks") and timing.get("run") else ""))
        if match.get("end") == "launch-timeout":
            print("    hint: START was not taken. Is MCC on the Reach custom games screen, without a dialog open?"
                  " (any map and gametype, even none: Artemis skips the variants check, A15 2.19)")
        for check in match.get("checks", []):
            if not check.get("ok"):
                print(f"    FAIL {check.get('name')}: {check.get('detail')}")
        passed = sum(1 for c in match.get("checks", []) if c.get("ok"))
        if match.get("checks"):
            print(f"    checks: {passed}/{len(match['checks'])} ok")
        for line in match.get("log", [])[:8]:
            print(f"    log: {line[:180]}")
        if len(match.get("log", [])) > 8:
            print(f"    log: ... {len(match['log']) - 8} more lines")
        for name, path in match.get("files", {}).items():
            local = path.replace("\\", "/")
            index = local.lower().find("/build/game/")
            print(f"    {name}: {('build/game/' + local[index + len('/build/game/'):]) if index >= 0 else path}")
    return 0 if report.get("result") == "ok" else 1


MATCH_KEYS = {"name", "config", "locals", "bots", "scale", "max_ticks", "max_seconds", "checks", "repeat", "actions", "requests",
              "probes", "ticktrace"}


def Compile(scenario, configs):
    """Turns the JSON scenario (Diseno/11 sec. 9) into the line format BenchUIService reads. Raises ValueError when it is wrong."""
    defaults = scenario.get("defaults", {})
    lines = [f"name {scenario.get('name', '')}"]
    count = 0
    for index, raw in enumerate(scenario.get("matches", [])):
        match = dict(defaults, **raw)
        unknown = set(match) - MATCH_KEYS
        if unknown:
            raise ValueError(f"match {index + 1}: unknown keys {sorted(unknown)}")
        if not match.get("config"):
            raise ValueError(f"match {index + 1}: no config")
        if configs is not None and match["config"] not in configs:
            raise ValueError(f"match {index + 1}: config '{match['config']}' is not recorded (have: {', '.join(configs) or 'none'})")
        for repeat in range(int(match.get("repeat", 1))):
            count += 1
            name = match.get("name") or match["config"]
            lines.append(f"match {name}" + (f" #{repeat + 1}" if int(match.get("repeat", 1)) > 1 else ""))
            lines.append(f"config {match['config']}")
            if "locals" in match:
                lines.append(f"locals {int(match['locals'])}")
            for team in match.get("bots", []):
                lines.append(f"bot {'auto' if team in (None, 'auto') else int(team)}")
            if "scale" in match:
                lines.append(f"scale {float(match['scale'])}")
            if match.get("max_ticks"):
                lines.append(f"max_ticks {int(match['max_ticks'])}")
            if match.get("max_seconds"):
                lines.append(f"max_seconds {float(match['max_seconds'])}")
            for key in ("actions", "requests", "probes", "checks"):
                for item in match.get(key, []):
                    words = [item] if isinstance(item, str) else list(item)
                    lines.append(f"{key[:-1]} " + " ".join(str(w) for w in words))
            if match.get("ticktrace"):
                lines.append("ticktrace")
    if not count:
        raise ValueError("the scenario has no matches")
    return "\n".join(lines) + "\n", count


def Export():
    """Compiles every Tools/scenarios/*.json into build/game/scenarios/<name>.txt, the files the Simulation tab can queue."""
    folder = os.path.join(GAME, "scenarios")
    os.makedirs(folder, exist_ok=True)
    failed = 0
    for source in sorted(glob.glob(os.path.join(ROOT, "Tools", "scenarios", "*.json"))):
        name = os.path.splitext(os.path.basename(source))[0]
        try:
            with open(source, encoding="utf-8-sig") as handle:
                text, count = Compile(json.load(handle), None)
        except ValueError as error:
            print(f"{name}: {error}")
            failed += 1
            continue
        target = os.path.join(folder, name + ".txt")
        with open(target + ".tmp", "w", encoding="utf-8", newline="\n") as handle:
            handle.write(text)
        os.replace(target + ".tmp", target)
        print(f"{name}: {count} matches -> build/game/scenarios/{name}.txt")
    return 1 if failed else 0


def Run(path, wait, timeout):
    NeedWatcher()
    with open(path, encoding="utf-8-sig") as handle:
        scenario = json.load(handle)
    bench = Bench()
    configs = bench.get("configs") if bench and bench["_alive"] else None
    try:
        text, count = Compile(scenario, configs)
    except ValueError as error:
        print(f"scenario {path}: {error}")
        return 1
    id = time.strftime("%Y%m%d-%H%M%S-", time.gmtime()) + uuid.uuid4().hex[:4]
    target = os.path.join(GAME, "inbox", id + ".txt")
    os.makedirs(os.path.dirname(target), exist_ok=True)
    with open(target + ".tmp", "w", encoding="utf-8", newline="\n") as handle:
        handle.write(text)
    os.replace(target + ".tmp", target)
    print(f"queued {id} ({scenario.get('name', os.path.basename(path))}, {count} matches)"
          + ("" if bench and bench["_alive"] else " - the bench is not alive yet, it runs when Artemis loads"))
    if not wait:
        return 0
    return WaitReport(id, timeout)


def WaitReport(id, timeout):
    start = time.time()
    while time.time() - start < timeout:
        report = ReadJson(os.path.join("report", id + ".json"))
        if report and report.get("done"):
            return PrintReport(report)
        livechat.Interrupt(f"python Tools/game.py report {id} --wait")
        time.sleep(3)
    bench = Bench() or {}
    print(f"still running after {timeout} s: phase {bench.get('phase')} match {bench.get('matchIndex')}/{bench.get('matchCount')}"
          f" tick {bench.get('tick')}. Run: python Tools/game.py report {id} --wait")
    return 2


def Log(n, every, pattern):
    try:
        with open(os.path.join(GAME, "Artemis.txt"), encoding="utf-8", errors="replace") as handle:
            lines = handle.read().splitlines()
    except OSError:
        print("no Artemis.txt copied yet")
        return 1
    if pattern:
        lines = [l for l in lines if re.search(pattern, l)]
    elif not every:
        lines = [l for l in lines if "WARNING" in l or "ERROR" in l]
    for line in lines[-n:]:
        print(line[:220])
    print(f"({len(lines)} matching lines, last {min(n, len(lines))} shown)")
    return 0


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", choices=["status", "info", "deploy", "launch", "kill", "wait", "run", "report", "log", "crashes", "export"])
    parser.add_argument("arg", nargs="?")
    parser.add_argument("--launch", action="store_true")
    parser.add_argument("--no-wait", action="store_true")
    parser.add_argument("--wait", action="store_true", help="report: wait for it to finish")
    parser.add_argument("--timeout", type=int, default=170)
    parser.add_argument("-n", type=int, default=30)
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--grep")
    args = parser.parse_args()
    os.makedirs(GAME, exist_ok=True)
    if args.command == "status":
        return Status()
    if args.command in ("deploy", "launch", "kill", "info"):
        return Request(args.command, launch=args.launch)
    if args.command == "wait":
        return Wait(args.arg or "loaded", args.timeout)
    if args.command == "run":
        return Run(args.arg, not args.no_wait, args.timeout)
    if args.command == "report":
        id = args.arg
        if not id:
            reports = sorted(glob.glob(os.path.join(GAME, "report", "*.json")), key=os.path.getmtime)
            if not reports:
                print("no reports yet")
                return 1
            id = os.path.splitext(os.path.basename(reports[-1]))[0]
        if args.wait:
            return WaitReport(id, args.timeout)
        report = ReadJson(os.path.join("report", id + ".json"))
        if not report:
            print(f"no report {id}")
            return 1
        return PrintReport(report)
    if args.command == "log":
        return Log(args.n, args.all, args.grep)
    if args.command == "export":
        return Export()
    if args.command == "crashes":
        dirs = sorted(glob.glob(os.path.join(GAME, "crashes", "*")))
        for d in dirs[-10:]:
            files = ReadJson(os.path.join(d, "files.json")) or {}
            print(f"{os.path.basename(d)}: {len(files.get('copied', []))} files, " + ", ".join(sorted(os.listdir(d)))[:200])
        print(f"({len(dirs)} crashes)")
        return 0


if __name__ == "__main__":
    sys.exit(Main())
