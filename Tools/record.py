r"""
record.py
Records an episode on its own (REC.1, Diseno/25): walks the recording plan, plays every take with its two policies and its director
sequence, and leaves ONE video in OBS with the takes glued together plus a marks file (JSON and an EDL of markers for DaVinci).
Runs on the Windows side with the venv of Agent\ (obsws-python is in its requirements) from Tools\record.bat, which passes its arguments
here. Needs: the game watcher (Tools\game_watcher.bat) open, MCC in the Reach custom games lobby with the config of the plan, and OBS
open with the WebSocket server on (Tools > WebSocket Server Settings, port 4455). The password goes in %LOCALAPPDATA%\Artemis\obs.json
({"host": "localhost", "port": 4455, "password": "..."}; host and port are optional) or in the variable ARTEMIS_OBS_PASSWORD. It is
never printed and never lives in the repo.

    Tools\record.bat "Ignore\Content\Episode 1\grabaciones.json" --dry-run      checks everything, plays and records nothing
    Tools\record.bat "Ignore\Content\Episode 1\grabaciones.json" --only R3,R8   only those takes (a disabled take runs if named)
    Tools\record.bat "Ignore\Content\Episode 1\grabaciones.json"                 the whole plan, then go away

Plan (JSON, paths relative to the repo):
  { "name": "E01", "config": "Map M.1", "snapshots": "Agent/runs/hitoa_noche/snapshots", "output": "Ignore/Content/Episode 1/Grabaciones",
    "max_seconds": 150, "takes": [ { "id", "red", "blue", "scale", "rounds", "sequence", "note", optional "config", "max_seconds",
    "enabled", "manual" } ] }
  red is --policy (the lowest Team, Red) and blue is --policy2: "u02300" is <snapshots>/policy_u02300.pt, seek / random are themselves,
  anything else is a path. "enabled": false skips the take unless --only names it. "max_seconds" is the cap of one round for env_check
  (the take's, else the plan's); the match of the bench gets rounds * that / max(scale, 1) + 60 s (wall seconds counted from Running: a safety
  net, see the skip below).
  The scale is set by env_check (--scale). env_check plays sampling (no --greedy) and with the full pipeline (no --sparse, every_tick
  false only keeps the bench from raising the every-tick demand).
  "manual": true = the take does not end by rounds: env_check runs with --episodes 0 (until stopped), "rounds" is ignored (the plan
  still needs it >= 1), the match gets a cap of 1800 s (or the take's "max_seconds" when given) and the take lasts until YOU end it,
  driving the Free camera in MCC. Global hotkeys (RegisterHotKey, work with MCC focused), registered only during a manual take:
      F9  = new attempt: marker "<id> attempt N" at the video time (the first attempt starts with the take)
      F10 = end the take: env_check stops cleanly (its summary is written) and the plan goes on with the next take
  --key-attempt / --key-end change them (F1..F24, A..Z, 0..9). If a key cannot be registered (another app owns it, an OBS hotkey) it
  says so and falls back to the console of record.py: Enter or "a" + Enter = attempt, "e" + Enter = end.

When a take ends (env_check finished, rounds done, F10, error or Ctrl+C) record.py asks the bench to end that match ("game.py skip", REC.2:
the match ends as "skipped", a clean end) instead of waiting for its cap, so the next take starts right after loading and the last match does
not leave the bots standing. If the skip does not work (the bench does not move on in 30 s) it waits for the cap as before.

One OBS recording per run: StartRecord at the first take, PauseRecord / ResumeRecord between takes (no loading, no quiet bots in the
video), StopRecord at the end. Output, in the plan's "output" folder: <name>_<YYYY-MM-DD_HHMM>_marcas.json (start / end of every take
in seconds of the VIDEO, the OBS file and the offset inside it, the attempts of a manual take, the env_check summary), the .edl and, per
take, <name>_<id>.txt (env_check output) and <name>_<id>.json (its summary). A failed take is an error in the marks and the plan goes
on. Ctrl+C: env_check is stopped (its summary is printed), the recording is stopped and the marks are written up to there.
"""
import argparse, json, logging, os, re, subprocess, sys, threading, time, unicodedata
from dataclasses import dataclass, field

TOOLS = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(TOOLS)
if TOOLS not in sys.path:
    sys.path.insert(0, TOOLS)

FPS = 60
LOAD_MARGIN_SECONDS = 60.0
MANUAL_CAP_SECONDS = 1800.0
DEFAULT_ROUND_SECONDS = 90.0
EDL_START = "01:00:00:00"
KEY_ATTEMPT = "F9"
KEY_END = "F10"
TAKE_KEYS = {"id", "red", "blue", "scale", "rounds", "sequence", "note", "config", "max_seconds", "enabled", "manual"}
PLAN_KEYS = {"name", "config", "snapshots", "output", "max_seconds", "takes"}
SNAPSHOT = re.compile(r"^u\d+$")


class ObsError(Exception):
    """OBS did not answer or refused a request."""


class HotkeyError(Exception):
    """The global hotkeys could not be registered."""


@dataclass
class Take:
    id: str
    red: str
    blue: str
    scale: float
    rounds: int
    sequence: str
    note: str = ""
    config: str = ""
    max_seconds: float = 0.0
    is_enabled: bool = True
    is_manual: bool = False
    match_seconds: float = 0.0


@dataclass
class Plan:
    name: str
    config: str
    snapshots: str
    output: str
    max_seconds: float
    takes: list = field(default_factory=list)


