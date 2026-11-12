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
    (deploy and run also compile Data/Presets with Tools/presets.py and have the watcher copy them to the storage folder.)
    python Tools/game.py py SCRIPT [ARGS...] [--no-wait]   Run a script of Agent/ with the Windows venv (D137) and print its output.
    python Tools/game.py py --id ID [--wait]     Print (or keep waiting for) the output of an earlier run.

A match of a scenario may record the TickDump (Diseno/15): "tickdump": true writes the .tdump of the match, "tickdump": "full" the .tdump
and the .sdump (any other value is an error of the scenario). The report lists them as "tickdump" and "snapshot" and prints, per match,
the records, the dropped ticks and the size of the .tdump.

When a match records the TickDump, the report runs the general invariants on its .tdump (python Tools/tickdump.py assert NAME --json, through the
python request of the watcher, because the file is in the TickDumps folder of the Windows side) and prints PASS or the failing rules under the match.
A match that fails them, or whose assert cannot run, makes the scenario FAIL with the reason tickdump-assert. An assert that is still running
(exit 2) is resumed with game.py py --id ID --wait.

Exit code: 0 ok, 1 failed (request or scenario), 2 still waiting (run the same wait again), 3 the watcher is not running,
4 a LiveChat message arrived (Tools/livechat.py; it is printed with the command that resumes the wait).
"""
import argparse, glob, json, ntpath, os, re, sys, time, uuid
from datetime import datetime, timezone
import livechat, presets

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


def Send(kind, id=None, **extra):
    """Writes a request for the watcher and waits up to 60 s for its answer. return: (ok, message), message None when it did not answer."""
    NeedWatcher()
    id = id or time.strftime("%Y%m%d-%H%M%S-", time.gmtime()) + uuid.uuid4().hex[:6]
    WriteJson("request.json", dict(id=id, kind=kind, **extra))
    start = time.time()
    while time.time() - start < 60:
        result = ReadJson("result.json")
        if result and result.get("id") == id:
            return bool(result.get("ok")), result.get("message")
        time.sleep(1)
    return False, None


def Request(kind, id=None, **extra):
    ok, message = Send(kind, id, **extra)
    if message is None and not ok:
        print(f"{kind}: no answer from the watcher in 60 s.")
        return 1
    print(f"{kind}: {'ok' if ok else 'FAILED'}: {message}")
    return 0 if ok else 1


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


def AssertArguments(path):
    """(script, args) that run the general invariants on the .tdump of a match through the python request of the watcher. The file is in the TickDumps
    folder of the Windows side, which this shell cannot read: tickdump.py resolves the file name alone in that folder, like show."""
    return "Tools\\tickdump.py", ["assert", ntpath.basename(path.replace("/", "\\")), "--json"]


def RunPython(script, args, timeout):
    """Runs a script with the Windows python of the watcher. return: (exit code, stdout, stderr), exit None when the request failed or the run is still
    going after the timeout (then stdout is the id to resume with game.py py --id ID --wait). The watcher keeps ID.out and ID.err in GAME/python."""
    id = time.strftime("%Y%m%d-%H%M%S-", time.gmtime()) + uuid.uuid4().hex[:4]
    ok, message = Send("python", id=id, script=script, args=args)
    if not ok:
        return None, f"the watcher refused the run ({message})", ""
    start = time.time()
    while time.time() - start < timeout:
        state = ReadJson(os.path.join("python", id + ".json"))
        if state and state.get("done"):
            texts = []
            for suffix in ("out", "err"):
                try:
                    with open(os.path.join(GAME, "python", f"{id}.{suffix}"), encoding="utf-8", errors="replace") as handle:
                        texts.append(handle.read())
                except OSError:
                    texts.append("")
            return state.get("exit"), texts[0], texts[1]
        time.sleep(2)
    return None, id, ""


def InterpretAssert(code, out, err=""):
    """(status, lines) of the exit code, stdout and stderr of tickdump.py assert --json. status: pass (0), fail (1, the failing rules as lines),
    error (2 or anything else) or pending (the run did not finish: out is its id). An error, or a fail whose rules cannot be read, shows the last lines
    of stderr (the traceback of the script), then the ones of stdout."""
    if code == 0:
        return "pass", []
    tail = (err.strip().splitlines()[-8:] + out.strip().splitlines()[-4:]) if err.strip() else out.strip().splitlines()[-8:]
    if code == 1:
        lines = []
        try:
            report = json.loads(next(line for line in out.splitlines() if line.startswith("{")))
            for rule in report.get("rules", []):
                lines.append(f"{rule['name']}: {rule['count']} case(s)")
                lines += [f"  tick {c['tick']} handle {c['handle']} {c['detail']}".rstrip() for c in rule.get("cases", [])[:3]]
        except (StopIteration, ValueError, KeyError, TypeError):
            lines = []
        return "fail", lines or ["no rule could be read from the output:"] + tail
    return "error", tail or [f"exit {code} without output"]


def AssertTickDump(path, timeout=60, runner=RunPython):
    """Runs tickdump.py assert on the .tdump of a match. return: (status, lines) of InterpretAssert, or pending when it did not finish."""
    script, args = AssertArguments(path)
    code, out, err = runner(script, args, timeout)
    if code is None and not out.startswith("the watcher"):
        return "pending", [f"still running: python Tools/game.py py --id {out} --wait"]
    if code is None:
        return "error", [out]
    return InterpretAssert(code, out, err)


def TickDumpSummary(path):
    """One line about the .tdump of a match: records, dropped (sum of the records / footer) and size. The file is in the TickDumps
    folder of the storage folder: when this shell cannot read it, the line says how to (game.py py Tools/tickdump.py show NAME)."""
    local = path.replace("\\", "/")
    try:
        import tickdump
        dump = tickdump.Dump(local)
    except Exception as error:
        return f"(not read here: {type(error).__name__}; run: game.py py Tools/tickdump.py show {os.path.basename(local)})"
    dropped = sum(record.dropped for record in dump.records)
    footer = int(dump.footer["DroppedTicks"]) if dump.footer is not None else "no footer"
    return f"ticks={len(dump.records)} dropped={dropped} (footer {footer}) size={dump.size / 1e6:.1f} MB"


def PerfLines(perf):
    """The two lines of the perf block of a match (Bench report): where the wall clock of the run went. return: [] without a block."""
    if not perf:
        return []
    sweep, lockstep = perf.get("sweep_ms", {}), perf.get("lockstep", {})
    lines = [f"    perf: wall {perf.get('wall_ms_per_tick', 0):.2f} ms/tick | engine {perf.get('engine_ms', 0):.2f} | "
             f"read wait {perf.get('read_wait_ms', 0):.2f} | sweep avg/p50/p99/max {sweep.get('avg', 0):.2f}/{sweep.get('p50', 0):.2f}/"
             f"{sweep.get('p99', 0):.2f}/{sweep.get('max', 0):.2f} ms | lockstep {lockstep.get('waits', 0)} "
             f"avg {lockstep.get('avg_ms', 0):.2f} max {lockstep.get('max_ms', 0):.2f} ms"]
    if "object_slots_max" in perf:
        lines[0] += f" | object slots max {perf['object_slots_max']} objects max {perf.get('objects_max', 0)}"
    light = perf.get("light", {})
    if light.get("ticks"):
        lines[0] += f" | light {light['ticks']} ticks avg {light.get('avg_us', 0):.0f} max {light.get('max_us', 0):.0f} us"
    heaviest = sorted(perf.get("stages", {}).items(), key=lambda item: -item[1])[:6]
    if heaviest:
        lines.append("    stages: " + ", ".join(f"{name} {ms:.2f} ms" for name, ms in heaviest))
    return lines


def PrintReport(report, asserter=AssertTickDump):
    """Prints a report. A match with a tickdump gets the general invariants run on its .tdump (asserter): a match that fails them, or whose assert
    cannot run, makes the scenario FAIL with the reason tickdump-assert. return: 0 ok, 1 failed, 2 an assert is still running."""
    asserts = {}
    for match in report.get("matches", []):
        path = match.get("files", {}).get("tickdump")
        if path:
            asserts[match.get("index")] = asserter(path)
    statuses = [status for status, _ in asserts.values()]
    is_failed = report.get("result") != "ok" or "fail" in statuses or "error" in statuses
    result = "fail" if is_failed and report.get("result") == "ok" else report.get("result", "?")
    message = report.get("message", "") + (" tickdump-assert" if "fail" in statuses or "error" in statuses else "")
    print(f"report {report.get('id')} ({report.get('scenario')}): {str(result).upper()}"
          f" | {report.get('seconds')} s | {message}")
    for match in report.get("matches", []):
        timing = match.get("timing", {})
        print(f"  match {match.get('index')} {match.get('name')} [{match.get('config')}]: {str(match.get('result')).upper()}"
              f" end={match.get('end')} ticks={match.get('ticks')} "
              + " ".join(f"{k}={v}" for k, v in timing.items())
              + (f" ms/tick={1000 * timing['run'] / match['ticks']:.2f}" if match.get("ticks") and timing.get("run") else ""))
        for line in PerfLines(match.get("perf")):
            print(line)
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
            if name == "tickdump":
                print(f"      {TickDumpSummary(path)}")
        if match.get("index") in asserts:
            status, lines = asserts[match.get("index")]
            print(f"    tickdump assert: {status.upper()}")
            for line in lines:
                print(f"      {line}")
    if is_failed:
        return 1
    return 2 if "pending" in statuses else 0


MATCH_KEYS = {"name", "config", "preset", "map", "locals", "bots", "roster", "rosters", "setups", "scale", "max_ticks", "max_seconds", "checks", "repeat", "actions", "requests",
              "probes", "tickdump", "events", "every_tick"}


MAX_TEAMS = 8


def RosterLine(match_number, roster, what):
    """Words of a roster for BenchUIService (D284): the bots per team 0..7, then auto. roster = {"0": 7, "1": 8, "auto": 0}."""
    if not isinstance(roster, dict):
        raise ValueError(f"match {match_number}: {what} must be an object like " + '{"0": 7, "1": 8, "auto": 0}' + f", not {roster!r}")
    counts = [0] * MAX_TEAMS
    auto = 0
    for key, value in roster.items():
        if isinstance(value, bool) or not isinstance(value, int) or value < 0:
            raise ValueError(f"match {match_number}: {what} count of {key!r} must be an integer >= 0, not {value!r}")
        if str(key) == "auto":
            auto = value
        elif str(key).isdigit() and int(key) < MAX_TEAMS:
            counts[int(key)] = value
        else:
            raise ValueError(f"match {match_number}: {what} key {key!r} is not a team 0-{MAX_TEAMS - 1} or auto")
    return " ".join(str(count) for count in counts) + f" auto {auto}"


def RosterFromBots(match_number, bots):
    """The roster of the bots shortcut: how many bots on each team, and how many on the team of the local player (null or auto)."""
    roster = {}
    for team in bots:
        key = "auto" if team in (None, "auto") else str(team)
        if key != "auto" and (not key.isdigit() or int(key) >= MAX_TEAMS):
            raise ValueError(f"match {match_number}: bots takes auto or a team 0-{MAX_TEAMS - 1}, not {team!r}")
        roster[key] = roster.get(key, 0) + 1
    return roster


def PresetWord(match_number, value):
    """The preset of a setup as BenchUIService reads it: a name ([a-z0-9_], up to 31 characters) or none."""
    preset = "none" if value is None else value
    if not isinstance(preset, str) or not re.fullmatch(r"[a-z0-9_]{1,31}", preset):
        raise ValueError(f"match {match_number}: preset '{preset}' is not a preset name ([a-z0-9_], up to 31 characters) or none")
    return preset


def MapWord(match_number, value, configs):
    """The map of a setup: the name of a recorded config whose map variant it is (Q98, D266), or none."""
    variant = "none" if value is None else value
    if not isinstance(variant, str) or not re.fullmatch(r"[A-Za-z0-9 _.-]{1,64}", variant) or variant != variant.strip():
        raise ValueError(f"match {match_number}: map '{variant}' is not the name of a recorded config (letters, digits, spaces, _ - ., up to 64) or none")
    if configs is not None and variant != "none" and variant not in configs:
        raise ValueError(f"match {match_number}: map '{variant}' is not a recorded config (have: {', '.join(configs) or 'none'})")
    return variant


SETUP_KEYS = {"preset", "map", "roster"}


def SetupFields(match_number, setup, configs, what):
    """The fields of one setup (D286) as BenchUIService reads them, "preset=x; map=y; roster=7 8 0 0 0 0 0 0 auto 0". A field left out
    is not written: the setup does not change it."""
    if not isinstance(setup, dict):
        raise ValueError(f"match {match_number}: {what} must be an object with preset, map and roster, not {setup!r}")
    unknown = set(setup) - SETUP_KEYS
    if unknown:
        raise ValueError(f"match {match_number}: {what} has unknown keys {sorted(unknown)} (preset, map, roster)")
    fields = []
    if "preset" in setup:
        fields.append("preset=" + PresetWord(match_number, setup["preset"]))
    if "map" in setup:
        fields.append("map=" + MapWord(match_number, setup["map"], configs))
    if "roster" in setup:
        fields.append("roster=" + RosterLine(match_number, setup["roster"], what + " roster"))
    if not fields:
        raise ValueError(f"match {match_number}: {what} changes nothing (give preset, map or roster)")
    return "; ".join(fields)


def Compile(scenario, configs):
    """Turns the JSON scenario (Diseno/11 sec. 9) into the line format BenchUIService reads. Raises ValueError when it is wrong."""
    defaults = scenario.get("defaults", {})
    lines = [f"name {scenario.get('name', '')}"]
    count = 0
    for index, raw in enumerate(scenario.get("matches", [])):
        match = dict(defaults, **raw)
        # One way to say the bots at load per match: the one of the match itself replaces the one of the defaults (D284).
        if "roster" in raw and "bots" in raw:
            raise ValueError(f"match {index + 1}: roster and bots cannot go together (bots is a shortcut of roster)")
        if "roster" in raw:
            match.pop("bots", None)
        if "bots" in raw:
            match.pop("roster", None)
        if match.get("roster") is not None and "bots" in match:
            raise ValueError(f"match {index + 1}: roster and bots cannot go together (bots is a shortcut of roster)")
        if match.get("setups") and match.get("rosters"):
            raise ValueError(f"match {index + 1}: setups and rosters cannot go together (rosters is a shortcut of setups)")
        unknown = set(match) - MATCH_KEYS
        if unknown:
            raise ValueError(f"match {index + 1}: unknown keys {sorted(unknown)}")
        if match.get("tickdump") not in (None, False, True, "full"):
            raise ValueError(f"match {index + 1}: tickdump must be true (the .tdump) or \"full\" (.tdump and .sdump), not {match['tickdump']!r}")
        if match.get("every_tick", True) not in (True, False):
            raise ValueError(f"match {index + 1}: every_tick must be true or false, not {match['every_tick']!r}")
        if match.get("every_tick") is False and (match.get("tickdump") or match.get("events")):
            raise ValueError(f"match {index + 1}: every_tick false (sparse allowed, Q101) cannot go with tickdump or events")
        if not match.get("config"):
            raise ValueError(f"match {index + 1}: no config")
        if configs is not None and match["config"] not in configs:
            raise ValueError(f"match {index + 1}: config '{match['config']}' is not recorded (have: {', '.join(configs) or 'none'})")
        for repeat in range(int(match.get("repeat", 1))):
            count += 1
            name = match.get("name") or match["config"]
            lines.append(f"match {name}" + (f" #{repeat + 1}" if int(match.get("repeat", 1)) > 1 else ""))
            lines.append(f"config {match['config']}")
            if "preset" in match:
                lines.append("preset " + PresetWord(index + 1, match["preset"]))
            if "map" in match:
                lines.append("map " + MapWord(index + 1, match["map"], configs))
            if "locals" in match:
                lines.append(f"locals {int(match['locals'])}")
            at_load = match.get("roster")
            if at_load is None and "bots" in match:
                at_load = RosterFromBots(index + 1, match["bots"])
            if at_load is not None:
                lines.append("roster " + RosterLine(index + 1, at_load, "roster"))
            # One setup per next round (D286): the rosters shortcut is the setups that only change the roster.
            setups = match.get("setups")
            if setups is not None and not isinstance(setups, list):
                raise ValueError(f"match {index + 1}: setups must be a list of objects with preset, map and roster")
            rosters = match.get("rosters")
            if rosters is not None and not isinstance(rosters, list):
                raise ValueError(f"match {index + 1}: rosters must be a list of rosters")
            if rosters:
                if at_load is None:
                    raise ValueError(f"match {index + 1}: rosters need a roster (or bots) at load: round 0 is checked against it")
                setups = [{"roster": roster} for roster in rosters]
            for setup in setups or []:
                lines.append("round_setup " + SetupFields(index + 1, setup, configs, "setups" if match.get("setups") else "rosters"))
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
            if match.get("tickdump"):
                lines.append("tickdump" + (" full" if match["tickdump"] == "full" else ""))
            if match.get("events"):
                lines.append("events")
            if match.get("every_tick") is False:
                lines.append("every_tick off")
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


def PrintPython(id, n):
    for suffix in ("out", "err"):
        path = os.path.join(GAME, "python", f"{id}.{suffix}")
        try:
            with open(path, encoding="utf-8", errors="replace") as handle:
                lines = handle.read().splitlines()
        except OSError:
            continue
        if lines:
            print(f"--- {suffix} ({len(lines)} lines{', last ' + str(n) if len(lines) > n else ''})")
            print("\n".join(lines[-n:]))


def WaitPython(id, timeout, n):
    start = time.time()
    while time.time() - start < timeout:
        state = ReadJson(os.path.join("python", id + ".json"))
        if state and state.get("done"):
            PrintPython(id, n)
            print(f"python {id}: exit {state.get('exit')}")
            return 0 if state.get("exit") == 0 else 1
        livechat.Interrupt(f"python Tools/game.py py --id {id} --wait")
        time.sleep(2)
    PrintPython(id, n)
    print(f"python {id}: still running after {timeout} s. Run: python Tools/game.py py --id {id} --wait")
    return 2


def Py(script, rest, wait, timeout, n):
    id = time.strftime("%Y%m%d-%H%M%S-", time.gmtime()) + uuid.uuid4().hex[:4]
    script = script.replace("/", "\\")
    if Request("python", id=id, script=script, args=rest) != 0:
        return 1
    print(f"python run id {id}")
    return WaitPython(id, timeout, n) if wait else 0


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", choices=["status", "info", "deploy", "launch", "kill", "wait", "run", "report", "log", "crashes", "export", "py"])
    parser.add_argument("arg", nargs="?")
    parser.add_argument("--launch", action="store_true")
    parser.add_argument("--no-wait", action="store_true")
    parser.add_argument("--wait", action="store_true", help="report: wait for it to finish")
    parser.add_argument("--timeout", type=int, default=170)
    parser.add_argument("-n", type=int, default=30)
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--grep")
    parser.add_argument("--id", help="py: an earlier run")
    args, rest = parser.parse_known_args()
    if rest and args.command != "py":
        parser.error("unrecognized arguments: " + " ".join(rest))
    os.makedirs(GAME, exist_ok=True)
    if args.command == "status":
        return Status()
    if args.command in ("deploy", "launch", "kill", "info"):
        if args.command == "deploy" and presets.Export():
            return 1
        return Request(args.command, launch=args.launch)
    if args.command == "wait":
        return Wait(args.arg or "loaded", args.timeout)
    if args.command == "run":
        if presets.Export() or Request("presets"):
            return 1
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
    if args.command == "py":
        if args.id:
            if args.wait:
                return WaitPython(args.id, args.timeout, args.n)
            PrintPython(args.id, args.n)
            return 0
        if not args.arg:
            parser.error("py needs a script (Agent/scripts/...)")
        return Py(args.arg, rest, not args.no_wait, args.timeout, args.n)
    if args.command == "crashes":
        dirs = sorted(glob.glob(os.path.join(GAME, "crashes", "*")))
        for d in dirs[-10:]:
            files = ReadJson(os.path.join(d, "files.json")) or {}
            print(f"{os.path.basename(d)}: {len(files.get('copied', []))} files, " + ", ".join(sorted(os.listdir(d)))[:200])
        print(f"({len(dirs)} crashes)")
        return 0


if __name__ == "__main__":
    sys.exit(Main())
