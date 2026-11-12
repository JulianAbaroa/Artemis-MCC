"""
match_setup.py
What a match gets in one step (LS.2): the preset, the map variant and the bots, the status Artemis answers a request with, and the roster as
the bytes of the bridge layout. Apart from bridge.py so the code that only describes a setup does not need the Windows shared memory.
"""
from dataclasses import dataclass
from typing import NamedTuple

import numpy as np

from . import bridge_layout as L
from .bridge_layout import BridgeSetupResult


class SetupStatus(NamedTuple):
    """What became of a setup request: the result, the round of the match it was applied in (-1 when none) and why (ASCII, empty when
    there is nothing to say). A Rejected result applied nothing. Applied or AtLoad with a reason "partial: ..." applied the preset and the
    map but not the whole roster."""
    result: BridgeSetupResult
    round: int
    reason: str


@dataclass(frozen=True)
class MatchSetup:
    """What a match gets in one step (LS.2): the preset, the map variant and the bots, as request_setup takes them. A field that is None
    is not asked for. Build it from a dict with the same keys with MatchSetup.of."""
    preset: str | None = None
    map: str | None = None
    roster: object = None

    @staticmethod
    def of(setup) -> "MatchSetup":
        """A MatchSetup from a MatchSetup or a dict with the keys preset, map and roster (None gives an empty one). Other keys are an error."""
        if setup is None:
            return MatchSetup()
        if isinstance(setup, MatchSetup):
            return setup
        unknown = set(setup) - {"preset", "map", "roster"}
        if unknown:
            raise ValueError(f"unknown keys of a match setup: {sorted(unknown)}")
        return MatchSetup(setup.get("preset"), setup.get("map"), setup.get("roster"))


def roster_to_array(roster) -> np.ndarray:
    """A roster as the bytes of the layout: the bots of each of the eight teams, then the automatic ones. `roster` is a dict with the team
    index (int or text) or "auto" as keys, as the "roster" of a bench scenario, or a sequence with the bots of the teams from team 0."""
    teams = np.zeros(L.MAX_TEAMS, dtype=np.uint8)
    auto = 0
    items = enumerate(roster) if isinstance(roster, (list, tuple)) else dict(roster).items()
    for key, count in items:
        count = int(count)
        if not 0 <= count <= 255:
            raise ValueError(f"bots out of range for {key!r}: {count}")
        if key == "auto":
            auto = count
            continue
        team = int(key)
        if not 0 <= team < L.MAX_TEAMS:
            raise ValueError(f"team out of range: {key!r}")
        teams[team] = count
    return np.concatenate([teams, np.array([auto], dtype=np.uint8)])


def roster_to_dict(teams, auto) -> dict:
    """The roster of a record as a dict: the teams with bots by index (text keys, as in a bench scenario) and "auto"."""
    result = {str(team): int(count) for team, count in enumerate(teams) if int(count) > 0}
    if int(auto) > 0:
        result["auto"] = int(auto)
    return result