def Number(value, what, minimum, is_int=False):
    if isinstance(value, bool) or not isinstance(value, (int,) if is_int else (int, float)):
        raise ValueError(f"{what} must be {'an integer' if is_int else 'a number'}, not {value!r}")
    if value < minimum:
        raise ValueError(f"{what} must be >= {minimum}, not {value!r}")
    return value


def ParsePlan(raw):
    """Plan from the dict of the JSON, validated. Raises ValueError with a clear message."""
    if not isinstance(raw, dict):
        raise ValueError("the plan must be a JSON object")
    unknown = set(raw) - PLAN_KEYS
    if unknown:
        raise ValueError(f"unknown plan keys {sorted(unknown)}")
    for key in ("name", "config", "output"):
        if not isinstance(raw.get(key), str) or not raw[key].strip():
            raise ValueError(f"plan: {key} must be a non-empty string")
    if re.search(r'[\\/:*?"<>|]', raw["name"]):
        raise ValueError(f"plan: name {raw['name']!r} cannot be part of a file name")
    plan_seconds = float(Number(raw.get("max_seconds", DEFAULT_ROUND_SECONDS), "plan max_seconds", 1))
    plan = Plan(raw["name"], raw["config"], raw.get("snapshots") or "", raw["output"], plan_seconds)
    if not isinstance(raw.get("takes"), list) or not raw["takes"]:
        raise ValueError("plan: takes must be a non-empty list")
    seen = set()
    for index, item in enumerate(raw["takes"]):
        where = f"take {index + 1}"
        if not isinstance(item, dict):
            raise ValueError(f"{where}: must be an object")
        unknown = set(item) - TAKE_KEYS
        if unknown:
            raise ValueError(f"{where}: unknown keys {sorted(unknown)}")
        for key in ("id", "red", "blue", "sequence"):
            if not isinstance(item.get(key), str) or not item[key].strip():
                raise ValueError(f"{where}: {key} must be a non-empty string")
        where = f"take {item['id']}"
        if not re.match(r"^[A-Za-z0-9_.-]+$", item["id"]):
            raise ValueError(f"{where}: id may only have letters, digits, _ . - (it is part of file names)")
        if item["id"] in seen:
            raise ValueError(f"{where}: id repeated")
        seen.add(item["id"])
        if any(c in item["sequence"] for c in "|\r\n"):
            raise ValueError(f"{where}: sequence cannot have | or line breaks")
        for key in ("enabled", "manual"):
            if key in item and not isinstance(item[key], bool):
                raise ValueError(f"{where}: {key} must be true or false, not {item[key]!r}")
        rounds = Number(item.get("rounds"), f"{where}: rounds", 1, is_int=True)
        scale = float(Number(item.get("scale", 1), f"{where}: scale", 0))
        config = item.get("config", plan.config)
        if not isinstance(config, str) or not config.strip():
            raise ValueError(f"{where}: config must be a non-empty string")
        given = float(Number(item["max_seconds"], f"{where}: max_seconds", 1)) if "max_seconds" in item else 0.0
        is_manual = bool(item.get("manual", False))
        round_seconds = given or plan_seconds
        # Wall seconds from Running: the rounds at their cap, at the speed of the scale (env_check --scale), plus a margin. Only a safety net,
        # every take is skipped when it ends.
        match_seconds = (given or MANUAL_CAP_SECONDS) if is_manual else rounds * round_seconds / max(scale, 1.0) + LOAD_MARGIN_SECONDS
        plan.takes.append(Take(item["id"], item["red"].strip(), item["blue"].strip(), scale, rounds, item["sequence"].strip(),
                               str(item.get("note", "")), config.strip(), round_seconds, item.get("enabled", True), is_manual,
                               match_seconds))
    return plan


def LoadPlan(path):
    try:
        with open(path, encoding="utf-8-sig") as handle:
            return ParsePlan(json.load(handle))
    except OSError as error:
        raise ValueError(f"cannot read the plan {path}: {error}") from error
    except json.JSONDecodeError as error:
        raise ValueError(f"the plan {path} is not valid JSON: {error}") from error


def Select(plan, only):
    """Takes that run, in plan order: the enabled ones, or the ones --only names (enabled or not). An unknown name raises ValueError."""
    if only is None:
        return [t for t in plan.takes if t.is_enabled]
    names = [n.strip() for n in only if n.strip()]
    known = {t.id for t in plan.takes}
    missing = [n for n in names if n not in known]
    if missing:
        raise ValueError(f"--only names takes the plan does not have: {missing} (have: {sorted(known)})")
    return [t for t in plan.takes if t.id in names]


def AbsPath(path, root=ROOT):
    return path if os.path.isabs(path) else os.path.join(root, *re.split(r"[\\/]", path))


def ResolvePolicy(value, plan, root=ROOT):
    """Argument of env_check for a red / blue value: uNNNNN -> <snapshots>/policy_uNNNNN.pt, seek / random as they are, else a path."""
    if value in ("seek", "random"):
        return value
    if SNAPSHOT.match(value):
        if not plan.snapshots:
            raise ValueError(f"policy {value}: the plan has no 'snapshots' folder")
        return os.path.join(AbsPath(plan.snapshots, root), f"policy_{value}.pt")
    return AbsPath(value, root)


def BuildScenario(plan, takes):
    """Bench scenario of the run: one match per take, in order. Scale is not here (env_check sets it)."""
    return {"name": f"rec_{plan.name}",
            "matches": [{"name": f"{plan.name} {t.id}", "config": t.config, "bots": [0, 1], "every_tick": False, "director": t.sequence,
                         "max_seconds": t.match_seconds} for t in takes]}


