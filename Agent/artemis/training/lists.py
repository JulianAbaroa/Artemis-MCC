"""
lists.py
Training lists (LS.2, D282, D286): a file Data/Training/<name>.json with entries (map, preset, roster, rounds, weight), the order they are
played in ("sequence" or "weighted") and the compiled text of every preset they use. next_setup() gives the MatchSetup of the next round, for
CtfEnv.reset(setup=...).

    { "name": "...", "order": "sequence" | "weighted", "entries": [
        { "map": "Map CTF 1v1 First", "preset": "artemis_ctf_rounds", "roster": {"auto": 0, "rival": 1}, "rounds": 3, "weight": 1.0 } ],
      "presets": { "artemis_ctf_rounds": { "source": "<hash>", "lines": [ ... ] } } }

map is a config of Maps.json. preset is null (no change), "" (the base values) or the name of a preset of Data/Presets. roster is relative to the local player 0 (D287), a passive rival that
does not count in NvM: {"auto": bots on the team of the local player, "rival": bots on the other team}, or null (no change). The compiled presets are the ones of Tools/presets.py when the
list was saved: a list whose preset changed since says so when it is loaded (StaleListError) and has to be saved again.
"""
import importlib.util
import json
import os
import random
from dataclasses import dataclass, field

from ..match_setup import RELATIVE_ROSTER_KEYS, MatchSetup
from .catalog import ROOT, load_catalog

ORDERS = ("sequence", "weighted")
PLAYERS = 16
PRESETS_FOLDER = os.path.join(ROOT, "Data", "Presets")
_TOOL = os.path.join(ROOT, "Tools", "presets.py")


class ListError(ValueError):
    """The list is not valid."""


class StaleListError(ListError):
    """A preset of the list is not the one it was saved with."""


def _presets_tool():
    """Tools/presets.py as a library: the one compiler of the presets, loaded from the repo."""
    spec = importlib.util.spec_from_file_location("artemis_presets_tool", _TOOL)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def compile_presets(names, folder=None, catalog_path=None) -> dict:
    """{name: {"source": hash, "lines": [...]}} of the compiled presets `names`, from Tools/presets.py. Raises ListError when one does not
    compile or does not exist."""
    folder = folder or PRESETS_FOLDER
    tool = _presets_tool()
    if not names:
        return {}
    catalog = tool.Catalog(catalog_path or os.path.join(folder, "Catalog.xml"))
    compiled = {}
    for name in sorted(names):
        if not os.path.isfile(os.path.join(folder, name + ".xml")):
            raise ListError(f"preset {name!r} does not compile: there is no {name}.xml in {folder}")
        try:
            text = tool.CompilePreset(name, folder, catalog)[0]
        except (tool.PresetError, OSError) as error:
            raise ListError(f"preset {name!r} does not compile: {error}") from error
        lines = text.splitlines()
        source = next((line.split(" ", 1)[1] for line in lines if line.startswith("source ")), "")
        compiled[name] = {"source": source, "lines": lines}
    return compiled


@dataclass
class TrainingEntry:
    map: str
    preset: str | None = None
    roster: dict | None = None
    rounds: int = 1
    weight: float = 1.0


@dataclass
class TrainingList:
    """The entries, the order and the compiled presets. next_setup() walks them: "sequence" plays every entry for its `rounds` rounds in order
    and starts again; "weighted" draws an entry by its weight (with the seed) and plays it for its `rounds` rounds."""
    name: str
    order: str
    entries: list
    presets: dict = field(default_factory=dict)
    seed: int = 0

    def __post_init__(self):
        self._rng = random.Random(self.seed)
        self._index = -1
        self._left = 0

    def next_setup(self) -> MatchSetup:
        """The setup of the next round."""
        if self._left == 0:
            self._index = self._next_index()
            self._left = self.entries[self._index].rounds
        self._left -= 1
        entry = self.entries[self._index]
        return MatchSetup(entry.preset, entry.map, None if entry.roster is None else dict(entry.roster))

    def _next_index(self):
        if self.order == "weighted":
            return self._rng.choices(range(len(self.entries)), weights=[entry.weight for entry in self.entries])[0]
        return (self._index + 1) % len(self.entries)


