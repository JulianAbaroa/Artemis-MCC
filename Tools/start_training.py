r"""
start_training.py
Starts a long training run by hand on the Windows side (no Claude needed): queues the bench scenario, waits until its first match is
Running, then opens two windows: train_stage0.py (PPO) and hw_monitor.py (heat / clocks). Run it with the venv of Agent\ (D137) from
Tools\start_training.bat, which passes its arguments here. The game watcher (Tools\game_watcher.bat) must be open and MCC must be in
the Reach custom games lobby with the config of the scenario (MCC in the green team for "Map M.1").

    Tools\start_training.bat                                    the night run: hitoa_noche, 2500 updates, from scratch
    Tools\start_training.bat --name prueba_2300 --scenario Ignore\Temporal\vw14_prueba.json --updates 60
    (--steps 1024 --max-seconds 150 --snapshot-every 50 by default; --init RUN\policy.pt to start from a policy)

A run name that already has a folder in Agent\runs is refused (the log would mix two runs). Closing the train window stops the
training; the scenario keeps the bots in the game (quiet) until its matches end. Afterwards:
    Agent\.venv\Scripts\python.exe Agent\scripts\hw_monitor.py --summary NAME --run NAME --bin 30
"""
import argparse, json, os, re, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
STATE = os.path.join(ROOT, "build", "game", "state.json")


def ReadState():
    try:
        with open(STATE, encoding="utf-8-sig") as handle:
            return json.load(handle)
    except (OSError, ValueError):
        return {}


def Window(title, command):
    """Opens command in a new console that stays open when it ends (cmd /k), so its last lines can be read in the morning."""
    flags = getattr(subprocess, "CREATE_NEW_CONSOLE", 0)
    return subprocess.Popen(["cmd", "/k", "title", title.replace(" ", "_"), "&"] + command, cwd=ROOT, creationflags=flags)


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--name", default="hitoa_noche")
    parser.add_argument("--scenario", default=os.path.join("Ignore", "Temporal", "hitoa_noche.json"))
    parser.add_argument("--updates", type=int, default=2500)
    parser.add_argument("--steps", type=int, default=1024)
    parser.add_argument("--max-seconds", type=float, default=150.0)
    parser.add_argument("--snapshot-every", type=int, default=50)
    parser.add_argument("--init", help="policy.pt to start from (the night run starts from scratch: leave it out)")
    parser.add_argument("--start-timeout", type=float, default=300.0, help="seconds to wait for the first match to be Running")
    args = parser.parse_args()
    python = sys.executable
    if os.path.exists(os.path.join(ROOT, "Agent", "runs", args.name)):
        print(f"FAIL Agent\\runs\\{args.name} already exists: use another --name.")
        return 1
    state = ReadState()
    print(f"bench now: phase {state.get('phase')} | lobbyReady {state.get('lobbyReady')} | {state.get('message', '')}")
    if state.get("phase") == "Running":
        print("FAIL the bench is already running a scenario: wait for it to end (or restart MCC) and try again.")
        return 1
    queued = subprocess.run([python, os.path.join("Tools", "game.py"), "run", args.scenario, "--no-wait"], cwd=ROOT,
                            capture_output=True, text=True)
    print((queued.stdout + queued.stderr).strip())
    found = re.search(r"queued (\S+)", queued.stdout)
    if queued.returncode != 0 or not found:
        print("FAIL the scenario was not queued (is Tools\\game_watcher.bat open?).")
        return 1
    scenario_id = found.group(1)
    print("waiting for the first match to start (MCC must be in the custom games lobby)...")
    start = time.time()
    while True:
        state = ReadState()
        if state.get("scenario") == scenario_id and state.get("phase") == "Running":
            break
        if time.time() - start > args.start_timeout:
            print(f"FAIL no match Running after {args.start_timeout:.0f} s: {state.get('phase')} | {state.get('message', '')}")
            return 1
        time.sleep(2)
    print(f"{time.strftime('%H:%M:%S')} Running: {state.get('message', '')}")
    hw = [python, os.path.join("Agent", "scripts", "hw_monitor.py"), "--name", args.name]
    train = [python, os.path.join("Agent", "scripts", "train_stage0.py"), "--updates", str(args.updates), "--steps", str(args.steps),
             "--max-seconds", str(args.max_seconds), "--snapshot-every", str(args.snapshot_every), "--name", args.name]
    if args.init:
        train += ["--init", args.init]
    Window("Artemis hw " + args.name, hw)
    Window("Artemis train " + args.name, train)
    print(f"started: training '{args.name}' ({args.updates} updates, ~{args.updates * 10 / 3600:.1f} h at ~10 s per update)")
    print(f"  log: Agent\\runs\\{args.name}\\log.txt | heat: Agent\\runs\\hw\\{args.name}.csv")
    return 0


if __name__ == "__main__":
    sys.exit(Main())