def SequencesOf(text):
    """Names of the sequences of a director file (lines 'sequence|<name>|<ids>')."""
    names = []
    for line in text.lstrip("﻿").splitlines():
        parts = line.strip().split("|")
        if len(parts) >= 2 and parts[0] == "sequence":
            names.append(parts[1].strip())
    return names


def SanitizeMapKey(name):
    """Same as SanitizeMapKey of DevTools.Viewer.Director (the file name of a map)."""
    return "".join(c if (c.isascii() and (c.isalnum() or c in "-_.")) else "_" for c in name) or "default"


def DirectorFolder(env=None):
    base = (env if env is not None else os.environ).get("LOCALAPPDATA")
    return os.path.join(base, "Artemis", "Director") if base else None


def CheckSequences(takes, folder):
    """(errors, warnings): every sequence of the takes exists in a director file. The file of a map is <SanitizeMapKey(map variant)>.txt;
    the plan only knows the config, so the file of that name is tried first and then every file of the folder."""
    if not folder or not os.path.isdir(folder):
        return [], [f"director folder not readable ({folder}): sequences not checked"]
    files = {}
    for name in sorted(os.listdir(folder)):
        if name.lower().endswith(".txt"):
            try:
                with open(os.path.join(folder, name), encoding="utf-8-sig") as handle:
                    files[name] = SequencesOf(handle.read())
            except OSError:
                continue
    errors = []
    for take in takes:
        first = SanitizeMapKey(take.config) + ".txt"
        if take.sequence in files.get(first, []) or any(take.sequence in names for names in files.values()):
            continue
        errors.append(f"take {take.id}: sequence {take.sequence!r} is in no director file of {folder} "
                      f"({sum(len(v) for v in files.values())} sequences in {len(files)} files)")
    return errors, []


def EstimateSeconds(take):
    """Wall seconds of a take: the rounds at their cap plus loading (the take is skipped when it ends, no wait for the cap of the match).
    Manual takes: the cap of the match."""
    if take.is_manual:
        return take.match_seconds
    return take.rounds * take.max_seconds / max(take.scale, 1.0) + 30.0


def Timecode(frames, fps=FPS):
    frames = int(frames)
    return f"{frames // (fps * 3600):02d}:{frames // (fps * 60) % 60:02d}:{frames // fps % 60:02d}:{frames % fps:02d}"


def ParseTimecode(text, fps=FPS):
    h, m, s, f = (int(x) for x in text.split(":"))
    return ((h * 60 + m) * 60 + s) * fps + f


def MarkerName(text):
    """Marker names for Resolve: ASCII, no | (the separator of its comment line), no line breaks."""
    ascii_text = unicodedata.normalize("NFKD", text).encode("ascii", "ignore").decode("ascii")
    return re.sub(r"[|\r\n\t]", " ", ascii_text).strip()


def Edl(title, markers, start=EDL_START, fps=FPS):
    """EDL CMX3600 of markers for DaVinci Resolve (Timeline > Import > Timeline Markers from EDL). markers: (name, start_s, seconds).
    Format (what Resolve itself writes when it exports markers to EDL; sources: Blackmagic forum "EDL file format?"
    https://forum.blackmagicdesign.com/viewtopic.php?p=727227 and https://editingtools.io/guides/convert-livenotes-to-resolve-marker
    which imports into Resolve with this layout): a header 'TITLE:' / 'FCM: NON-DROP FRAME', then per marker an event line (source in
    = record in, out one frame later, on V) and a comment line ' |C:<color> |M:<name> |D:<duration in frames>'. The timecodes start at
    the timeline start (Resolve's default is 01:00:00:00): the markers must fall inside the timeline."""
    base = ParseTimecode(start, fps)
    lines = [f"TITLE: {MarkerName(title)}", "FCM: NON-DROP FRAME", ""]
    for number, (name, start_s, seconds) in enumerate(markers, 1):
        first = base + int(round(start_s * fps))
        frames = max(1, int(round(seconds * fps)))
        tin, tout = Timecode(first, fps), Timecode(first + 1, fps)
        lines.append(f"{number:03d}  001      V     C        {tin} {tout} {tin} {tout}")
        lines.append(f" |C:ResolveColorBlue |M:{MarkerName(name)} |D:{frames}")
        lines.append("")
    return "\n".join(lines)


def ObsSettings(env=None, path=None):
    """(host, port, password) from obs.json of %LOCALAPPDATA%\\Artemis or ARTEMIS_OBS_PASSWORD. Raises ValueError."""
    env = os.environ if env is None else env
    if path is None and env.get("LOCALAPPDATA"):
        path = os.path.join(env["LOCALAPPDATA"], "Artemis", "obs.json")
    if path and os.path.isfile(path):
        try:
            with open(path, encoding="utf-8-sig") as handle:
                data = json.load(handle)
        except (OSError, ValueError) as error:
            raise ValueError(f"{path} cannot be read: {error}") from error
        if not isinstance(data, dict):
            raise ValueError(f"{path} must be an object with host, port and password")
        port = data.get("port", 4455)
        if isinstance(port, bool) or not isinstance(port, int):
            raise ValueError(f"{path}: port must be an integer")
        return str(data.get("host", "localhost")), port, str(data.get("password", ""))
    if "ARTEMIS_OBS_PASSWORD" in env:
        return "localhost", 4455, env["ARTEMIS_OBS_PASSWORD"]
    raise ValueError("no OBS settings: write %LOCALAPPDATA%\\Artemis\\obs.json with host, port and password, or set ARTEMIS_OBS_PASSWORD")