def _check_entry(entry, index, maps, locals_):
    where = f"entry {index}"
    if entry.map not in maps:
        raise ListError(f"{where}: map {entry.map!r} is not in the catalog of maps")
    if entry.preset is not None and not isinstance(entry.preset, str):
        raise ListError(f"{where}: preset is null, a name or an empty text")
    if not isinstance(entry.rounds, int) or isinstance(entry.rounds, bool) or entry.rounds < 1:
        raise ListError(f"{where}: rounds {entry.rounds!r} must be an integer of at least 1")
    if not isinstance(entry.weight, (int, float)) or isinstance(entry.weight, bool) or not entry.weight > 0:
        raise ListError(f"{where}: weight {entry.weight!r} must be above 0")
    if entry.roster is not None:
        roster = entry.roster
        if not isinstance(roster, dict) or not set(roster) <= set(RELATIVE_ROSTER_KEYS):
            raise ListError(f"{where}: roster has the keys {list(RELATIVE_ROSTER_KEYS)} only (bots on the team of the local player and on the other team)")
        for key, count in roster.items():
            if not isinstance(count, int) or isinstance(count, bool) or count < 0:
                raise ListError(f"{where}: roster {key!r} must be an integer of at least 0")
        if roster.get("rival", 0) < 1:
            raise ListError(f"{where}: roster needs rival of at least 1 (the team of the local player always has the local player)")
        bots = roster.get("auto", 0) + roster["rival"]
        if bots + locals_ > PLAYERS:
            raise ListError(f"{where}: roster of {bots} bots and {locals_} local players does not fit in {PLAYERS} players")


def _build(data, maps, locals_, seed, path):
    if not isinstance(data, dict) or not isinstance(data.get("entries"), list) or not data["entries"]:
        raise ListError(f"{path}: expected an object with a non empty list of entries")
    if data.get("order") not in ORDERS:
        raise ListError(f"{path}: order {data.get('order')!r} is not one of {ORDERS}")
    entries = []
    for index, row in enumerate(data["entries"]):
        if not isinstance(row, dict) or not set(row) <= {"map", "preset", "roster", "rounds", "weight"} or "map" not in row:
            raise ListError(f"{path}: entry {index}: expected the keys map, preset, roster, rounds and weight (map is needed)")
        entries.append(TrainingEntry(row["map"], row.get("preset"), row.get("roster"), row.get("rounds", 1), row.get("weight", 1.0)))
    for index, entry in enumerate(entries):
        _check_entry(entry, index, maps, locals_)
    presets = data.get("presets", {})
    if not isinstance(presets, dict):
        raise ListError(f"{path}: presets is an object")
    return TrainingList(str(data.get("name", "")), data["order"], entries, presets, seed)


def load_list(path, seed=0, locals=1, maps_path=None, presets_folder=None, catalog_path=None) -> TrainingList:
    """Reads and validates a list: its maps are in the catalog, its rosters fit in 16 players with `locals` local players, rounds >= 1,
    weight > 0, a valid order. Every preset the entries use must be saved in the list and be the same as the compiled one now: if not, it
    raises StaleListError ("lista desactualizada: preset X") and the list has to be saved again. seed is the one of the weighted order."""
    with open(path, encoding="utf-8") as handle:
        data = json.load(handle)
    result = _build(data, load_catalog(maps_path), locals, seed, path)
    used = sorted({entry.preset for entry in result.entries if entry.preset})
    current = compile_presets(used, presets_folder, catalog_path)
    for name in used:
        saved = result.presets.get(name)
        if saved is None:
            raise StaleListError(f"lista desactualizada: preset {name} (the list does not hold it)")
        if saved.get("lines") != current[name]["lines"] or saved.get("source") != current[name]["source"]:
            raise StaleListError(f"lista desactualizada: preset {name} (it changed since the list was saved: save the list again)")
    return result


def save_list(path, name, order, entries, maps_path=None, presets_folder=None, catalog_path=None, locals=1):
    """Writes a list: validates it as load_list does and stores the compiled text of every preset the entries use. entries are dicts or
    TrainingEntry. Returns the TrainingList written."""
    rows = []
    for entry in entries:
        entry = entry if isinstance(entry, TrainingEntry) else TrainingEntry(**entry)
        rows.append({"map": entry.map, "preset": entry.preset, "roster": entry.roster, "rounds": entry.rounds, "weight": entry.weight})
    used = sorted({row["preset"] for row in rows if row["preset"]})
    data = {"name": name, "order": order, "entries": rows, "presets": compile_presets(used, presets_folder, catalog_path)}
    result = _build(data, load_catalog(maps_path), locals, 0, path)
    lines = ["{", f'  "name": {json.dumps(name)},', f'  "order": {json.dumps(order)},', '  "entries": [']
    lines += ["    " + json.dumps(row) + ("," if index < len(rows) - 1 else "") for index, row in enumerate(rows)]
    lines += ["  ],", '  "presets": {']
    for index, (preset, body) in enumerate(data["presets"].items()):
        lines.append(f"    {json.dumps(preset)}: {{")
        lines.append(f'      "source": {json.dumps(body["source"])},')
        lines.append('      "lines": [')
        lines += ["        " + json.dumps(line) + ("," if k < len(body["lines"]) - 1 else "") for k, line in enumerate(body["lines"])]
        lines.append("      ]")
        lines.append("    }" + ("," if index < len(data["presets"]) - 1 else ""))
    lines += ["  }", "}"]
    with open(path, "wb") as handle:
        handle.write("\r\n".join(lines).encode("ascii"))
    return result
