"""
catalog.py
The catalog of the training maps (D285): Data/Training/Maps.json, one match config per map, recorded by the user in the lobby with the
gametype Artemis CTF and that map. The name of the config is exact (capitals included): it is what Artemis looks up to read the map variant.
"""
import json
import os
import re
from dataclasses import dataclass

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
MAPS_PATH = os.path.join(ROOT, "Data", "Training", "Maps.json")
SIZE = re.compile(r"^[1-9]v[1-9]$")
STAGES = ("First", "Final")


@dataclass(frozen=True)
class MapInfo:
    """config: name of the match config (what a setup asks for). name: name to show. size: "NvN". stage: First (no weapons or vehicles)
    or Final (everything)."""
    config: str
    name: str
    size: str
    stage: str


def load_catalog(path=None) -> dict:
    """{config: MapInfo} of the catalog. Raises ValueError with what is wrong: a key missing, a size that is not NvN, a stage that is not
    First or Final, or a config or name that repeats."""
    path = path or MAPS_PATH
    with open(path, encoding="utf-8") as handle:
        rows = json.load(handle)
    if not isinstance(rows, list):
        raise ValueError(f"{path}: expected a list of maps")
    maps = {}
    names = set()
    for index, row in enumerate(rows):
        where = f"{path}: map {index}"
        if not isinstance(row, dict) or set(row) != {"config", "name", "size", "stage"}:
            raise ValueError(f"{where}: expected the keys config, name, size and stage")
        if not all(isinstance(value, str) and value for value in row.values()):
            raise ValueError(f"{where}: every value is a non empty text")
        if not SIZE.match(row["size"]):
            raise ValueError(f"{where}: size {row['size']!r} is not NvN")
        if row["stage"] not in STAGES:
            raise ValueError(f"{where}: stage {row['stage']!r} is not First or Final")
        if row["config"] in maps:
            raise ValueError(f"{where}: config {row['config']!r} repeats")
        if row["name"] in names:
            raise ValueError(f"{where}: name {row['name']!r} repeats")
        names.add(row["name"])
        maps[row["config"]] = MapInfo(**row)
    return maps