class Obs:
    """What record.py needs of OBS over a client with the obsws_python ReqClient methods (a fake in the tests). Keeps the file of the
    recording (RecordFileChanged) and a clock in seconds of the video: if OBS counts the paused time in outputDuration the clock
    subtracts it (checked at every Resume)."""

    def __init__(self, req, secret="", clock=time.monotonic):
        self.req, self.secret, self.clock = req, secret, clock
        self.lock = threading.RLock()
        self.offset = 0.0
        self.files = []
        self.pause_counted = None
        self.chapters = None
        self._paused_at = None

    def Call(self, name, *args):
        with self.lock:
            try:
                return getattr(self.req, name)(*args)
            except Exception as error:
                text = f"{type(error).__name__}: {error}"
                raise ObsError(f"OBS {name}: {text.replace(self.secret, '***') if self.secret else text}") from error

    def Status(self):
        return self.Call("get_record_status")

    def Wait(self, check, what, seconds=8.0):
        end = time.monotonic() + seconds
        while True:
            status = self.Status()
            if check(status):
                return status
            if time.monotonic() > end:
                raise ObsError(f"OBS did not report {what} in {seconds:.0f} s")
            time.sleep(0.05)

    def Video(self, status):
        return status.output_duration / 1000.0 - self.offset

    def Position(self):
        """{"video_s", "file", "file_offset_s"} now."""
        video = self.Video(self.Status())
        return self.Locate(video)

    def Locate(self, video):
        current = {"path": None, "start_s": 0.0}
        for entry in self.files:
            if entry["start_s"] <= video + 1e-6:
                current = entry
        return {"video_s": round(video, 3), "file": current["path"], "file_offset_s": round(video - current["start_s"], 3)}

    def OnState(self, state, path):
        """RecordStateChanged: the first file comes with the started event."""
        if path and "STARTED" in str(state):
            if not self.files:
                self.files.append({"path": path, "start_s": 0.0})
            elif self.files[0]["path"] is None:
                self.files[0]["path"] = path

    def OnFileChanged(self, path):
        """RecordFileChanged (OBS splits the file): from here on the offsets are inside the new file."""
        video = self.Video(self.Status())
        with self.lock:
            self.files.append({"path": path, "start_s": video})

    def Start(self):
        self.Call("start_record")
        self.Wait(lambda s: s.output_active, "the recording started")
        if not self.files:
            self.files.append({"path": None, "start_s": 0.0})

    def Pause(self):
        before = self.Status().output_duration / 1000.0
        self.Call("pause_record")
        self.Wait(lambda s: s.output_paused, "the recording paused")
        self._paused_at = (before, self.clock())

    def Resume(self):
        self.Call("resume_record")
        status = self.Wait(lambda s: not s.output_paused, "the recording resumed")
        if self._paused_at is not None:
            before, at = self._paused_at
            wall, gap = self.clock() - at, status.output_duration / 1000.0 - before
            if wall >= 2.0:
                if gap > max(1.0, 0.5 * wall):
                    self.offset += gap
                    self.pause_counted = True
                    print(f"note: OBS counted the paused time ({gap:.1f} s) in outputDuration: the marks subtract it")
                elif self.pause_counted is None:
                    self.pause_counted = False
            self._paused_at = None

    def Chapter(self, name):
        if self.chapters is False:
            return
        try:
            self.Call("create_record_chapter", name)
            self.chapters = True
        except ObsError as error:
            self.chapters = False
            print(f"note: no chapters in the video ({error}); the marks file and the EDL still have the takes")

    def Stop(self):
        return getattr(self.Call("stop_record"), "output_path", None)


def ConnectObs(host, port, password):
    """Obs over obsws_python (imported here: it is only in the venv of Agent\\), with the events of the record state."""
    try:
        import obsws_python as obsws
    except ImportError as error:
        raise ObsError("obsws-python is not installed: Agent\\.venv\\Scripts\\python.exe -m pip install -r Agent\\requirements.txt") from error
    logging.getLogger("obsws_python").setLevel(logging.CRITICAL)  # its own tracebacks: the message below says it better
    try:
        req = obsws.ReqClient(host=host, port=port, password=password, timeout=10)
    except Exception as error:
        text = f"{type(error).__name__}: {error}"
        if password:
            text = text.replace(password, "***")
        raise ObsError(f"cannot connect to OBS at {host}:{port} ({text}): is OBS open with the WebSocket server on and the password right?") from error
    obs = Obs(req, password)
    try:
        events = obsws.EventClient(host=host, port=port, password=password)

        def on_record_state_changed(data):
            obs.OnState(getattr(data, "output_state", ""), getattr(data, "output_path", None))

        def on_record_file_changed(data):
            try:
                obs.OnFileChanged(getattr(data, "new_output_path", None))
            except ObsError as error:
                print(f"note: RecordFileChanged not tracked: {error}")
        events.callback.register([on_record_state_changed, on_record_file_changed])
        obs.events = events
    except Exception as error:
        print(f"note: no OBS events ({type(error).__name__}); a file split of OBS would not be followed")
    return obs


class Marks:
    """The marks of a run: written after every take (JSON and EDL) so a power cut keeps what is done."""

    def __init__(self, plan, plan_path, takes, folder, stamp, started):
        self.plan, self.folder = plan, folder
        self.base = os.path.join(folder, f"{plan.name}_{stamp}")
        self.data = {"plan": plan.name, "plan_file": plan_path, "started": started, "fps": FPS, "obs": {}, "files": [],
                     "pause_counted_in_duration": None, "chapters": None, "takes": []}
        self.selected = {t.id for t in takes}
        for t in plan.takes:
            self.data["takes"].append({"id": t.id, "note": t.note, "sequence": t.sequence, "policies": {"red": t.red, "blue": t.blue},
                                       "scale": t.scale, "rounds": t.rounds, "manual": t.is_manual,
                                       "status": "pending" if t.id in self.selected else "skipped", "error": None, "start_s": None,
                                       "end_s": None, "file": None, "file_offset_s": None, "attempts": [], "summary": None})

    def Take(self, take_id):
        return next(t for t in self.data["takes"] if t["id"] == take_id)

    def Begin(self, take_id, position):
        record = self.Take(take_id)
        record["start_s"], record["file"], record["file_offset_s"] = position["video_s"], position["file"], position["file_offset_s"]
        record["status"] = "running"
        if record["manual"]:
            self.Attempt(take_id, position)

    def Attempt(self, take_id, position):
        record = self.Take(take_id)
        number = len(record["attempts"]) + 1
        record["attempts"].append({"n": number, "start_s": position["video_s"], "end_s": None, "file": position["file"],
                                   "file_offset_s": position["file_offset_s"]})
        return number

    def Finish(self, take_id, position, status, error=None, summary=None):
        record = self.Take(take_id)
        if position is not None:
            record["end_s"] = position["video_s"]
            if record["attempts"]:
                record["attempts"][-1]["end_s"] = position["video_s"]
        record["status"], record["error"] = status, error
        if summary is not None:
            record["summary"] = summary

    def Markers(self):
        """EDL markers (name, start_s, seconds): one per take, one per attempt of a manual take (until the next attempt or the take end)."""
        markers = []
        for t in self.data["takes"]:
            if t["start_s"] is None or t["end_s"] is None:
                continue
            tag = " [ERROR]" if t["status"] == "error" else ""
            if t["manual"] and t["attempts"]:
                for index, a in enumerate(t["attempts"]):
                    end = t["attempts"][index + 1]["start_s"] if index + 1 < len(t["attempts"]) else t["end_s"]
                    markers.append((f"{t['id']} attempt {a['n']}{tag if index + 1 == len(t['attempts']) else ''}", a["start_s"], end - a["start_s"]))
            else:
                markers.append((f"{t['id']} {t['note']}".strip() + tag, t["start_s"], t["end_s"] - t["start_s"]))
        return markers

    def Write(self, obs=None):
        if obs is not None:
            self.data["files"] = [e["path"] for e in obs.files]
            self.data["pause_counted_in_duration"] = obs.pause_counted
            self.data["chapters"] = obs.chapters
        os.makedirs(self.folder, exist_ok=True)
        for suffix, text in (("_marcas.json", json.dumps(self.data, indent=1)), (".edl", Edl(self.plan.name, self.Markers()))):
            with open(self.base + suffix + ".tmp", "w", encoding="utf-8", newline="\n") as handle:
                handle.write(text + "\n")
            os.replace(self.base + suffix + ".tmp", self.base + suffix)


def ParseKey(name):
    """Virtual key code of F1..F24, A..Z or 0..9. Raises ValueError."""
    text = name.strip().upper()
    found = re.match(r"^F(\d{1,2})$", text)
    if found and 1 <= int(found.group(1)) <= 24:
        return 0x6F + int(found.group(1))
    if re.match(r"^[A-Z0-9]$", text):
        return ord(text)
    raise ValueError(f"key {name!r}: use F1..F24, A..Z or 0..9")


class Win32Hotkeys:
    """Global hotkeys (RegisterHotKey on a thread with a message loop): they work while MCC has the focus. Poll() gives the names pressed
    since the last call. Registered on creation, unregistered by Close (on the same thread, as Windows needs)."""

    def __init__(self, keys):
        import queue
        self.keys, self.events = keys, queue.Queue()
        self.ready, self.error, self.thread_id = threading.Event(), None, None
        self.thread = threading.Thread(target=self.Loop, daemon=True)
        self.thread.start()
        self.ready.wait(5)
        if self.error or not self.ready.is_set():
            raise HotkeyError(self.error or "the hotkey thread did not start")

    def Loop(self):
        import ctypes
        from ctypes import wintypes
        user32, kernel32 = ctypes.WinDLL("user32", use_last_error=True), ctypes.WinDLL("kernel32")
        self.thread_id = kernel32.GetCurrentThreadId()
        message = wintypes.MSG()
        user32.PeekMessageW(ctypes.byref(message), None, 0, 0, 0)
        registered = []
        for number, (name, code) in enumerate(self.keys.items(), 1):
            if not user32.RegisterHotKey(None, number, 0x4000, code):
                self.error = f"key {name} cannot be registered (error {ctypes.get_last_error()}): another app owns it?"
                break
            registered.append(number)
        self.ready.set()
        if not self.error:
            names = {n: name for n, name in enumerate(self.keys, 1)}
            while user32.GetMessageW(ctypes.byref(message), None, 0, 0) > 0:
                if message.message == 0x0312 and message.wParam in names:
                    self.events.put(names[message.wParam])
        for number in registered:
            user32.UnregisterHotKey(None, number)

    def Poll(self):
        out = []
        while not self.events.empty():
            out.append(self.events.get())
        return out

    def Close(self):
        if self.thread_id:
            import ctypes
            ctypes.WinDLL("user32").PostThreadMessageW(self.thread_id, 0x0012, 0, 0)
        self.thread.join(3)


_CONSOLE = {"started": False, "events": []}


class ConsoleKeys:
    """Fallback: lines typed in this console. Enter or 'a' = attempt, 'e' = end."""

    def __init__(self):
        _CONSOLE["events"].clear()
        if not _CONSOLE["started"]:
            _CONSOLE["started"] = True
            threading.Thread(target=self.Read, daemon=True).start()

    @staticmethod
    def Read():
        for line in sys.stdin:
            text = line.strip().lower()
            if text in ("", "a"):
                _CONSOLE["events"].append("attempt")
            elif text == "e":
                _CONSOLE["events"].append("end")

    def Poll(self):
        out, _CONSOLE["events"][:] = list(_CONSOLE["events"]), []
        return out

    def Close(self):
        _CONSOLE["events"].clear()


def OpenKeys(attempt, end, factory=None):
    """Hotkey source for a manual take: global hotkeys, or the console when they cannot be registered (said clearly)."""
    keys = {"attempt": ParseKey(attempt), "end": ParseKey(end)}
    try:
        if factory is not None:
            return factory(keys)
        if os.name != "nt":
            raise HotkeyError("global hotkeys need Windows")
        source = Win32Hotkeys(keys)
        print(f"hotkeys: {attempt.upper()} = new attempt, {end.upper()} = end the take (global, work with MCC focused)")
        return source
    except HotkeyError as error:
        print(f"ERROR hotkeys: {error}")
        print("  fallback: in this console press Enter (or a + Enter) = new attempt, e + Enter = end the take")
        return ConsoleKeys()


def HandleKey(marks, take_id, key, obs, out=print):
    """A key of a manual take: attempt (new marker at the video time) or end. Returns True when the take must end."""
    position = obs.Position()
    if key == "end":
        out(f"[{position['video_s']:8.1f} s] end of take {take_id}")
        return True
    number = marks.Attempt(take_id, position)
    previous = marks.Take(take_id)["attempts"]
    if len(previous) > 1:
        previous[-2]["end_s"] = position["video_s"]
    obs.Chapter(f"{take_id} attempt {number}")
    out(f"[{position['video_s']:8.1f} s] {take_id} attempt {number}")
    marks.Write(obs)
    return False


def StopProcess(proc, seconds=30):
    """Ends env_check as Ctrl+C would (it prints its summary): CTRL_BREAK on Windows (new process group), SIGINT elsewhere."""
    if proc.poll() is not None:
        return
    import signal
    try:
        proc.send_signal(signal.CTRL_BREAK_EVENT if os.name == "nt" else signal.SIGINT)
        proc.wait(seconds)
    except (subprocess.TimeoutExpired, OSError):
        proc.kill()
        proc.wait(10)


def LoadSummary(path):
    try:
        with open(path, encoding="utf-8") as handle:
            return json.load(handle)
    except (OSError, ValueError):
        return None


def WaitRunning(game, scenario_id, index, timeout, sleep=time.sleep, now=time.time):
    """(result, state) once the bench has match `index` (1-based: matchIndex of state.json) of our scenario Running. result: ok, timeout, missed
    (the bench is already past it), gone (another scenario replaced ours) or dead (no heartbeat for 60 s)."""
    start, stale, seen = now(), None, False
    while True:
        state = game.Bench()
        if state and state.get("_alive"):
            stale = None
            if state.get("scenario") == scenario_id:
                seen = True
                number = int(state.get("matchIndex") or 0)
                if number > index:
                    return "missed", state
                if number == index and state.get("phase") == "Running":
                    return "ok", state
            elif seen:
                return "gone", state
        else:
            stale = stale or now()
            if now() - stale > 60:
                return "dead", state
        if now() - start > timeout:
            return "timeout", state
        sleep(1)


def SkipTake(game, scenario_id, number):
    """Asks the bench to end match `number` of our scenario (game.Skip). return: whether the bench moved on; if not, the cap of the match ends it."""
    try:
        return game.Skip(number, scenario=scenario_id) == 0
    except (OSError, ValueError) as error:
        print(f"WARNING skip of take {number} failed: {error}")
        return False


def EnvCheckCommand(python, take, plan, summary_path, root=ROOT):
    return [python, os.path.join(root, "Agent", "scripts", "env_check.py"), "--policy", ResolvePolicy(take.red, plan, root),
            "--policy2", ResolvePolicy(take.blue, plan, root), "--episodes", "0" if take.is_manual else str(take.rounds),
            "--scale", f"{take.scale:g}", "--max-seconds", f"{take.max_seconds:g}", "--summary-json", summary_path]


def Validate(plan, takes, root=ROOT, director=None):
    """(errors, warnings) that need no OBS and no game: policies exist, sequences exist."""
    errors, warnings = [], []
    for take in takes:
        for who, value in (("red", take.red), ("blue", take.blue)):
            try:
                path = ResolvePolicy(value, plan, root)
            except ValueError as error:
                errors.append(f"take {take.id}: {error}")
                continue
            if value not in ("seek", "random") and not os.path.isfile(path):
                errors.append(f"take {take.id}: {who} policy {value}: {path} does not exist")
    more_errors, more_warnings = CheckSequences(takes, director)
    return errors + more_errors, warnings + more_warnings


def Table(takes):
    lines = [f"{'take':6} {'red':8} {'blue':8} {'scale':>5} {'rounds':>6} {'cap s':>6} {'est s':>6}  sequence"]
    for t in takes:
        rounds = "manual" if t.is_manual else str(t.rounds)
        lines.append(f"{t.id:6} {t.red:8} {t.blue:8} {t.scale:5g} {rounds:>6} {t.match_seconds:6.0f} {EstimateSeconds(t):6.0f}  {t.sequence}")
    total = sum(EstimateSeconds(t) for t in takes)
    lines.append(f"{len(takes)} takes, estimated up to {total / 60:.0f} min of wall time (rounds at their cap + ~30 s of loading per take; "
                 "each take is skipped when it ends)")
    return lines


def CompileScenario(game, scenario, state):
    configs = state.get("configs") if state and state.get("_alive") else None
    return game.Compile(scenario, configs)


def DryRun(plan, takes, scenario, game, settings, connect, root=ROOT, director=None):
    """The checks without playing or recording. Returns the number of errors."""
    errors, warnings = Validate(plan, takes, root, director)
    try:
        CompileScenario(game, scenario, game.Bench())
        print(f"scenario: compiles ({len(scenario['matches'])} matches)")
    except ValueError as error:
        errors.append(f"scenario: {error}")
    try:
        obs = connect(*settings)
        version = obs.Call("get_version")
        print(f"OBS {version.obs_version}, websocket {version.obs_web_socket_version}")
        print(f"recording folder: {obs.Call('get_record_directory').record_directory}")
        if obs.Status().output_active:
            errors.append("OBS is already recording: stop it first (record.py never records over a recording)")
    except ObsError as error:
        errors.append(str(error))
    watcher, bench = game.Watcher(), game.Bench()
    if not watcher:
        errors.append("the game watcher is not running (Tools\\game_watcher.bat)")
    else:
        print(f"watcher: ok | MCC {'running' if watcher.get('mccRunning') else 'NOT running'}")
    if not bench or not bench.get("_alive"):
        errors.append("the bench does not answer (no fresh state.json: is MCC open with Artemis?)")
    else:
        print(f"bench: phase {bench.get('phase')} | lobbyReady {bench.get('lobbyReady')} | {bench.get('message', '')}")
        if bench.get("phase") == "Running":
            errors.append("the bench is already running a scenario: wait for it to end")
    print("\n".join(Table(takes)))
    for line in warnings:
        print("WARNING", line)
    for line in errors:
        print("ERROR", line)
    print("dry run: " + ("FAIL" if errors else "ok"))
    return len(errors)


def Execute(args, plan, takes, scenario, game, obs, plan_path, keys_factory=None):
    """The run. Returns the exit code."""
    stamp = time.strftime("%Y-%m-%d_%H%M")
    folder = AbsPath(plan.output)
    os.makedirs(folder, exist_ok=True)
    marks = Marks(plan, plan_path, takes, folder, stamp, time.strftime("%Y-%m-%dT%H:%M:%S"))
    scenario_path = AbsPath(os.path.join("Ignore", "Temporal", f"rec_{plan.name}.json"))
    os.makedirs(os.path.dirname(scenario_path), exist_ok=True)
    with open(scenario_path, "w", encoding="utf-8") as handle:
        json.dump(scenario, handle, indent=1)
    queued = subprocess.run([sys.executable, os.path.join(TOOLS, "game.py"), "run", scenario_path, "--no-wait"], cwd=ROOT,
                            capture_output=True, text=True)
    print((queued.stdout + queued.stderr).strip())
    found = re.search(r"queued (\S+)", queued.stdout)
    if queued.returncode != 0 or not found:
        print("FAIL the scenario was not queued (is Tools\\game_watcher.bat open?).")
        return 1
    scenario_id = found.group(1)
    code, is_recording, proc, previous_cap, to_skip = 0, False, None, 0.0, None
    try:
        for number, take in enumerate(takes, 1):
            summary_path = os.path.join(folder, f"{plan.name}_{take.id}.json")
            log_path = os.path.join(folder, f"{plan.name}_{take.id}.txt")
            print(f"\n== take {take.id} ({number}/{len(takes)}) {take.red} vs {take.blue}, {take.sequence}")
            result, state = WaitRunning(game, scenario_id, number, args.start_timeout + previous_cap)
            if result != "ok":
                text = {"timeout": f"no match Running after {args.start_timeout + previous_cap:.0f} s", "missed": "the bench is past this match",
                        "gone": "another scenario replaced ours", "dead": "the bench stopped answering"}[result]
                print(f"ERROR take {take.id}: {text}: {(state or {}).get('phase')} | {(state or {}).get('message', '')}")
                marks.Finish(take.id, None, "error", text)
                marks.Write(obs)
                code = 1
                if result in ("gone", "dead"):
                    for rest in takes[number:]:
                        marks.Finish(rest.id, None, "error", "not run: " + text)
                    break
                previous_cap = take.match_seconds
                continue
            if not is_recording:
                obs.Start()
                is_recording = True
            else:
                obs.Resume()
            position = obs.Position()
            marks.Begin(take.id, position)
            obs.Chapter(f"{take.id} {take.note}".strip())
            print(f"[{position['video_s']:8.1f} s] recording take {take.id}: {state.get('message', '')}")
            command = EnvCheckCommand(sys.executable, take, plan, summary_path)
            flags = getattr(subprocess, "CREATE_NEW_PROCESS_GROUP", 0) if os.name == "nt" else 0
            keys, interrupted, status, error = None, False, "ok", None
            with open(log_path, "w", encoding="utf-8") as log:
                proc = subprocess.Popen(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, creationflags=flags)
                began, is_ended = time.time(), False
                try:
                    if take.is_manual:
                        keys = OpenKeys(args.key_attempt, args.key_end, keys_factory)
                    while proc.poll() is None:
                        for key in (keys.Poll() if keys else []):
                            if HandleKey(marks, take.id, key, obs):
                                is_ended = True
                        if is_ended:
                            StopProcess(proc)
                            break
                        if time.time() - began > take.match_seconds + 120:
                            error = f"env_check did not end in {take.match_seconds + 120:.0f} s: stopped"
                            StopProcess(proc)
                            break
                        time.sleep(0.25)
                except KeyboardInterrupt:
                    interrupted = True
                    print("\ninterrupted: stopping env_check (its summary is printed)")
                    StopProcess(proc)
                except ObsError:
                    StopProcess(proc)
                    raise
                finally:
                    if keys:
                        keys.Close()
            summary = LoadSummary(summary_path)
            if error is None and proc.returncode not in (0, None) and not (take.is_manual and is_ended and summary):
                error = f"env_check exit code {proc.returncode} (see {os.path.basename(log_path)})"
            if interrupted:
                status, error = "error", "interrupted"
            elif error:
                status = "error"
            to_skip = number
            end = obs.Position()
            marks.Finish(take.id, end, status, error, summary)
            if status == "error":
                code = 1
            print(f"[{end['video_s']:8.1f} s] take {take.id}: {status}{' - ' + error if error else ''}")
            if interrupted:
                for rest in takes[number:]:
                    marks.Finish(rest.id, None, "skipped", "interrupted")
                code = 130
                marks.Write(obs)
                break
            if number < len(takes):
                obs.Pause()
                marks.Write(obs)
                previous_cap = 0.0 if SkipTake(game, scenario_id, number) else take.match_seconds
                to_skip = None
            else:
                marks.Write(obs)
    except ObsError as error:
        print(f"FAIL {error}")
        for record in marks.data["takes"]:
            if record["status"] == "running":
                record["status"], record["error"] = "error", str(error)
        code = 1
    except KeyboardInterrupt:
        code = 130
    finally:
        if is_recording:
            try:
                path = obs.Stop()
                print(f"recording stopped: {path or '(see OBS)'}")
            except ObsError as error:
                print(f"ERROR could not stop the recording: {error}")
        marks.Write(obs)
        print(f"marks: {marks.base}_marcas.json | {marks.base}.edl")
        if to_skip is not None:
            # After StopRecord, so the video does not show the match being left.
            if not SkipTake(game, scenario_id, to_skip):
                print("WARNING the last match was not skipped: the bench keeps it (quiet bots) until its cap ends it.")
    return code


def Main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("plan", help="the plan, e.g. Ignore\\Content\\Episode 1\\grabaciones.json")
    parser.add_argument("--only", help="comma separated take ids (a disabled take runs if named)")
    parser.add_argument("--dry-run", action="store_true", help="check everything; play and record nothing")
    parser.add_argument("--start-timeout", type=float, default=300.0, help="seconds to wait for a match to be Running")
    parser.add_argument("--key-attempt", default=KEY_ATTEMPT, help="manual takes: new attempt (default F9)")
    parser.add_argument("--key-end", default=KEY_END, help="manual takes: end the take (default F10)")
    args = parser.parse_args(argv)
    try:
        ParseKey(args.key_attempt), ParseKey(args.key_end)
        if args.key_attempt.upper() == args.key_end.upper():
            raise ValueError("--key-attempt and --key-end must differ")
        plan_path = AbsPath(args.plan, os.getcwd()) if os.path.isfile(args.plan) else AbsPath(args.plan)
        plan = LoadPlan(plan_path)
        takes = Select(plan, args.only.split(",") if args.only else None)
        if not takes:
            raise ValueError("no take to run (all disabled? use --only)")
        scenario = BuildScenario(plan, takes)
        settings = ObsSettings()
    except ValueError as error:
        print(f"FAIL {error}")
        return 1
    import game
    director = DirectorFolder()
    if args.dry_run:
        return 1 if DryRun(plan, takes, scenario, game, settings, ConnectObs, ROOT, director) else 0
    errors, warnings = Validate(plan, takes, ROOT, director)
    for line in warnings:
        print("WARNING", line)
    try:
        CompileScenario(game, scenario, game.Bench())
    except ValueError as error:
        errors.append(f"scenario: {error}")
    bench = game.Bench()
    if not game.Watcher():
        errors.append("the game watcher is not running (Tools\\game_watcher.bat)")
    if bench and bench.get("phase") == "Running":
        errors.append("the bench is already running a scenario: wait for it to end (or restart MCC)")
    if errors:
        print("\n".join("FAIL " + e for e in errors))
        return 1
    try:
        obs = ConnectObs(*settings)
        if obs.Status().output_active:
            print("FAIL OBS is already recording: stop it first (record.py never records over a recording)")
            return 1
        print("\n".join(Table(takes)))
        return Execute(args, plan, takes, scenario, game, obs, plan_path)
    except ObsError as error:
        print(f"FAIL {error}")
        return 1


if __name__ == "__main__":
    sys.exit(Main())
