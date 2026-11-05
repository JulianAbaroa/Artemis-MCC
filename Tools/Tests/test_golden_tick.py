"""
test_golden_tick.py
F2.A (D270): assertions on the golden.tdump of the nine films, field by field. ReplayTests.Recordings already proves that the
pipeline gives each golden byte for byte, so asserting the golden asserts the pipeline, and Python reads it without a compile.
The fields that ReplayTests asserts already (weapons, projectiles, explosives, lifts, five traits, powerups, ground, respawn, MapId, clock) are
listed in COVERED with the name of that test. test_every_tick_field_is_asserted keeps the F2 condition from now on: a field added to
Bridge.xml with no entry in COVERED or EXEMPT fails.

One pass per film (Survey, cached for the session) gathers everything the tests ask: the tests only assert on what it found. A failure message
lists the first 10 cases with film, GameTick and handle. A film without golden.tdump skips its tests. The measured values are written as constants
with the film and the tick they come from. A measurement that contradicts the film.txt, the physics or the contract is not asserted: the test is
xfail(strict) with the number of its HALLAZGO of the report.
"""
import collections
import enum
import functools
import math
import os
import re
import sys
import xml.etree.ElementTree as ElementTree

import numpy as np
import pytest

import tickdump
from tdump_writer import Repo

sys.path.insert(0, os.path.join(Repo(), "Agent"))
from artemis import bridge_layout as bl  # noqa: E402
from artemis import semantic as sem  # noqa: E402

FILMS = ("armas_a", "armas_b", "cgb_displace_1min", "ctf_c", "ctf_d", "explosivos", "lifts", "powerups", "suelo")
NO_HANDLE = bl.NO_HANDLE
NOT_LOCAL = bl.NOT_LOCAL
MAX_PLAYERS = bl.MAX_PLAYERS
TICK_RATE = 60.0

# Enums of the engine that Semantic.xml and Bridge.xml do not generate for Python (Common and Blam in C++).


class ObjectKind(enum.IntEnum):
    """Blam ObjectKind (_DefinitionsObject.ixx): the Kind of an entity."""
    Biped = 0
    Vehicle = 1
    Weapon = 2
    Equipment = 3
    Terminal = 4
    Projectile = 5
    Scenery = 6
    Machine = 7
    Control = 8
    SoundScenery = 9
    Crate = 10
    Creature = 11
    Giant = 12
    EffectScenery = 13


class Team(enum.IntEnum):
    """Common Team (Common.Team.Type.ixx)."""
    Red = 0
    Blue = 1
    Neutral = 8


class GrenadeType(enum.IntEnum):
    """Blam GrenadeType (ObjectBiped.ixx)."""
    Frag = 0
    Plasma = 1


class ZoomLevel(enum.IntEnum):
    """Blam ZoomLevel (ObjectBiped.ixx): None is 0xFF."""
    Zoom1 = 0
    Zoom2 = 1
    NONE = 0xFF


class Connection(enum.IntEnum):
    """Values of Players.Connection measured in the goldens. 1 is the Connected of Blam ConnectionState. 8 and 9 (8 and 1) are the 601
    ticks before the first spawn (suelo 1..601, ctf_c 1..604), 17 is 16 and 1 (the player of the engine in the Connected state with the bit 0x10).
    10 (8 and 2) appears only in cgb_displace_1min, the players that join or leave the match during the film."""
    Connected = 1
    Connecting = 8
    ConnectingConnected = 9
    ConnectedBit16 = 17
    ConnectingDisconnected = 10


class AffectsBit(enum.IntFlag):
    """Bits of LiftStage.Affects (Bridge.xml)."""
    Players = 1
    Bipeds = 2
    Vehicles = 4
    Items = 8


FLAG_KINDS = (sem.Event.FlagTaken, sem.Event.FlagDropped, sem.Event.FlagReturned, sem.Event.FlagReset, sem.Event.FlagCaptured)
# Loadout handles of a Player row, in the order of the lookups of the survey (column of L).
LOOKUPS = ("BipedHandle", "PrimaryWeapon", "SecondaryWeapon", "EquippedWeapon", "Ability", "Objective")
BODY_HANDLES = ("PrimaryWeapon", "SecondaryWeapon", "EquippedWeapon", "Ability", "Objective", "LastDamagerPlayer", "GroundObject")
CAPABILITY_MASK = functools.reduce(lambda a, b: a | b, [int(c) for c in sem.Capability])


def RecordingsFolder():
    """ARTEMIS_RECORDINGS if it is set, else Ignore/Regresion (the rule of Tests.Support.FilmPass)."""
    return os.environ.get("ARTEMIS_RECORDINGS") or os.path.join(Repo(), "Ignore", "Regresion")


def Vec(rows):
    """float64 array (..., 3) of a BridgeVec3 field."""
    return np.stack([rows["X"], rows["Y"], rows["Z"]], axis=-1).astype(np.float64)


class Findings:
    """What the pass of one film found. Counts and the first cases of each rule, plus the measured values the tests compare."""

    def __init__(self, film):
        self.film = film
        self.counts = collections.Counter()
        self.cases = collections.defaultdict(list)
        self.values = {}

    def Add(self, name, tick, handle, detail=""):
        self.counts[name] += 1
        if len(self.cases[name]) < 10:
            self.cases[name].append((int(tick), int(handle), detail))

    def Flag(self, name, mask, ticks, handles, detail=None):
        """Counts the True of mask and keeps the first cases. ticks and handles are aligned with mask, detail too (an array or None)."""
        mask = np.asarray(mask, dtype=bool)
        total = int(mask.sum())
        if total == 0:
            return
        self.counts[name] += total
        room = 10 - len(self.cases[name])
        for index in np.flatnonzero(mask)[:max(room, 0)]:
            extra = "" if detail is None else str(detail[index])
            self.cases[name].append((int(ticks[index]), int(handles[index]), extra))

    def Report(self, name):
        lines = [f"{self.film}: {self.counts[name]} case(s) of {name}, the first ones (GameTick, handle, detail):"]
        lines += [f"  tick {t} handle 0x{h:08X} {d}" for t, h, d in self.cases[name]]
        return "\n".join(lines)

    def Clean(self, name):
        assert self.counts[name] == 0, self.Report(name)

    def Exactly(self, name, expected):
        assert self.counts[name] == expected, f"expected {expected}. " + self.Report(name)


# ---------------------------------------------------------------------------------------------------------------------------------
# The pass of one film
# ---------------------------------------------------------------------------------------------------------------------------------

def Lookup(tab, handles):
    """(found, rows) of the handles in a table sorted by handle. rows of a missing handle are the row at the insertion point."""
    handles = np.ascontiguousarray(handles)
    count = len(tab)
    if count == 0:
        return np.zeros(len(handles), bool), np.zeros(len(handles), tab.dtype)
    index = np.minimum(np.searchsorted(tab["Handle"], handles), count - 1)
    return tab["Handle"][index] == handles, tab[index]


def Concat(chunks, dtype):
    return np.concatenate(chunks) if chunks else np.zeros(0, dtype)


@functools.lru_cache(maxsize=None)
def Survey(film):
    """Findings of the film, or None when it has no golden.tdump."""
    path = os.path.join(RecordingsFolder(), film, "golden.tdump")
    if not os.path.isfile(path):
        return None
    return Collect(film, tickdump.Dump(path))


def Collect(film, dump):
    s = Findings(film)
    records = dump.records
    s.ticks = np.array([r.game_tick for r in records], dtype=np.int64)
    s.record_count = len(records)
    types = dump.types
    entity_chunks, entity_record, entity_parent = [], [], []
    player_chunks, player_record, found_chunks, lookup_chunks = [], [], [], []
    event_chunks, event_record = [], []
    objective_chunks, objective_record, stand_chunks, stand_record = [], [], [], []
    lift_chunks, lift_record, lift_found, lift_origin = [], [], [], []
    last_sequence = 0
    previous_handles = np.zeros(0, np.uint32)
    previous_players = np.zeros(0, np.uint32)
    for i, (r, tab) in enumerate(dump.states()):
        tick = r.game_tick
        handles = tab["Handle"]
        # A table sorted by handle without repeats is what every lookup of the pass relies on.
        if len(handles) > 1 and not np.all(handles[1:] > handles[:-1]):
            s.Add("table_not_sorted_or_repeated", tick, handles[0])
        header = r.header
        for name, size in (("PlayerCount", len(r.players)), ("EntityCount", len(tab)), ("ObjectiveCount", len(r.objectives)),
                           ("StandCount", len(r.stands)), ("WeaponCount", len(r.weapons)), ("ProjectileCount", len(r.projectiles)),
                           ("ExplosiveCount", len(r.explosives)), ("LiftCount", len(r.lifts))):
            if int(header[name]) != size:
                s.Add("header_count_" + name, tick, 0, f"header {int(header[name])} rows {size}")
        if int(header["Flags"]) != 0:
            s.Add("header_flags", tick, 0, f"0x{int(header['Flags']):X}")
        # Entities: the rows that changed, with the kind of their parent in the table of the tick.
        e = r.entities
        if len(e):
            entity_chunks.append(e)
            entity_record.append(np.full(len(e), i, np.int32))
            parent = np.full(len(e), -1, np.int16)
            linked = e["BoundTo"] != NO_HANDLE
            if linked.any():
                found, rows = Lookup(tab, e["BoundTo"][linked])
                parent[linked] = np.where(found, rows["Kind"], -1)
            entity_parent.append(parent)
            if len(np.unique(e["Handle"])) != len(e):
                s.Add("entity_handle_repeated_in_a_record", tick, e["Handle"][0])
        if len(r.removed) and len(tab):
            linked = tab["BoundTo"] != NO_HANDLE
            found, _ = Lookup(tab, tab["BoundTo"][linked])
            for h in tab["Handle"][linked][~found][:3]:
                s.Add("link_to_a_removed_entity", tick, h)
        # Players and the entities they point to.
        p = r.players
        if len(p):
            player_chunks.append(p)
            player_record.append(np.full(len(p), i, np.int32))
            wanted = np.stack([p[n] for n in LOOKUPS], axis=1)
            found, rows = Lookup(tab, wanted.ravel())
            found_chunks.append(found.reshape(len(p), len(LOOKUPS)))
            lookup_chunks.append(rows.reshape(len(p), len(LOOKUPS)))
        current_players = p["Handle"] if len(p) else np.zeros(0, np.uint32)
        # Events: the objects and players they name exist in this tick or in the previous one.
        ev = r.events
        if len(ev):
            event_chunks.append(ev)
            event_record.append(np.full(len(ev), i, np.int32))
            for name in ("CauseObject", "EffectObject"):
                v = ev[name]
                missing = (v != NO_HANDLE) & ~(np.isin(v, handles) | np.isin(v, previous_handles))
                for index in np.flatnonzero(missing)[:3]:
                    kind = int(ev["Kind"][index])
                    gone = kind in (int(sem.Event.FlagReturned), int(sem.Event.FlagReset))
                    s.Add("event_flag_object_gone" if gone else "event_object_unknown_" + name, tick, v[index], f"kind {kind}")
            for name in ("CausePlayer", "EffectPlayer"):
                v = ev[name][ev[name] != NO_HANDLE]
                missing = ~(np.isin(v, current_players) | np.isin(v, previous_players))
                for h in v[missing][:3]:
                    s.Add("event_player_unknown_" + name, tick, h)
            last_sequence = max(last_sequence, int(ev["Sequence"].max()))
        if int(header["LastEventSequence"]) != last_sequence:
            s.Add("header_last_event_sequence", tick, 0, f"header {int(header['LastEventSequence'])} events {last_sequence}")
        # Flags and stands.
        if len(r.objectives):
            objective_chunks.append(r.objectives)
            objective_record.append(np.full(len(r.objectives), i, np.int32))
            CheckFlags(s, r, tab, tick)
        if len(r.stands):
            stand_chunks.append(r.stands)
            stand_record.append(np.full(len(r.stands), i, np.int32))
        # Lifts: the origin of the entity of each one.
        if len(r.lifts):
            found, rows = Lookup(tab, r.lifts["Handle"])
            lift_chunks.append(r.lifts)
            lift_record.append(np.full(len(r.lifts), i, np.int32))
            lift_found.append(found)
            lift_origin.append(rows["WorldOrigin"])
        previous_handles, previous_players = handles, current_players
    s.matches = np.array([r.match for r in records], dtype=types["BridgeMatch"])
    s.P = Concat(player_chunks, types["BridgePlayer"])
    s.P_record = Concat(player_record, np.int32)
    s.P_found = Concat(found_chunks, bool).reshape(-1, len(LOOKUPS))
    s.L = Concat(lookup_chunks, types["BridgeEntity"]).reshape(-1, len(LOOKUPS))
    s.E = Concat(entity_chunks, types["BridgeEntity"])
    s.E_record = Concat(entity_record, np.int32)
    s.E_parent = Concat(entity_parent, np.int16)
    s.EV = Concat(event_chunks, types["BridgeEvent"])
    s.EV_record = Concat(event_record, np.int32)
    s.O = Concat(objective_chunks, types["BridgeObjective"])
    s.O_record = Concat(objective_record, np.int32)
    s.S = Concat(stand_chunks, types["BridgeStand"])
    s.S_record = Concat(stand_record, np.int32)
    s.LI = Concat(lift_chunks, types["BridgeLift"])
    s.LI_record = Concat(lift_record, np.int32)
    s.LI_found = Concat(lift_found, bool)
    s.LI_origin = Concat(lift_origin, types["BridgeVec3"])
    AnalyzeEntities(s)
    AnalyzePlayers(s)
    AnalyzeMatch(s)
    AnalyzeEvents(s)
    AnalyzeObjectives(s)
    AnalyzeLifts(s)
    return s


def CheckFlags(s, r, tab, tick):
    """Per record rules of the flags: state and carrier, biped of the carrier, stand of the flag and the Objective of the carrier."""
    p = r.players
    biped_of = dict(zip(p["Handle"].tolist(), p["BipedHandle"].tolist()))
    objective_of = dict(zip(p["Handle"].tolist(), p["Objective"].tolist()))
    # An Objective of 0 is a handle that is not valid (HALLAZGO 2): counted apart in AnalyzePlayers.
    stands = {int(h): (int(b), int(o)) for h, b, o in zip(r.stands["Handle"], r.stands["BaseTeam"], r.stands["OwnerTeam"])}
    carried_by = {}
    for o in r.objectives:
        flag, carrier, state = int(o["Handle"]), int(o["CarrierPlayer"]), int(o["State"])
        if (state == sem.FlagState.Carried) != (carrier != NO_HANDLE):
            s.Add("flag_state_vs_carrier", tick, flag, f"state {state} carrier 0x{carrier:08X}")
        if carrier != NO_HANDLE:
            carried_by[carrier] = flag
            if int(o["CarrierBiped"]) != biped_of.get(carrier, -1):
                s.Add("flag_carrier_biped", tick, flag, f"CarrierBiped 0x{int(o['CarrierBiped']):08X}")
            if objective_of.get(carrier, -1) != flag:
                s.Add("flag_carrier_objective", tick, flag, f"Objective of the carrier 0x{objective_of.get(carrier, 0):08X}")
        elif int(o["CarrierBiped"]) != NO_HANDLE:
            s.Add("flag_carrier_biped", tick, flag, "CarrierBiped without carrier")
        home = stands.get(int(o["HomeStand"]))
        if home is None:
            s.Add("flag_home_stand_missing", tick, flag, f"HomeStand 0x{int(o['HomeStand']):08X}")
        else:
            if home[0] != int(o["BaseTeam"]):
                s.Add("flag_home_stand_base_team", tick, flag, f"stand {home[0]} flag {int(o['BaseTeam'])}")
            if home[1] != int(o["OwnerTeam"]):
                s.Add("flag_owner_team_vs_stand", tick, flag, f"stand {home[1]} flag {int(o['OwnerTeam'])}")
    for player, objective in objective_of.items():
        if objective not in (NO_HANDLE, 0) and carried_by.get(player) != objective:
            s.Add("player_objective_not_carried", tick, player, f"Objective 0x{objective:08X}")


# ---------------------------------------------------------------------------------------------------------------------------------
# Analysis of what the pass gathered
# ---------------------------------------------------------------------------------------------------------------------------------

# Frames: |Forward|, |Up| and Forward.Up measured at most 3.2e-7 in 9 films (float32 rounding), every kind, no zero vector.
ORTHO_TOLERANCE = 1e-5
# Entities with no bounding radius in the tags: weapons (Function Weapon, Kind Weapon) and projectiles.
ZERO_RADIUS_KINDS = (ObjectKind.Weapon, ObjectKind.Projectile)
# |BoundingCenter - WorldOrigin| - BoundingRadius of an entity that is not bound to a parent. Measured at most 3.40 (cgb, a vehicle), 0.05 in most films.
BOUNDS_MARGIN = 3.5
# Lowest Vitality of a dead entity: -22.22 in suelo (death by a long fall, 9441), a fraction can go below 0 when the damage is larger.
VITALITY_FLOOR = -23.0
# Aim and CameraForward are unit vectors with float32 rounding (measured 1.0000000908 at most).
UNIT_TOLERANCE = 1e-5
# Aim equals CameraForward in every row with Body (measured at most 1.2e-6 degrees).
AIM_ANGLE_TOLERANCE_DEGREES = 1e-4


# Shield: 1.0 full, 4.0 with the red powerup (ShieldMultiplier 4, powerups film). Measured at most 4.0.
SHIELD_MAX = 4.0
# Grenades of each type: 2 at the start of the variants of the films (measured at most 2).
GRENADES_MAX = 2
# Distance of the camera of a local player to the origin of its biped: 0.66 on foot, 3.09 in the Warthog (suelo 8032-8248).
CAMERA_DISTANCE_MAX = 3.5
# Distance of a stage of a lift to the origin of its entity: 3.947 at most (the gravity lifts of the Forge palette with a long second stage).
LIFT_STAGE_DISTANCE_MAX = 4.0
# Ticks between a Death or Spawn event and the change of HasBody of its player. Measured 1 at most.
EVENT_WINDOW_TICKS = 3
FIRST_TICKS_ALIVE = 1
HIT_WINDOW_TICKS = 3


# Traits rows of the goldens (Players.Traits, 31 fields). The main row of a film is the one of the players with no flag and no powerup. TRAITS_FILM says which one
# each film has, and the other rows it shows: (ticks of player rows, the fields that differ from the main row), by group. The group "base" has no flag and no powerup,
# "flag" carries an objective, the others a custom powerup. Measured in the golden of each film (the counts are player rows).
TRAITS_MAIN = {
    "MAIN_0": {
        "DamageResistance": 1.0, "HealthMultiplier": 1.0, "HealthRegenRate": 1.0, "ShieldMultiplier": 1.0, "ShieldRegenRate": 1.0,
        "OvershieldRegenRate": 1.0, "ShieldVampirism": 0.0, "DamageMultiplier": 1.0, "MeleeMultiplier": 1.0,
        "MovementSpeed": 1.100000023841858, "Gravity": 0.75, "JumpHeight": 1.0, "RadarRange": 25.0, "HeadshotImmune": 0,
        "AssassinationImmune": 0, "CannotDieFromDamage": 0, "GrenadeRegen": 0, "WeaponPickup": 1, "AbilitiesDropOnDeath": 0,
        "InfiniteAbilities": 1, "InfiniteAmmo": 2, "AbilityUsage": 2, "VehicleUse": 8, "VisibleWaypoint": 2, "VisibleName": 3,
        "RadarMode": 3, "FragGrenades": 2, "PlasmaGrenades": 2, "ActiveCamo": 1, "PrimaryWeapon": 0, "SecondaryWeapon": 1, "Ability": 0,
    },
    "MAIN_1": {
        "DamageResistance": 1.0, "HealthMultiplier": 1.0, "HealthRegenRate": 1.0, "ShieldMultiplier": 1.0, "ShieldRegenRate": 1.0,
        "OvershieldRegenRate": 1.0, "ShieldVampirism": 0.0, "DamageMultiplier": 1.0, "MeleeMultiplier": 1.0,
        "MovementSpeed": 1.100000023841858, "Gravity": 0.75, "JumpHeight": 1.0, "RadarRange": 25.0, "HeadshotImmune": 0,
        "AssassinationImmune": 0, "CannotDieFromDamage": 0, "GrenadeRegen": 0, "WeaponPickup": 1, "AbilitiesDropOnDeath": 0,
        "InfiniteAbilities": 1, "InfiniteAmmo": 1, "AbilityUsage": 2, "VehicleUse": 8, "VisibleWaypoint": 2, "VisibleName": 3,
        "RadarMode": 3, "FragGrenades": 2, "PlasmaGrenades": 2, "ActiveCamo": 1, "PrimaryWeapon": 0, "SecondaryWeapon": 1, "Ability": 0,
    },
    "MAIN_2": {
        "DamageResistance": 1.0, "HealthMultiplier": 1.0, "HealthRegenRate": math.nan, "ShieldMultiplier": 1.0, "ShieldRegenRate": 1.0,
        "OvershieldRegenRate": 1.0, "ShieldVampirism": 0.0, "DamageMultiplier": 1.0, "MeleeMultiplier": 1.0,
        "MovementSpeed": 1.100000023841858, "Gravity": 0.75, "JumpHeight": 1.0, "RadarRange": 25.0, "HeadshotImmune": 0,
        "AssassinationImmune": 0, "CannotDieFromDamage": 0, "GrenadeRegen": 0, "WeaponPickup": 1, "AbilitiesDropOnDeath": 0,
        "InfiniteAbilities": 1, "InfiniteAmmo": 2, "AbilityUsage": 2, "VehicleUse": 8, "VisibleWaypoint": 2, "VisibleName": 3,
        "RadarMode": 3, "FragGrenades": 2, "PlasmaGrenades": 2, "ActiveCamo": 1, "PrimaryWeapon": 0, "SecondaryWeapon": 1, "Ability": 0,
    },
}
# What the traits of the carrier of the flag change (ctf_c, ctf_d and cgb): jump 1.5 and RadarRange 75 with the flag, 25 without it (ctf_c tick 11201).
TRAITS_CARRIER = {"ShieldVampirism": 1.0, "MeleeMultiplier": 1.25, "MovementSpeed": 0.8999999761581421, "JumpHeight": 1.5, "RadarRange": 75.0, "InfiniteAbilities": 0, "AbilityUsage": 1, "VehicleUse": 6, "VisibleWaypoint": 3}
TRAITS_FILM = {
    "armas_a": ("MAIN_0", {
        "base": [(34208, {}), (1080, {"RadarRange": 50.0})],
    }),
    "armas_b": ("MAIN_1", {
        "base": [(7840, {})],
    }),
    "cgb_displace_1min": ("MAIN_2", {
        "base": [(48915, {}), (2039, {"RadarRange": 75.0}), (1860, {"MovementSpeed": 1.0, "Gravity": 1.0, "InfiniteAbilities": 0, "InfiniteAmmo": 1, "FragGrenades": 255, "PlasmaGrenades": 255, "PrimaryWeapon": -2, "SecondaryWeapon": -2, "Ability": -2}), (302, {"MovementSpeed": 1.0, "Gravity": 1.0, "RadarRange": 75.0, "InfiniteAbilities": 0, "InfiniteAmmo": 1, "FragGrenades": 255, "PlasmaGrenades": 255, "PrimaryWeapon": -2, "SecondaryWeapon": -2, "Ability": -2}), (270, {"ActiveCamo": 3}), (164, {"RadarRange": 75.0, "ActiveCamo": 3}), (22, TRAITS_CARRIER)],
        "flag": [(448, TRAITS_CARRIER), (25, {})],
    }),
    "ctf_c": ("MAIN_0", {
        "base": [(76987, {}), (1213, TRAITS_CARRIER), (180, {"RadarRange": 50.0})],
        "flag": [(9552, TRAITS_CARRIER), (71, {})],
    }),
    "ctf_d": ("MAIN_0", {
        "base": [(22903, {}), (810, TRAITS_CARRIER)],
        "flag": [(2617, TRAITS_CARRIER), (30, {})],
    }),
    "explosivos": ("MAIN_0", {
        "base": [(5813, {})],
    }),
    "lifts": ("MAIN_0", {
        "base": [(10310, {}), (360, {"RadarRange": 50.0})],
    }),
    "powerups": ("MAIN_0", {
        "base": [(5008, {}), (180, {"RadarRange": 50.0}), (1, {"ActiveCamo": 5})],
        "red": [(1198, {"DamageResistance": math.inf, "ShieldMultiplier": 4.0}), (2, {})],
        "blue": [(505, {"ActiveCamo": 5}), (2, {})],
        "yellow": [(1199, {"DamageResistance": math.inf}), (1, {"ActiveCamo": 5})],
    }),
    "suelo": ("MAIN_0", {
        "base": [(9335, {}), (407, {"RadarRange": 50.0})],
    }),
}


def TraitsKey(row):
    """Tuple of the 31 fields of a Traits row, NaN written as "nan" so that two rows compare equal."""
    return tuple("nan" if isinstance(v, float) and math.isnan(v) else v for v in (float(row[n]) if row[n].dtype.kind == "f" else int(row[n]) for n in row.dtype.names))


def AnalyzeEntities(s):
    E, ticks, h = s.E, s.ticks[s.E_record], s.E["Handle"]
    s.values["entity_rows"] = len(E)
    for name, enum_type in (("Kind", ObjectKind), ("Function", sem.Function), ("Group", sem.Group), ("Lifecycle", sem.Lifecycle),
                            ("Binding", sem.Binding), ("Physics", sem.Physics)):
        s.Flag("entity_unknown_" + name, ~np.isin(E[name], [int(v) for v in enum_type]), ticks, h, E[name])
        s.values["entity_" + name + "_seen"] = sorted(int(v) for v in np.unique(E[name]))
    s.Flag("entity_capability_bit_unknown", (E["Capabilities"].astype(np.int64) & ~CAPABILITY_MASK) != 0, ticks, h, E["Capabilities"])
    # Frames.
    forward, up = Vec(E["Forward"]), Vec(E["Up"])
    forward_norm, up_norm = np.linalg.norm(forward, axis=1), np.linalg.norm(up, axis=1)
    dot = np.abs((forward * up).sum(axis=1))
    s.values["frame_deviation"] = (float(np.abs(forward_norm - 1).max()), float(np.abs(up_norm - 1).max()), float(dot.max())) if len(E) else (0, 0, 0)
    s.Flag("entity_forward_not_unit", ~(np.abs(forward_norm - 1) <= ORTHO_TOLERANCE), ticks, h, forward_norm)
    s.Flag("entity_up_not_unit", ~(np.abs(up_norm - 1) <= ORTHO_TOLERANCE), ticks, h, up_norm)
    s.Flag("entity_forward_not_perpendicular_to_up", ~(dot <= ORTHO_TOLERANCE), ticks, h, dot)
    # Links.
    binding, bound, parent = E["Binding"], E["BoundTo"] != NO_HANDLE, s.E_parent
    s.Flag("entity_binding_vs_bound_to", (binding == sem.Binding.Free) == bound, ticks, h, binding)
    s.Flag("entity_bound_to_vs_parent_handle", E["BoundTo"] != E["ParentHandle"], ticks, h)
    s.Flag("entity_bound_to_not_in_the_tick", bound & (parent < 0), ticks, h)
    item = np.isin(E["Kind"], [ObjectKind.Weapon, ObjectKind.Equipment])
    held = item & (parent == ObjectKind.Biped)
    seat = (E["Kind"] == ObjectKind.Biped) & (parent == ObjectKind.Vehicle)
    s.Flag("entity_carried_is_an_item_of_a_biped", (binding == sem.Binding.Carried) != held, ticks, h, binding)
    s.Flag("entity_seated_is_a_biped_of_a_vehicle", (binding == sem.Binding.Seated) != seat, ticks, h, binding)
    s.Flag("entity_attached_is_the_rest", (binding == sem.Binding.Attached) != (bound & ~held & ~seat), ticks, h, binding)
    # Bounds.
    radius = E["BoundingRadius"].astype(np.float64)
    s.Flag("entity_radius_not_finite", ~np.isfinite(radius), ticks, h, radius)
    s.Flag("entity_radius_not_positive", (radius <= 0) & ~np.isin(E["Kind"], ZERO_RADIUS_KINDS), ticks, h, radius)
    s.values["zero_radius_rows"] = int(((radius <= 0) & np.isin(E["Kind"], ZERO_RADIUS_KINDS)).sum())
    overshoot = np.linalg.norm(Vec(E["BoundingCenter"]) - Vec(E["WorldOrigin"]), axis=1) - radius
    free = (binding == sem.Binding.Free) & (radius > 0)
    s.values["bounds_overshoot_free"] = float(overshoot[free].max()) if free.any() else 0.0
    s.Flag("entity_center_outside_the_radius", free & (overshoot > BOUNDS_MARGIN), ticks, h, overshoot)
    # Vitality.
    vitality = E["Vitality"].astype(np.float64)
    known = ~np.isnan(vitality)
    active = E["Lifecycle"] == sem.Lifecycle.Active
    s.values["vitality_range"] = (float(vitality[known].min()), float(vitality[known].max())) if known.any() else (0, 0)
    s.Flag("entity_vitality_not_finite", np.isinf(vitality), ticks, h, vitality)
    s.Flag("entity_active_vitality_not_a_fraction", known & active & ~((vitality > 0) & (vitality <= 1)), ticks, h, vitality)
    s.Flag("entity_dead_vitality_above_zero", known & ~active & (vitality > 0), ticks, h, vitality)
    s.Flag("entity_vitality_below_the_floor", known & (vitality < VITALITY_FLOOR), ticks, h, vitality)
    # Velocities.
    linear, angular = Vec(E["LinearVelocity"]), Vec(E["AngularVelocity"])
    s.Flag("entity_linear_velocity_not_finite", ~np.isfinite(linear).all(axis=1), ticks, h)
    s.Flag("entity_angular_velocity_not_finite", ~np.isfinite(angular).all(axis=1), ticks, h)
    spinning = np.linalg.norm(angular, axis=1) > 0
    s.values["angular_rows"] = int(spinning.sum())
    s.values["angular_functions"] = dict(collections.Counter(int(f) for f in E["Function"][spinning]))
    s.values["linear_rows"] = int((np.linalg.norm(linear, axis=1) > 0).sum())


def PlayerPairs(s):
    """(a, b) indices of the rows of the same player in consecutive records, in the order of the player."""
    P, record = s.P, s.P_record
    order = np.lexsort((record, P["Handle"]))
    a, b = order[:-1], order[1:]
    adjacent = (P["Handle"][a] == P["Handle"][b]) & (record[b] == record[a] + 1)
    return a[adjacent], b[adjacent]


def AnalyzePlayers(s):
    P, L, F = s.P, s.L, s.P_found
    record, ticks, h = s.P_record, s.ticks[s.P_record], s.P["Handle"]
    body = P["HasBody"] == 1
    s.values["player_rows"] = len(P)
    s.values["body_rows"] = int(body.sum())
    s.Flag("player_alive_vs_body", (P["IsAlive"] == 1) != body, ticks, h)
    # The biped.
    biped, biped_found = L[:, 0], F[:, 0]
    s.Flag("player_body_biped_not_alive", body & ~(biped_found & (biped["Kind"] == ObjectKind.Biped) & (biped["Lifecycle"] == sem.Lifecycle.Active)), ticks, h)
    s.Flag("player_no_body_biped_found_but_alive", ~body & biped_found & (biped["Lifecycle"] != sem.Lifecycle.Dead), ticks, h)
    s.values["no_body_biped"] = {"no_handle": int((~body & (P["BipedHandle"] == NO_HANDLE)).sum()), "dead": int((~body & biped_found).sum()),
                                 "gone": int((~body & ~biped_found & (P["BipedHandle"] != NO_HANDLE)).sum())}
    s.Flag("player_biped_team", biped_found & (biped["Team"] != P["Team"]), ticks, h, biped["Team"])
    s.Flag("player_alive_biped_vitality", body & biped_found & ~(biped["Vitality"] > 0), ticks, h, biped["Vitality"])
    # Without Body every handle of the body is NoHandle and the numbers are zero (TicksSinceDamage is NoHandle).
    for name in BODY_HANDLES + ("TicksSinceDamage",):
        s.Flag("player_no_body_" + name, ~body & (P[name] != NO_HANDLE), ticks, h, P[name])
    for name in ("Zoom", "SelectedGrenade", "FragGrenades", "PlasmaGrenades", "IsAbilityActive", "TicksAlive", "AbilityEnergy", "Shield"):
        s.Flag("player_no_body_" + name + "_not_zero", ~body & (P[name] != 0), ticks, h, P[name])
    s.Flag("player_no_body_aim_not_zero", ~body & (np.abs(Vec(P["Aim"])).sum(axis=1) != 0), ticks, h)
    # TicksAlive.
    a, b = PlayerPairs(s)
    continuing = body[a] & body[b] & (P["BipedHandle"][a] == P["BipedHandle"][b])
    step = P["TicksAlive"][b].astype(np.int64) - P["TicksAlive"][a].astype(np.int64)
    one_tick = ticks[b] == ticks[a] + 1
    s.Flag("player_ticks_alive_step", continuing & one_tick & (step != 1), ticks[b], h[b], step)
    s.Flag("player_ticks_alive_stands_still", continuing & ~one_tick & (step != 0), ticks[b], h[b], step)
    follows = np.zeros(len(P), bool)
    follows[b[continuing]] = True
    first = body & ~follows
    s.values["first_ticks_alive"] = dict(collections.Counter(int(v) for v in P["TicksAlive"][first]))
    odd = first & (P["TicksAlive"] != FIRST_TICKS_ALIVE)
    s.values["first_ticks_alive_odd"] = [(int(t), int(v)) for t, v in zip(ticks[odd], P["TicksAlive"][odd])]
    s.values["ticks_alive_steps"] = [(int(t), int(v)) for t, v in zip(ticks[b][continuing & one_tick & (step != 1)], step[continuing & one_tick & (step != 1)])]
    # Ranges.
    shield = P["Shield"].astype(np.float64)
    s.values["shield_range"] = (float(np.nanmin(shield[body])), float(np.nanmax(shield[body]))) if body.any() else (0, 0)
    s.Flag("player_shield_out_of_range", body & ~np.isnan(shield) & ~((shield >= 0) & (shield <= SHIELD_MAX)), ticks, h, shield)
    s.Flag("player_ability_energy_out_of_range", body & ~((P["AbilityEnergy"] >= 0) & (P["AbilityEnergy"] <= 1)), ticks, h, P["AbilityEnergy"])
    s.values["grenades_max"] = (int(P["FragGrenades"].max()), int(P["PlasmaGrenades"].max())) if len(P) else (0, 0)
    s.Flag("player_grenades_out_of_range", (P["FragGrenades"] > GRENADES_MAX) | (P["PlasmaGrenades"] > GRENADES_MAX), ticks, h)
    s.Flag("player_selected_grenade_unknown", ~np.isin(P["SelectedGrenade"], [int(g) for g in GrenadeType]), ticks, h, P["SelectedGrenade"])
    s.values["selected_grenade"] = dict(collections.Counter(int(v) for v in P["SelectedGrenade"][body]))
    s.values["ability_active_ticks"] = int((body & (P["IsAbilityActive"] == 1)).sum())
    s.Flag("player_ability_active_flag", ~np.isin(P["IsAbilityActive"], [0, 1]), ticks, h)
    # Camera and aim.
    aim, forward, position = Vec(P["Aim"]), Vec(P["CameraForward"]), Vec(P["CameraPosition"])
    aim_norm, forward_norm = np.linalg.norm(aim, axis=1), np.linalg.norm(forward, axis=1)
    s.Flag("player_aim_not_unit", body & ~(np.abs(aim_norm - 1) <= UNIT_TOLERANCE), ticks, h, aim_norm)
    # The camera is the view of a local player: in a theater film nobody is local and the camera of the viewer is not a player (ignored by tickdump).
    local = body & (P["LocalIndex"] != NOT_LOCAL)
    s.values["local_body_rows"] = int(local.sum())
    s.Flag("player_camera_forward_not_unit", local & ~(np.abs(forward_norm - 1) <= UNIT_TOLERANCE), ticks, h, forward_norm)
    cosine = np.clip((aim * forward).sum(axis=1) / np.where(aim_norm * forward_norm > 0, aim_norm * forward_norm, 1), -1, 1)
    angle = np.degrees(np.arccos(cosine))
    s.values["aim_camera_angle_max_local"] = float(angle[local].max()) if local.any() else 0.0
    s.values["aim_camera_angle_max_not_local"] = float(angle[body & ~local].max()) if (body & ~local).any() else 0.0
    s.Flag("player_aim_differs_from_the_camera", local & ~(angle <= AIM_ANGLE_TOLERANCE_DEGREES), ticks, h, angle)
    distance = np.linalg.norm(position - Vec(biped["WorldOrigin"]), axis=1)
    near = local & biped_found
    s.values["camera_distance_max"] = float(distance[near].max()) if near.any() else 0.0
    s.Flag("player_camera_far_from_the_biped", near & ~(distance <= CAMERA_DISTANCE_MAX), ticks, h, distance)
    # Loadout.
    seated = biped_found & (biped["Binding"] == sem.Binding.Seated)
    holds_objective = (P["Objective"] != NO_HANDLE) & (P["Objective"] != 0)
    for name in ("BipedHandle",) + BODY_HANDLES:
        s.Flag("player_handle_zero_" + name, P[name] == 0, ticks, h)
    captured = [int(t) for t, k in zip(s.EV["GameTick"], s.EV["Kind"]) if k == sem.Event.FlagCaptured]
    after_capture = np.isin(ticks, [t + d for t in captured for d in (0, 1)])
    for column, name in ((1, "PrimaryWeapon"), (2, "SecondaryWeapon")):
        value, found, row = P[name], F[:, column], L[:, column]
        present = body & (value != NO_HANDLE)
        s.Flag("player_" + name + "_not_a_weapon", present & ~(found & (row["Function"] == sem.Function.Weapon)), ticks, h)
        attached = found & ((row["BoundTo"] == P["BipedHandle"]) | (row["ParentHandle"] == P["BipedHandle"]))
        s.Flag("player_" + name + "_not_with_the_biped", present & found & ~attached, ticks, h)
        s.Flag("player_" + name + "_not_with_the_biped_unexplained", present & found & ~attached & ~holds_objective & ~seated & ~after_capture, ticks, h)
        s.Flag("player_" + name + "_lost", present & ~found, ticks, h)
    equipped = P["EquippedWeapon"]
    s.Flag("player_equipped_is_not_a_slot", body & ~((equipped == NO_HANDLE) | (equipped == P["PrimaryWeapon"]) | (equipped == P["SecondaryWeapon"]) |
                                                      (equipped == P["Objective"])), ticks, h)
    s.values["equipped_is"] = {"no_handle": int((body & (equipped == NO_HANDLE)).sum()),
                               "objective": int((body & holds_objective & (equipped == P["Objective"])).sum()),
                               "primary": int((body & (equipped != NO_HANDLE) & (equipped == P["PrimaryWeapon"])).sum()),
                               "secondary": int((body & (equipped != NO_HANDLE) & (equipped == P["SecondaryWeapon"]) &
                                                 (equipped != P["PrimaryWeapon"])).sum())}
    s.Flag("player_ability_not_an_ability", body & (P["Ability"] != NO_HANDLE) & ~(F[:, 4] & (L[:, 4]["Function"] == sem.Function.ArmorAbility)), ticks, h)
    s.Flag("player_objective_not_a_flag", body & holds_objective & ~(F[:, 5] & (L[:, 5]["Function"] == sem.Function.ObjectiveItem)), ticks, h)
    # Damage.
    keys = (record.astype(np.int64) << 32) | h.astype(np.int64)
    damager = P["LastDamagerPlayer"]
    same_tick_players = np.isin((record.astype(np.int64) << 32) | damager.astype(np.int64), keys)
    s.Flag("player_last_damager_unknown", body & (damager != NO_HANDLE) & ~same_tick_players, ticks, h, damager)
    s.values["last_damager_rows"] = int((body & (damager != NO_HANDLE)).sum())
    since = P["TicksSinceDamage"].astype(np.int64)
    known_a, known_b = since[a] != NO_HANDLE, since[b] != NO_HANDLE
    delta = since[b] - since[a]
    s.Flag("player_ticks_since_damage_step", continuing & one_tick & known_a & known_b & (delta != 1) & (since[b] != 0), ticks[b], h[b], delta)
    s.Flag("player_ticks_since_damage_forgotten", continuing & known_a & ~known_b, ticks[b], h[b])
    hit = continuing & one_tick & known_b & (since[b] == 0)
    s.values["damage_hits"] = int(hit.sum())
    # The shield or the health of the biped drops in the tick of the hit or in the next HIT_WINDOW_TICKS ones.
    following = np.full(len(P), -1, np.int64)
    following[a] = b
    shield_a, vitality_a = np.nan_to_num(shield[a]), biped["Vitality"][a].astype(np.float64)
    dropped = np.zeros(len(a), bool)
    row = b.copy()
    for _ in range(HIT_WINDOW_TICKS + 1):
        dropped |= (np.nan_to_num(shield[row]) < shield_a) | (biped["Vitality"][row].astype(np.float64) < vitality_a)
        row = np.where(following[row] >= 0, following[row], row)
    s.Flag("player_hit_without_a_drop", hit & ~dropped, ticks[b], h[b])
    s.values["ticks_since_damage_values"] = (int(since[since != NO_HANDLE].min()) if (since != NO_HANDLE).any() else None,
                                             int(since[since != NO_HANDLE].max()) if (since != NO_HANDLE).any() else None)
    # Connection, local index, zoom.
    s.values["connection_counts"] = dict(collections.Counter(int(v) for v in P["Connection"]))
    s.Flag("player_connection_unknown", ~np.isin(P["Connection"], [int(c) for c in Connection]), ticks, h, P["Connection"])
    s.values["local_index_counts"] = dict(collections.Counter(int(v) for v in P["LocalIndex"]))
    s.values["local_index_handles"] = sorted({int(x) for x in h[P["LocalIndex"] != NOT_LOCAL]})
    s.values["zoom_counts"] = dict(collections.Counter(int(v) for v in P["Zoom"][body]))
    s.Flag("player_zoom_unknown", body & ~np.isin(P["Zoom"], [int(z) for z in ZoomLevel]), ticks, h, P["Zoom"])
    s.Flag("player_team_unknown", ~np.isin(P["Team"], [int(t) for t in Team]), ticks, h, P["Team"])
    # The motion of the biped of the local player: the displacement of one tick against LinearVelocity (wu/s).
    rows = continuing & one_tick & local[a] & local[b] & biped_found[a] & biped_found[b] & (biped["BoundTo"][a] == NO_HANDLE) & (biped["BoundTo"][b] == NO_HANDLE)
    if rows.any():
        shift = Vec(biped["WorldOrigin"][b[rows]]) - Vec(biped["WorldOrigin"][a[rows]])
        residual = shift - Vec(biped["LinearVelocity"][b[rows]]) / TICK_RATE
        unchanged = (Vec(biped["LinearVelocity"][a[rows]]) == Vec(biped["LinearVelocity"][b[rows]])).all(axis=1)
        s.values["velocity_fit"] = {"rows": int(rows.sum()), "rms": float(np.sqrt((residual ** 2).mean())), "max": float(np.abs(residual).max()),
                                    "above_0_01": float((np.abs(residual).max(axis=1) > 0.01).mean()), "unchanged": float(unchanged.mean()),
                                    "speed_max": float(np.linalg.norm(Vec(biped["LinearVelocity"][b[rows]]), axis=1).max())}
    else:
        s.values["velocity_fit"] = None
    # Traits: the distinct rows of each group (no flag and no powerup, with the flag, with each powerup) with their count.
    traits = np.ascontiguousarray(P["Traits"])
    raw = traits.view(f"V{traits.dtype.itemsize}").ravel()
    powerup = P["Powerup"]
    groups = {"base": ~holds_objective & (powerup == 0), "flag": holds_objective & (powerup == 0), "flag_powerup": holds_objective & (powerup != 0)}
    for number, name in ((1, "red"), (2, "blue"), (3, "yellow")):
        groups[name] = ~holds_objective & (powerup == number)
    s.traits_groups = {}
    for group, mask in groups.items():
        if not mask.any():
            continue
        _, first, counts = np.unique(raw[mask], return_index=True, return_counts=True)
        where = np.flatnonzero(mask)[first]
        rows = [(int(count), TraitsKey(traits[index])) for count, index in zip(counts, where)]
        s.traits_groups[group] = sorted(rows, key=lambda row: (-row[0], repr(row[1])))


def AnalyzeMatch(s):
    M, ticks = s.matches, s.ticks
    zeros = np.zeros(len(M), np.int64)
    s.Flag("match_phase_unknown", ~np.isin(M["Phase"], [int(v) for v in sem.MatchPhase]), ticks, zeros, M["Phase"])
    s.Flag("match_round_state_unknown", ~np.isin(M["RoundState"], [int(v) for v in sem.RoundState]), ticks, zeros, M["RoundState"])
    key = np.stack([M["Phase"], M["RoundState"], M["RoundIndex"]], axis=1).astype(np.int64)
    changed = np.concatenate([[True], (key[1:] != key[:-1]).any(axis=1)]) if len(M) else np.zeros(0, bool)
    s.values["match_transitions"] = [(int(ticks[i]), int(key[i][0]), int(key[i][1]), int(key[i][2])) for i in np.flatnonzero(changed)]
    totals, rounds, players = M["TeamTotalScores"].astype(np.int64), M["TeamRoundScores"].astype(np.int64), M["PlayerScores"].astype(np.int64)
    if len(M) > 1:
        s.Flag("match_total_score_decreased", (totals[1:] < totals[:-1]).any(axis=1), ticks[1:], zeros[1:])
        new_round = (M["RoundState"][1:] == sem.RoundState.InPlay) & (M["RoundState"][:-1] == sem.RoundState.Between)
        s.Flag("match_round_score_not_reset", new_round & (rounds[1:] != 0).any(axis=1), ticks[1:], zeros[1:])
        s.Flag("match_player_score_not_reset", new_round & (players[1:] != 0).any(axis=1), ticks[1:], zeros[1:])
        s.Flag("match_round_index_not_by_one", (M["RoundIndex"][1:] - M["RoundIndex"][:-1] > 1) | (M["RoundIndex"][1:] < M["RoundIndex"][:-1]), ticks[1:], zeros[1:])
    s.Flag("match_unused_team_score", (totals[:, 2:] != 0).any(axis=1) | (rounds[:, 2:] != 0).any(axis=1), ticks, zeros)
    score_key = np.concatenate([totals[:, :2], rounds[:, :2], players], axis=1)
    moved = np.concatenate([[True], (score_key[1:] != score_key[:-1]).any(axis=1)]) if len(M) else np.zeros(0, bool)
    s.values["score_changes"] = [(int(ticks[i]), tuple(int(v) for v in totals[i][:2]), tuple(int(v) for v in rounds[i][:2]),
                                  {int(j): int(players[i][j]) for j in np.flatnonzero(players[i])}) for i in np.flatnonzero(moved)]


def AnalyzeEvents(s):
    E, record = s.EV, s.EV_record
    ticks = s.ticks[record] if len(E) else np.zeros(0, np.int64)
    s.values["events"] = len(E)
    if not len(E):
        s.values["engine_events"] = 0
        return
    sequence = E["Sequence"].astype(np.int64)
    s.Flag("event_sequence_not_continuous", sequence != np.arange(1, len(E) + 1), ticks, sequence)
    s.Flag("event_tick_goes_back", np.concatenate([[False], E["GameTick"][1:] < E["GameTick"][:-1]]), ticks, sequence)
    s.Flag("event_kind_unknown", ~np.isin(E["Kind"], [int(k) for k in sem.Event]), ticks, sequence, E["Kind"])
    engine = E["EngineSequence"] != 0
    s.values["engine_events"] = int(engine.sum())
    engine_sequence = E["EngineSequence"][engine].astype(np.int64)
    s.Flag("event_engine_sequence_not_increasing", np.concatenate([[False], engine_sequence[1:] <= engine_sequence[:-1]]), ticks[engine], sequence[engine])
    for index in np.flatnonzero(engine):
        key, kind = int(E["Key"][index]), int(E["Kind"][index])
        if kind != int(sem.event_kind(key)):
            s.Add("event_engine_key_not_in_eventkeys", ticks[index], sequence[index], f"key 0x{key:X} kind {kind}")
    for index in np.flatnonzero(~engine):
        if int(E["Key"][index]) != 0 or int(E["Kind"][index]) not in [int(k) for k in FLAG_KINDS]:
            s.Add("event_artemis_not_a_flag_event", ticks[index], sequence[index], f"key 0x{int(E['Key'][index]):X} kind {int(E['Kind'][index])}")
    s.values["event_kinds"] = dict(collections.Counter(int(k) for k in E["Kind"]))
    s.values["event_headshots"] = int(E["IsHeadshot"].sum())
    s.values["event_stuck"] = int(E["IsStuck"].sum())
    s.values["event_reporting_types"] = dict(collections.Counter(int(k) for k in E["DamageReportingType"]))
    s.Flag("event_flags_not_0_or_1", ~(np.isin(E["IsHeadshot"], [0, 1]) & np.isin(E["IsStuck"], [0, 1])), ticks, sequence)
    # Deaths and spawns against the change of HasBody of the player.
    P, record_p = s.P, s.P_record
    a, b = PlayerPairs(s)
    body = P["HasBody"] == 1
    p_ticks = s.ticks[record_p]
    died = body[a] & ~body[b]
    born = ~body[a] & body[b]
    deaths = {(int(P["Handle"][j]), int(p_ticks[j])) for j in b[died]}
    spawns = {(int(P["Handle"][j]), int(p_ticks[j])) for j in b[born]}
    s.values["bodies_lost"] = len(deaths)
    s.values["bodies_gained"] = len(spawns)
    if not engine.any():
        return
    starts = [int(t) for t, k in zip(ticks, E["Kind"]) if k == sem.Event.RoundStart]
    death_dt, spawn_dt, matched_deaths, matched_spawns = [], [], set(), set()
    for index in range(len(E)):
        kind, tick = int(E["Kind"][index]), int(E["GameTick"][index])
        if kind in (int(sem.Event.Death), int(sem.Event.Kill)):
            victim = int(E["EffectPlayer"][index])
            near = [(abs(t - tick), (h, t)) for h, t in deaths if h == victim and abs(t - tick) <= EVENT_WINDOW_TICKS]
            if not near:
                s.Add("event_death_without_a_body_lost", tick, victim)
            else:
                dt, hit = min(near)
                death_dt.append(dt)
                matched_deaths.add(hit)
        elif kind == int(sem.Event.Spawn):
            who = int(E["CausePlayer"][index])
            near = [(abs(t - tick), (h, t)) for h, t in spawns if h == who and abs(t - tick) <= EVENT_WINDOW_TICKS]
            if not near:
                s.Add("event_spawn_without_a_body_gained", tick, who)
            else:
                dt, hit = min(near)
                spawn_dt.append(dt)
                matched_spawns.add(hit)
    s.values["death_event_distance_max"] = max(death_dt) if death_dt else None
    s.values["spawn_event_distance_max"] = max(spawn_dt) if spawn_dt else None
    s.values["body_lost_without_death"] = sorted((t, h) for h, t in deaths - matched_deaths)
    s.values["round_starts"] = starts
    state = s.matches["RoundState"]
    new_round = np.flatnonzero((state[1:] == sem.RoundState.InPlay) & (state[:-1] == sem.RoundState.Between)) + 1
    s.values["new_round_ticks"] = sorted({int(s.ticks[i]) for i in new_round})


def AnalyzeObjectives(s):
    def Changes(rows, rows_record):
        out = []
        for team in (np.unique(rows["BaseTeam"]) if len(rows) else []):
            mask = rows["BaseTeam"] == team
            owner, at = rows["OwnerTeam"][mask], s.ticks[rows_record[mask]]
            changed = np.concatenate([[True], owner[1:] != owner[:-1]])
            out += [(int(at[i]), int(team), int(owner[i])) for i in np.flatnonzero(changed)]
        return sorted(out)
    s.values["stand_owner_changes"] = Changes(s.S, s.S_record)
    s.values["flag_owner_changes"] = Changes(s.O, s.O_record)
    s.values["flag_rows"] = len(s.O)
    s.values["flag_states"] = dict(collections.Counter(int(v) for v in s.O["State"]))
    s.Flag("flag_state_unknown", ~np.isin(s.O["State"], [int(v) for v in sem.FlagState]), s.ticks[s.O_record], s.O["Handle"])
    s.Flag("flag_team_unknown", ~(np.isin(s.O["BaseTeam"], [int(t) for t in Team]) & np.isin(s.O["OwnerTeam"], [int(t) for t in Team])),
           s.ticks[s.O_record], s.O["Handle"])
    s.Flag("stand_team_unknown", ~(np.isin(s.S["BaseTeam"], [int(t) for t in Team]) & np.isin(s.S["OwnerTeam"], [int(t) for t in Team])),
           s.ticks[s.S_record], s.S["Handle"])


def AnalyzeLifts(s):
    LI = s.LI
    s.values["lift_rows"] = len(LI)
    tuples = {}
    if not len(LI):
        s.values["lift_stage_distance_max"] = 0.0
        s.values["lift_stage_tuples"] = {}
        return
    ticks = s.ticks[s.LI_record]
    origin = Vec(s.LI_origin)
    far = 0.0
    for stage in range(bl.MAX_LIFT_STAGES):
        used = (LI["StageCount"] > stage) & s.LI_found
        row = LI["Stages"][:, stage]
        distance = np.linalg.norm(Vec(row["Position"]) - origin, axis=1)
        far = max(far, float(distance[used].max()) if used.any() else 0.0)
        s.Flag("lift_stage_far_from_the_lift", used & ~(distance <= LIFT_STAGE_DISTANCE_MAX), ticks, LI["Handle"], distance)
        s.Flag("lift_stage_does_not_affect_players", used & ((row["Affects"] & AffectsBit.Players) == 0), ticks, LI["Handle"], row["Affects"])
        s.Flag("lift_stage_axis_not_finite", used & ~(np.isfinite(row["AxisAcceleration"]) & np.isfinite(row["AxisMaximumVelocity"])), ticks, LI["Handle"])
        for index in np.flatnonzero(used):
            key = (int(LI["Handle"][index]), stage)
            value = (float(row["Acceleration"][index]), float(row["MaximumVelocity"][index]), float(row["AxisAcceleration"][index]),
                     float(row["AxisMaximumVelocity"][index]), int(row["Affects"][index]))
            if key not in tuples:
                tuples[key] = value
            elif tuples[key] != value:
                s.Add("lift_stage_changes", ticks[index], key[0], f"stage {stage}")
    s.Flag("lift_stage_count_too_big", LI["StageCount"] > bl.MAX_LIFT_STAGES, ticks, LI["Handle"])
    s.values["lift_stage_distance_max"] = far
    s.values["lift_stage_tuples"] = tuples


# ---------------------------------------------------------------------------------------------------------------------------------
# Constants measured in the goldens
# ---------------------------------------------------------------------------------------------------------------------------------

LOCAL_PLAYER = 0xEC700000  # player table index 0: the only local player of every film but cgb_displace_1min (theater, nobody local).
LOCAL_FILMS = tuple(f for f in FILMS if f != "cgb_displace_1min")
# Player rows with IsAbilityActive, with Body (armor ability in use).
ABILITY_ACTIVE_ROWS = {"armas_a": 3260, "armas_b": 150, "cgb_displace_1min": 7676, "ctf_c": 7602, "ctf_d": 3547, "explosivos": 199, "lifts": 2774,
                       "powerups": 1044, "suelo": 1890}
# Player rows with Body and Zoom1 (the other rows with Body have no zoom). Zoom2 never appears. Not even the DMR of armas_a was zoomed.
ZOOM1_ROWS = {"cgb_displace_1min": 1556, "ctf_c": 136}
# Steps of TicksAlive that are not +1 in a tick: (tick, step). suelo 8248 is the tick the biped leaves the Warthog seat (8032-8248).
TICKS_ALIVE_STEPS = {"suelo": [(8248, 2)], "cgb_displace_1min": [(2146, 2), (2821, 2)]}
# First TicksAlive of a body that is not 1: the player that appears in the last records of cgb (GameTick 3579 three times, D174).
FIRST_TICKS_ALIVE_ODD = {"cgb_displace_1min": [(3579, 0)]}
# Rows with Objective 0 (a handle that is not valid: HALLAZGO 2), cgb tick 3579: the first tick of that body, camera and Objective not written yet.
ZERO_OBJECTIVE_ROWS = {"cgb_displace_1min": 2}
# Hits (TicksSinceDamage 0) after which neither the shield nor the health dropped in the next HIT_WINDOW_TICKS ticks.
HITS_WITHOUT_A_DROP = {"cgb_displace_1min": 6}
# Rows of PrimaryWeapon and SecondaryWeapon that point to a weapon that is not with the biped: the player carries the flag (the flag is the
# equipped weapon and the weapons are on the floor), is seated in a vehicle, or it is the tick of a capture. HALLAZGO 3.
LOADOUT_NOT_WITH_THE_BIPED = {"armas_a": (1646, 1646), "cgb_displace_1min": (2519, 2398), "ctf_c": (4818, 4169), "ctf_d": (2379, 0), "suelo": (216, 216)}
# Events that name a flag that the engine already recreated (FlagReset, FlagReturned): the entity is not in the tick nor in the previous one.
FLAG_EVENT_OBJECTS_GONE = {"ctf_c": 3}
# Engine-measured LastDamagerPlayer rows (with Body).
LAST_DAMAGER_ROWS = {"armas_a": 1207, "armas_b": 0, "cgb_displace_1min": 4090, "ctf_c": 301, "ctf_d": 0, "explosivos": 0, "lifts": 302, "powerups": 442, "suelo": 7}
# Hits of the engine (TicksSinceDamage back to 0 from a known value).
DAMAGE_HITS = {"armas_a": 18, "armas_b": 0, "cgb_displace_1min": 55, "ctf_c": 1, "ctf_d": 0, "explosivos": 0, "lifts": 2, "powerups": 2, "suelo": 1}

# Events of the film by Kind (Artemis numbers them all: flags come from Artemis, the rest from the engine ring).
EVENT_KINDS = {
    "armas_a": {sem.Event.Kill: 5, sem.Event.Death: 6, sem.Event.Spawn: 8, sem.Event.Suicide: 1, sem.Event.GameOver: 1, sem.Event.Medal: 15, sem.Event.Stat: 37,
                sem.Event.RoundStart: 2},
    "armas_b": {sem.Event.Spawn: 1, sem.Event.Stat: 4, sem.Event.RoundStart: 1},
    "cgb_displace_1min": {sem.Event.FlagTaken: 12, sem.Event.FlagDropped: 12},
    "ctf_c": {sem.Event.Death: 1, sem.Event.Spawn: 10, sem.Event.Suicide: 1, sem.Event.GameOver: 1, sem.Event.Stat: 36, sem.Event.FlagTaken: 71,
              sem.Event.FlagDropped: 68, sem.Event.FlagReturned: 2, sem.Event.FlagReset: 1, sem.Event.FlagCaptured: 3, sem.Event.RoundEnd: 2, sem.Event.RoundStart: 9},
    "ctf_d": {sem.Event.Spawn: 6, sem.Event.GameOver: 1, sem.Event.Stat: 27, sem.Event.FlagTaken: 30, sem.Event.FlagDropped: 28, sem.Event.FlagCaptured: 2,
              sem.Event.RoundEnd: 3, sem.Event.RoundStart: 6},
    "explosivos": {sem.Event.Death: 1, sem.Event.Spawn: 1, sem.Event.Suicide: 1, sem.Event.Stat: 7, sem.Event.RoundStart: 1},
    "lifts": {sem.Event.Death: 2, sem.Event.Spawn: 3, sem.Event.Suicide: 2, sem.Event.Medal: 1, sem.Event.Stat: 12, sem.Event.RoundStart: 1},
    "powerups": {sem.Event.Death: 1, sem.Event.Spawn: 2, sem.Event.Suicide: 1, sem.Event.Stat: 8, sem.Event.RoundStart: 1},
    "suelo": {sem.Event.Death: 1, sem.Event.Spawn: 1, sem.Event.Suicide: 1, sem.Event.Medal: 1, sem.Event.Stat: 6, sem.Event.RoundStart: 1},
}
# DamageReportingType of the events: raw ids of the engine (Artemis has no table of them), by count. 0 is no damage (stats, spawns, flags). The suicides
# by a fall are 41 (suelo, lifts) or 42 (lifts, ctf_c), the explosion of a propane tank 44, the stuck plasma grenade 22, the needles 14 (armas_a).
EVENT_REPORTING_TYPES = {"armas_a": {0: 45, 9: 9, 22: 15, 14: 6}, "armas_b": {0: 6}, "cgb_displace_1min": {0: 24}, "ctf_c": {0: 203, 42: 2}, "ctf_d": {0: 103},
                         "explosivos": {0: 9, 44: 2}, "lifts": {0: 16, 42: 2, 41: 3}, "powerups": {0: 11, 22: 2}, "suelo": {0: 8, 41: 3}}
# Events with IsStuck (the projectile that did it was stuck to the victim): armas_a has the plasma grenade (22) and the needles (14) stuck. Never IsHeadshot.
EVENT_STUCK = {"armas_a": 13}
STUCK_REPORTING_TYPES = (14, 22)
# Events that exist only in Artemis (flags) keep Key 0 and EngineSequence 0: the films that have engine events.
FILMS_WITH_ENGINE_EVENTS = tuple(f for f in FILMS if f != "cgb_displace_1min")

INGAME, ENDED, UNKNOWN_PHASE = sem.MatchPhase.InGame, sem.MatchPhase.Ended, sem.MatchPhase.Unknown
PLAY, OVER, BETWEEN, UNKNOWN_STATE = sem.RoundState.InPlay, sem.RoundState.Ended, sem.RoundState.Between, sem.RoundState.Unknown
# (GameTick of the record, Phase, RoundState, RoundIndex) at each change of any of the three. Phase Ended with RoundState InPlay for one tick is the
# tick of each death of the local player and the tick the round is recreated, when the Phase goes back to InGame one tick after the RoundState (HALLAZGO 4).
# ctf_c 11203 is the tick after the capture of 11202, 11564 the one after the recreation of 11563.
PHASE_FLICKER_FILMS = ("armas_a", "ctf_c", "ctf_d", "explosivos", "lifts", "powerups", "suelo")
MATCH_TRANSITIONS = {
    "armas_a": [
        (1, INGAME, PLAY, 0), (8903, ENDED, PLAY, 0), (8904, INGAME, PLAY, 0), (10845, ENDED, PLAY, 0), (10846, INGAME, PLAY, 0),
        (12401, ENDED, PLAY, 0), (12402, INGAME, PLAY, 0), (13839, ENDED, PLAY, 0), (13840, INGAME, PLAY, 0), (16569, ENDED, PLAY, 0),
        (16570, INGAME, PLAY, 0), (17224, ENDED, OVER, 0),
    ],
    "armas_b": [
        (1, INGAME, PLAY, 0),
    ],
    "cgb_displace_1min": [
        (0, UNKNOWN_PHASE, PLAY, 0), (0, INGAME, PLAY, 0),
    ],
    "ctf_c": [
        (1, INGAME, PLAY, 0), (4281, ENDED, PLAY, 0), (4282, INGAME, PLAY, 0), (11203, ENDED, OVER, 0), (11503, ENDED, BETWEEN, 1),
        (11563, ENDED, PLAY, 1), (11564, INGAME, PLAY, 1), (23159, ENDED, OVER, 1), (23459, ENDED, BETWEEN, 2), (23519, ENDED, PLAY, 2),
        (23520, INGAME, PLAY, 2), (32671, ENDED, OVER, 2),
    ],
    "ctf_d": [
        (1, INGAME, PLAY, 0), (7801, ENDED, OVER, 0), (8101, ENDED, BETWEEN, 1), (8161, ENDED, PLAY, 1), (8162, INGAME, PLAY, 1),
        (12356, ENDED, OVER, 1), (12656, ENDED, BETWEEN, 2), (12716, ENDED, PLAY, 2), (12717, INGAME, PLAY, 2), (15981, ENDED, OVER, 2),
    ],
    "explosivos": [
        (1, INGAME, PLAY, 0), (5398, ENDED, PLAY, 0), (5399, INGAME, PLAY, 0),
    ],
    "lifts": [
        (1, INGAME, PLAY, 0), (3319, ENDED, PLAY, 0), (3320, INGAME, PLAY, 0), (9761, ENDED, PLAY, 0), (9762, INGAME, PLAY, 0),
    ],
    "powerups": [
        (1, INGAME, PLAY, 0), (6679, ENDED, PLAY, 0), (6680, INGAME, PLAY, 0),
    ],
    "suelo": [
        (1, INGAME, PLAY, 0), (9441, ENDED, PLAY, 0), (9442, INGAME, PLAY, 0),
    ],
}
SCORE_CHANGES = {
    "armas_a": [
        (1, (0, 0), (0, 0), {}),
    ],
    "armas_b": [
        (1, (0, 0), (0, 0), {}),
    ],
    "cgb_displace_1min": [
        (0, (0, 0), (0, 0), {}),
    ],
    "ctf_c": [
        (1, (0, 0), (0, 0), {}),
        (11203, (1, 0), (1, 0), {0: 1}),
        (11563, (1, 0), (0, 0), {}),
        (23159, (2, 0), (1, 0), {0: 1}),
        (23519, (2, 0), (0, 0), {}),
        (32671, (3, 0), (1, 0), {13: 1}),
    ],
    "ctf_d": [
        (1, (0, 0), (0, 0), {}),
        (12356, (1, 0), (1, 0), {0: 1}),
        (12716, (1, 0), (0, 0), {}),
        (15981, (2, 0), (1, 0), {0: 1}),
    ],
    "explosivos": [
        (1, (0, 0), (0, 0), {}),
    ],
    "lifts": [
        (1, (0, 0), (0, 0), {}),
    ],
    "powerups": [
        (1, (0, 0), (0, 0), {}),
    ],
    "suelo": [
        (1, (0, 0), (0, 0), {}),
    ],
}
STAND_OWNER_CHANGES = {
    "armas_a": ([(1, 0, 0), (1, 1, 1)], [(1, 0, 0), (1, 1, 1)]),
    "armas_b": ([(1, 0, 0), (1, 1, 1)], [(1, 0, 0)]),
    "cgb_displace_1min": ([(0, 0, 0), (0, 1, 1)], [(1, 0, 0), (1, 1, 1)]),
    "ctf_c": ([(1, 0, 0), (1, 1, 1), (11563, 0, 1), (11563, 1, 0), (23519, 0, 0), (23519, 1, 1)], [(1, 0, 0), (1086, 1, 1), (11564, 0, 1), (11564, 1, 0), (23520, 0, 0), (23520, 1, 1)]),
    "ctf_d": ([(1, 0, 0), (1, 1, 1), (8161, 0, 1), (8161, 1, 0), (12716, 0, 0), (12716, 1, 1)], [(1, 0, 0), (6443, 1, 1), (8162, 0, 1), (8162, 1, 0), (12717, 0, 0), (12717, 1, 1)]),
    "explosivos": ([(1, 0, 0), (1, 1, 1)], [(1, 0, 0)]),
    "lifts": ([(1, 0, 0), (1, 1, 1)], [(1, 0, 0)]),
    "powerups": ([(1, 0, 0), (1, 1, 1)], [(1, 0, 0)]),
    "suelo": ([(1, 0, 0), (1, 1, 1)], [(1, 0, 0)]),
}

# Stages of the lifts of the Forge palette placed in the lifts film (film.txt): (handle, stage) -> (Acceleration, MaximumVelocity, AxisAcceleration,
# AxisMaximumVelocity, Affects). The tags: E42101B2 ff_grav_lift, B3 ff_man_cannon_forge, B4 ff_man_cannon_forge_heavy, B5 ff_man_cannon_forge_light,
# B6 ff_veh_man_cannon, B7 dlc_medium_mancannon, B8 _heavy, B9 _light, BA heh_gravlift_tunnels, BB heh_gravlift, BC man_cannon (cex_headlong).
# The stage 1 of the heavy cannon caps at 11.0, the film measured 10.989 (ReplayTests.LiftsCarryTheStagesOfTheirTagsAndTheMeasuredExitOfTheLiftsFilm).
LIFTS_FILM_TABLE = {
    (0xE42101B2, 0): (10.0, 7.0, 8.0, 5.0, 7), (0xE42101B2, 1): (10.0, 7.0, 8.0, 5.0, 7),
    (0xE42201B3, 0): (8.0, 5.0, 10.0, 6.0, 15), (0xE42201B3, 1): (11.0, 7.5, 25.0, 20.0, 15),
    (0xE42301B4, 0): (8.0, 5.0, 10.0, 6.0, 15), (0xE42301B4, 1): (16.0, 11.0, 35.0, 25.0, 15),
    (0xE42401B5, 0): (8.0, 5.0, 10.0, 6.0, 15), (0xE42401B5, 1): (10.0, 8.0, 25.0, 20.0, 15),
    (0xE42501B6, 0): (30.0, 25.0, 8.0, 4.0, 7),
    (0xE42601B7, 0): (0.0, 0.0, 0.0, 0.0, 15), (0xE42601B7, 1): (16.0, 7.800000190734863, 30.0, 24.0, 11), (0xE42601B7, 2): (16.0, 7.800000190734863, 30.0, 24.0, 15),
    (0xE42701B8, 0): (1.0, 1.0, 1.0, 1.0, 15), (0xE42701B8, 1): (18.0, 9.0, 30.0, 24.0, 11), (0xE42701B8, 2): (16.0, 8.0, 30.0, 24.0, 15),
    (0xE42801B9, 0): (1.0, 1.0, 1.0, 1.0, 15), (0xE42801B9, 1): (16.0, 6.5, 20.0, 18.0, 11), (0xE42801B9, 2): (16.0, 6.5, 20.0, 18.0, 15),
    (0xE42901BA, 0): (10.0, 4.0, 10.0, 4.0, 11), (0xE42901BA, 1): (10.0, 3.0, 10.0, 3.0, 11),
    (0xE42A01BB, 0): (30.0, 4.0, 30.0, 4.0, 11), (0xE42A01BB, 1): (20.0, 3.5, 20.0, 3.5, 11),
    (0xE42B01BC, 0): (8.0, 5.0, 10.0, 6.0, 11), (0xE42B01BC, 1): (15.0, 8.0, 25.0, 20.0, 15),
}


def Film(film):
    """Findings of a film, or a skip when the recording is not in the folder."""
    s = Survey(film)
    if s is None:
        pytest.skip(f"no golden.tdump in {os.path.join(RecordingsFolder(), film)} (set ARTEMIS_RECORDINGS)")
    return s


def Films(xfail=(), reason=""):
    """The films as pytest parameters, those of xfail as strict expected failures with the HALLAZGO of the reason."""
    return [pytest.param(f, marks=pytest.mark.xfail(strict=True, reason=reason)) if f in xfail else f for f in FILMS]


ALL = pytest.mark.parametrize("film", FILMS)


# ---------------------------------------------------------------------------------------------------------------------------------
# Header
# ---------------------------------------------------------------------------------------------------------------------------------

@ALL
def test_header_counts_match_the_rows(film):
    """Covers: TickHeader.PlayerCount, TickHeader.EntityCount, TickHeader.ObjectiveCount, TickHeader.StandCount, TickHeader.WeaponCount,
    TickHeader.ProjectileCount, TickHeader.ExplosiveCount, TickHeader.LiftCount, TickHeader.Flags, TickHeader.LastEventSequence.
    Every count is the number of rows of its block, no tick is truncated and LastEventSequence is the Sequence of the newest event so far."""
    s = Film(film)
    for name in ("PlayerCount", "EntityCount", "ObjectiveCount", "StandCount", "WeaponCount", "ProjectileCount", "ExplosiveCount", "LiftCount"):
        s.Clean("header_count_" + name)
    s.Clean("header_flags")
    s.Clean("header_last_event_sequence")
    s.Clean("table_not_sorted_or_repeated")


# ---------------------------------------------------------------------------------------------------------------------------------
# Entities
# ---------------------------------------------------------------------------------------------------------------------------------

@ALL
def test_entity_enums_are_known(film):
    """Covers: Entity.Group, Entity.Lifecycle, Entity.Binding, Entity.Physics, Entity.Capabilities.
    Kind, Function, Group, Lifecycle, Binding and Physics are values of their enums, Capabilities has no bit out of Capability, and a record has each handle once."""
    s = Film(film)
    for name in ("Kind", "Function", "Group", "Lifecycle", "Binding", "Physics"):
        s.Clean("entity_unknown_" + name)
    s.Clean("entity_capability_bit_unknown")
    s.Clean("entity_handle_repeated_in_a_record")
    assert s.values["entity_Group_seen"], "the film has no entity"


@ALL
def test_entity_frames_are_orthonormal(film):
    """Covers: Entity.Forward, Entity.Up.
    |Forward| and |Up| are 1 and Forward.Up is 0 in every row of every kind, with float32 rounding (3.2e-7 at most, no zero vector)."""
    s = Film(film)
    s.Clean("entity_forward_not_unit")
    s.Clean("entity_up_not_unit")
    s.Clean("entity_forward_not_perpendicular_to_up")
    assert max(s.values["frame_deviation"]) < ORTHO_TOLERANCE / 10


@ALL
def test_entity_links_point_to_live_entities(film):
    """Covers: Entity.BoundTo, Entity.ParentHandle.
    Binding is Free exactly when BoundTo is NoHandle, BoundTo is ParentHandle and an entity of the same tick (or it is checked in every tick that removed
    entities). Carried is an item (weapon or equipment) of a biped, Seated a biped of a vehicle, Attached any other child (ClassifierService BindingOf)."""
    s = Film(film)
    for name in ("entity_binding_vs_bound_to", "entity_bound_to_vs_parent_handle", "entity_bound_to_not_in_the_tick", "entity_carried_is_an_item_of_a_biped",
                 "entity_seated_is_a_biped_of_a_vehicle", "entity_attached_is_the_rest", "link_to_a_removed_entity"):
        s.Clean(name)


@ALL
def test_entity_bounds_are_sane(film):
    """Covers: Entity.BoundingRadius, Entity.BoundingCenter.
    The radius is finite and positive (weapons and projectiles have none: 0). The center of an entity that has no parent is within the radius of its
    origin plus 3.5 wu (measured 3.40 at most, a vehicle of cgb, 0.05 in most films). The origin of an entity with a parent is not a point of its own."""
    s = Film(film)
    s.Clean("entity_radius_not_finite")
    s.Clean("entity_radius_not_positive")
    s.Clean("entity_center_outside_the_radius")
    assert s.values["bounds_overshoot_free"] <= BOUNDS_MARGIN


@ALL
def test_entity_vitality_is_a_fraction(film):
    """Covers: Entity.Vitality.
    NaN without vitality. An active entity has a vitality in (0, 1] (never above 1, not even with the red powerup: that is the shield). A dead one has
    0 or less: the fraction goes negative with the damage that killed it (suelo and lifts -22.2, a fall). The biped of a player with Body has more than 0."""
    s = Film(film)
    for name in ("entity_vitality_not_finite", "entity_active_vitality_not_a_fraction", "entity_dead_vitality_above_zero", "entity_vitality_below_the_floor",
                 "player_alive_biped_vitality"):
        s.Clean(name)
    low, high = s.values["vitality_range"]
    assert high <= 1.0 and low >= VITALITY_FLOOR


@pytest.mark.parametrize("film", ["suelo", "lifts"])
def test_entity_velocity_matches_the_motion(film):
    """Covers: Entity.LinearVelocity, Entity.WorldOrigin.
    LinearVelocity is in wu/s: the displacement of the origin of the biped of the local player in a tick (without parent) is the velocity of that
    tick over 60 (rms 0.001 wu, 0.054 at most in suelo and 0.078 in lifts, 0.2% of the ticks above 0.01). The velocity is the one of the physics at 30 Hz
    (A12): in suelo it repeats between consecutive ticks in 47% of the pairs. The entities with a velocity are finite in every film."""
    s = Film(film)
    fit = s.values["velocity_fit"]
    assert fit and fit["rows"] > 8000
    assert fit["rms"] < 0.002 and fit["max"] < 0.1 and fit["above_0_01"] < 0.005, fit
    if film == "suelo":
        assert 0.40 <= fit["unchanged"] <= 0.55, fit
    assert fit["speed_max"] < 20


@ALL
def test_entity_angular_velocity_is_finite_and_turns_somewhere(film):
    """Covers: Entity.AngularVelocity.
    Finite in every row. Objects that turn (props 24, weapons 4, grenades 7, vehicles 2, bipeds 1...) have a non-zero one in every film."""
    s = Film(film)
    s.Clean("entity_angular_velocity_not_finite")
    s.Clean("entity_linear_velocity_not_finite")
    assert s.values["angular_rows"] > 1000, s.values["angular_functions"]
    assert s.values["linear_rows"] > 10000


@ALL
def test_entity_team_of_a_player_biped(film):
    """Covers: Entity.Team.
    The biped of a player (with Body, or the dead one that is still in the table) has the Team of the player."""
    s = Film(film)
    s.Clean("player_biped_team")


# ---------------------------------------------------------------------------------------------------------------------------------
# Players
# ---------------------------------------------------------------------------------------------------------------------------------

@ALL
def test_player_body_and_biped_agree(film):
    """Covers: Player.Handle, Player.BipedHandle, Player.IsAlive, Player.HasBody.
    HasBody is IsAlive (no player is alive without Body). With Body the biped is an active Biped entity of the tick. Without Body the 7 handles of the
    body, GroundObject and TicksSinceDamage are NoHandle and the numbers of the body are zero, BipedHandle is NoHandle (before the first spawn: the engine
    stores 0 and Artemis reads it as NoHandle, F2.A), the dead biped that the table still has, or the handle of a biped that is gone."""
    s = Film(film)
    s.Clean("player_alive_vs_body")
    s.Clean("player_body_biped_not_alive")
    s.Clean("player_no_body_biped_found_but_alive")
    for name in BODY_HANDLES + ("TicksSinceDamage",):
        s.Clean("player_no_body_" + name)
    for name in ("Zoom", "SelectedGrenade", "FragGrenades", "PlasmaGrenades", "IsAbilityActive", "TicksAlive", "AbilityEnergy", "Shield"):
        s.Clean("player_no_body_" + name + "_not_zero")
    s.Clean("player_no_body_aim_not_zero")
    s.Clean("player_handle_zero_BipedHandle")
    kinds = s.values["no_body_biped"]
    assert kinds["no_handle"] + kinds["dead"] + kinds["gone"] == s.values["player_rows"] - s.values["body_rows"]
    assert s.values["body_rows"] > 0


@pytest.mark.parametrize("film", Films(xfail=("cgb_displace_1min",), reason="HALLAZGO 2: Objective is 0 (a handle that is not valid) in the first tick of the body of the last records of cgb"))
def test_player_handles_are_never_zero(film):
    """Covers: Player.Objective.
    No handle of a player row is 0: a handle of salt 0 is never a datum (NoHandle is 0xFFFFFFFF)."""
    s = Film(film)
    for name in ("BipedHandle",) + BODY_HANDLES:
        s.Clean("player_handle_zero_" + name)


@pytest.mark.parametrize("film", Films(xfail=FILMS, reason="HALLAZGO 1: Zoom of a player without Body is 0 (Zoom1), with Body and no zoom it is 0xFF"))
def test_player_zoom_without_body_is_none(film):
    """Covers: Player.Zoom.
    Zoom says no zoom (0xFF) for a player without Body, as with Body and no zoom (ZoomLevel None). Today it reads 0, which is Zoom1."""
    s = Film(film)
    P = s.P
    nobody = P["HasBody"] == 0
    assert (P["Zoom"][nobody] == ZoomLevel.NONE).all()


@ALL
def test_player_ticks_alive_count_up(film):
    """Covers: Player.TicksAlive.
    While the same biped lives TicksAlive goes up 1 per GameTick, and the first tick of a body is 1, but for the measured exceptions (TICKS_ALIVE_STEPS and
    FIRST_TICKS_ALIVE_ODD: suelo 8248 leaving the Warthog, two in cgb, and the last records of cgb)."""
    s = Film(film)
    assert s.values["ticks_alive_steps"] == TICKS_ALIVE_STEPS.get(film, [])
    assert s.values["first_ticks_alive_odd"] == FIRST_TICKS_ALIVE_ODD.get(film, [])
    s.Clean("player_ticks_alive_stands_still")
    assert s.values["first_ticks_alive"].get(FIRST_TICKS_ALIVE, 0) > 0


@pytest.mark.parametrize("film", Films(xfail=("suelo", "cgb_displace_1min"), reason="HALLAZGO 5: TicksAlive jumps 2 in one tick (leaving a seat, cgb 2146 and 2821)"))
def test_player_ticks_alive_never_skips(film):
    """Covers: Player.TicksAlive.
    TicksAlive goes up exactly 1 per GameTick while the biped lives."""
    s = Film(film)
    s.Clean("player_ticks_alive_step")


@ALL
def test_player_shield_and_ability_energy_ranges(film):
    """Covers: Player.Shield, Player.AbilityEnergy, Player.FragGrenades, Player.PlasmaGrenades, Player.SelectedGrenade, Player.IsAbilityActive.
    Shield is NaN or in [0, 4] (1 full, 4 with the red powerup, powerups film), AbilityEnergy in [0, 1], the grenades in [0, 2], SelectedGrenade
    a GrenadeType, IsAbilityActive 0 or 1 and the rows with the ability in use are the measured ones (ABILITY_ACTIVE_ROWS)."""
    s = Film(film)
    for name in ("player_shield_out_of_range", "player_ability_energy_out_of_range", "player_grenades_out_of_range", "player_selected_grenade_unknown",
                 "player_ability_active_flag"):
        s.Clean(name)
    assert s.values["ability_active_ticks"] == ABILITY_ACTIVE_ROWS[film]
    assert set(s.values["selected_grenade"]) <= {int(g) for g in GrenadeType}


@ALL
def test_player_camera_and_aim(film):
    """Covers: Player.CameraPosition, Player.CameraForward, Player.Aim.
    Aim is a unit vector with Body. For a local player (with Body) CameraForward is a unit vector, equal to Aim (1.2e-6 degrees at most), and the
    camera is within 3.5 wu of the origin of its biped (0.66 on foot, 3.09 in the Warthog). The camera of a player that is not local, or of the theater, is not asserted."""
    s = Film(film)
    s.Clean("player_aim_not_unit")
    s.Clean("player_camera_forward_not_unit")
    s.Clean("player_aim_differs_from_the_camera")
    s.Clean("player_camera_far_from_the_biped")
    assert (s.values["local_body_rows"] > 0) == (film in LOCAL_FILMS)


@ALL
def test_player_loadout_handles(film):
    """Covers: Player.PrimaryWeapon, Player.SecondaryWeapon, Player.EquippedWeapon, Player.Ability.
    PrimaryWeapon and SecondaryWeapon are NoHandle or a weapon of the tick. The weapon is with the biped (BoundTo or ParentHandle) except while the player
    carries the flag, is seated in a vehicle or in the tick of a capture (LOADOUT_NOT_WITH_THE_BIPED, HALLAZGO 3). EquippedWeapon is NoHandle, the Primary,
    the Secondary or the Objective. Ability is NoHandle or an armor ability of the tick. The rows with the ability in use are in ABILITY_ACTIVE_ROWS."""
    s = Film(film)
    for name in ("PrimaryWeapon", "SecondaryWeapon"):
        s.Clean("player_" + name + "_not_a_weapon")
        s.Clean("player_" + name + "_not_with_the_biped_unexplained")
        s.Clean("player_" + name + "_lost")
    s.Clean("player_equipped_is_not_a_slot")
    s.Clean("player_ability_not_an_ability")
    primary, secondary = LOADOUT_NOT_WITH_THE_BIPED.get(film, (0, 0))
    s.Exactly("player_PrimaryWeapon_not_with_the_biped", primary)
    s.Exactly("player_SecondaryWeapon_not_with_the_biped", secondary)
    assert sum(s.values["equipped_is"].values()) == s.values["body_rows"]


@pytest.mark.parametrize("film", Films(xfail=tuple(LOADOUT_NOT_WITH_THE_BIPED), reason="HALLAZGO 3: PrimaryWeapon and SecondaryWeapon keep the handle of a weapon the player no longer carries"))
def test_player_loadout_weapons_are_always_with_the_biped(film):
    """Covers: Player.PrimaryWeapon, Player.SecondaryWeapon.
    PrimaryWeapon and SecondaryWeapon are NoHandle or a weapon of the biped, always."""
    s = Film(film)
    s.Clean("player_PrimaryWeapon_not_with_the_biped")
    s.Clean("player_SecondaryWeapon_not_with_the_biped")


@pytest.mark.parametrize("film", ["ctf_c", "ctf_d", "cgb_displace_1min"])
def test_player_objective_is_the_flag_it_carries(film):
    """Covers: Player.Objective.
    Objective is the flag whose CarrierPlayer is the player, and each Carried flag has its carrier with Objective equal to it, in every tick (ctf_c 11203: after the
    capture of 11202 the player 0 is left without Objective). The Objective is an ObjectiveItem entity. Players without a flag have NoHandle."""
    s = Film(film)
    s.Clean("flag_carrier_objective")
    s.Clean("player_objective_not_carried")
    s.Clean("player_objective_not_a_flag")
    assert s.values["flag_states"].get(int(sem.FlagState.Carried), 0) > 0
    s.Exactly("player_handle_zero_Objective", ZERO_OBJECTIVE_ROWS.get(film, 0))


@ALL
def test_player_damage_fields(film):
    """Covers: Player.LastDamagerPlayer, Player.TicksSinceDamage.
    LastDamagerPlayer is NoHandle or a player of the same tick. TicksSinceDamage is NoHandle until the first hit, then goes up 1 per tick or goes back to 0 at
    a hit, and never goes back to NoHandle. In the tick of a hit the shield or the health of the biped drops in that tick or in the next 3, but for
    the measured exceptions (HITS_WITHOUT_A_DROP: 6 in cgb, hits that took nothing). The rows with a damager and the hits are the measured ones."""
    s = Film(film)
    s.Clean("player_last_damager_unknown")
    s.Clean("player_ticks_since_damage_step")
    s.Clean("player_ticks_since_damage_forgotten")
    s.Exactly("player_hit_without_a_drop", HITS_WITHOUT_A_DROP.get(film, 0))
    assert s.values["last_damager_rows"] == LAST_DAMAGER_ROWS[film]
    assert s.values["damage_hits"] == DAMAGE_HITS[film]


@ALL
def test_player_connection_and_local_index(film):
    """Covers: Player.Connection, Player.LocalIndex, Player.AppliedDecision, Player.Team.
    Connection is one of the measured values (Connection enum: 1 connected, 8 and 9 connecting before the first spawn, 17, and 10 in cgb). LocalIndex is 0 for
    the player 0 and NotLocal for the others (nobody is local in cgb, a theater film; the second human of armas_a is a remote client). The goldens have
    no Python client: AppliedDecision is 0. Team is one of the Team values."""
    s = Film(film)
    s.Clean("player_connection_unknown")
    s.Clean("player_team_unknown")
    assert s.values["local_index_handles"] == ([] if film not in LOCAL_FILMS else [LOCAL_PLAYER])
    assert set(s.values["local_index_counts"]) <= {0, NOT_LOCAL}
    assert not s.P["AppliedDecision"].any()
    if film in LOCAL_FILMS:
        assert s.values["local_index_counts"][0] == s.record_count


@ALL
def test_player_zoom(film):
    """Covers: Player.Zoom.
    With Body Zoom is a ZoomLevel. Zoom1 appears only in ctf_c (136 rows) and cgb (1556): nobody zoomed in the other films, not even the DMR of armas_a. Zoom2 never."""
    s = Film(film)
    s.Clean("player_zoom_unknown")
    counts = s.values["zoom_counts"]
    assert counts.get(int(ZoomLevel.Zoom1), 0) == ZOOM1_ROWS.get(film, 0)
    assert counts.get(int(ZoomLevel.Zoom2), 0) == 0
    assert sum(counts.values()) == s.values["body_rows"]


@ALL
def test_player_traits_base_rows(film):
    """Covers: Player.Traits, PlayerTraits.HealthMultiplier, PlayerTraits.HealthRegenRate, PlayerTraits.ShieldMultiplier, PlayerTraits.ShieldRegenRate, PlayerTraits.OvershieldRegenRate, PlayerTraits.ShieldVampirism,
    PlayerTraits.DamageMultiplier, PlayerTraits.MeleeMultiplier, PlayerTraits.RadarRange, PlayerTraits.HeadshotImmune, PlayerTraits.AssassinationImmune, PlayerTraits.CannotDieFromDamage,
    PlayerTraits.GrenadeRegen, PlayerTraits.WeaponPickup, PlayerTraits.AbilitiesDropOnDeath, PlayerTraits.InfiniteAbilities, PlayerTraits.InfiniteAmmo, PlayerTraits.AbilityUsage, PlayerTraits.VehicleUse,
    PlayerTraits.VisibleWaypoint, PlayerTraits.VisibleName, PlayerTraits.RadarMode, PlayerTraits.FragGrenades, PlayerTraits.PlasmaGrenades, PlayerTraits.PrimaryWeapon, PlayerTraits.SecondaryWeapon,
    PlayerTraits.Ability, PlayerTraits.DamageResistance, PlayerTraits.MovementSpeed, PlayerTraits.Gravity, PlayerTraits.JumpHeight, PlayerTraits.ActiveCamo.
    The 31 fields of the Traits of the players with no flag and no powerup are the main row of the film (TRAITS_MAIN) in nearly all the rows, and the other rows are the
    measured ones (TRAITS_FILM: radar 50 for the 180 ticks after a spawn, the carrier of the flag the tick before it takes it...). With the flag
    jump 1.5 and RadarRange 75 (ctf_c 11201, 25 without it). With a powerup: red DamageResistance infinite and shield x4, blue the camo 5, yellow infinite resistance."""
    s = Film(film)
    main_name, groups = TRAITS_FILM[film]
    main = TRAITS_MAIN[main_name]
    names = list(main)

    def Key(overrides):
        merged = {**main, **overrides}
        return tuple("nan" if isinstance(merged[n], float) and math.isnan(merged[n]) else merged[n] for n in names)

    expected = {g: sorted([(count, Key(over)) for count, over in rows], key=lambda row: (-row[0], repr(row[1]))) for g, rows in groups.items()}
    assert list(s.P["Traits"].dtype.names) == names
    assert s.traits_groups == expected


# ---------------------------------------------------------------------------------------------------------------------------------
# Flags and stands
# ---------------------------------------------------------------------------------------------------------------------------------

CTF_FILMS = ("ctf_c", "ctf_d", "cgb_displace_1min")


@pytest.mark.parametrize("film", CTF_FILMS)
def test_flag_carrier_biped(film):
    """Covers: Objective.CarrierBiped, Objective.CarrierPlayer, Objective.State.
    State Carried exactly when CarrierPlayer is not NoHandle, and CarrierBiped is the BipedHandle of that player in the same tick (NoHandle without carrier)."""
    s = Film(film)
    s.Clean("flag_state_vs_carrier")
    s.Clean("flag_carrier_biped")
    s.Clean("flag_state_unknown")
    assert s.values["flag_states"].get(int(sem.FlagState.Carried), 0) > 0


@pytest.mark.parametrize("film", CTF_FILMS)
def test_flag_home_stand(film):
    """Covers: Objective.HomeStand.
    HomeStand is a stand of the same tick with the same BaseTeam as the flag."""
    s = Film(film)
    s.Clean("flag_home_stand_missing")
    s.Clean("flag_home_stand_base_team")


@pytest.mark.parametrize("film", CTF_FILMS)
def test_owner_teams(film):
    """Covers: Objective.OwnerTeam, Stand.OwnerTeam.
    The OwnerTeam of a flag is the one of its HomeStand. In ctf_c and ctf_d the sides swap in each round (ctf_c 11563: the stand of BaseTeam 0 passes to OwnerTeam 1, back at
    23519 with the flag one tick later) and in cgb they are constant. The exact changes are STAND_OWNER_CHANGES (tick, BaseTeam, OwnerTeam)."""
    s = Film(film)
    s.Clean("flag_owner_team_vs_stand")
    s.Clean("flag_team_unknown")
    s.Clean("stand_team_unknown")
    stands, flags = STAND_OWNER_CHANGES[film]
    assert s.values["stand_owner_changes"] == stands
    assert s.values["flag_owner_changes"] == flags


@pytest.mark.parametrize("film", [f for f in FILMS if f not in CTF_FILMS])
def test_films_without_flags_have_none(film):
    """The films that are not a capture game carry only the stands of the map and no flag state but Home (the armas, suelo, lifts... have a flag that nobody takes), and nobody has an Objective."""
    s = Film(film)
    assert set(s.values["flag_states"]) <= {int(sem.FlagState.Home)}
    s.Clean("flag_carrier_objective")
    s.Clean("player_objective_not_carried")


# ---------------------------------------------------------------------------------------------------------------------------------
# Match
# ---------------------------------------------------------------------------------------------------------------------------------

@ALL
def test_match_phase_and_round_state(film):
    """Covers: Match.Phase, Match.RoundState.
    Both are values of their enums and the changes of (Phase, RoundState, RoundIndex) are exactly MATCH_TRANSITIONS: Phase Ended with RoundState Ended is the end of
    the round (ctf_c 11202 capture -> 11203), Between the break (11503), the recreation of the round is the tick RoundState goes back to InPlay (11563) and 11564 InGame."""
    s = Film(film)
    s.Clean("match_phase_unknown")
    s.Clean("match_round_state_unknown")
    s.Clean("match_round_index_not_by_one")
    assert s.values["match_transitions"] == [(t, int(p), int(r), i) for t, p, r, i in MATCH_TRANSITIONS[film]]


@pytest.mark.parametrize("film", Films(xfail=PHASE_FLICKER_FILMS, reason="HALLAZGO 4: Phase is Ended with the round in play for one tick at each death of the local player and at each recreation of a round"))
def test_match_phase_is_ingame_while_the_round_is_in_play(film):
    """Covers: Match.Phase.
    While RoundState is InPlay the Phase is InGame (or Unknown while loading), never Ended."""
    s = Film(film)
    M = s.matches
    assert not ((M["Phase"] == ENDED) & (M["RoundState"] == PLAY)).any()


@ALL
def test_match_scores(film):
    """Covers: Match.PlayerScores, Match.TeamRoundScores, Match.TeamTotalScores.
    TeamTotalScores never goes down, the teams out of the first two stay 0, TeamRoundScores and PlayerScores go back to 0 when the round starts (RoundState from
    Between to InPlay) and every score moves exactly as SCORE_CHANGES (ctf_c: +1 total, round and player 0 at 11203, the bot, player index 13, scores the capture of 32671). Each FlagCaptured is one point of the
    team and of the player (by player table index) at most 3 ticks later."""
    s = Film(film)
    s.Clean("match_total_score_decreased")
    s.Clean("match_unused_team_score")
    s.Clean("match_round_score_not_reset")
    s.Clean("match_player_score_not_reset")
    assert s.values["score_changes"] == SCORE_CHANGES[film]
    # Captures against points: the total of the two teams goes up by 1 with each FlagCaptured, at most 3 ticks later, and the player that scores it is its cause.
    events = s.EV
    captures = [(int(e["GameTick"]), int(e["CausePlayer"]) & 0xFFFF) for e in events if e["Kind"] == sem.Event.FlagCaptured]
    changes = SCORE_CHANGES[film]
    increases = [(t, scores) for (_, before, _, _), (t, after, _, scores) in zip(changes, changes[1:]) if sum(after) == sum(before) + 1]
    assert len(increases) == len(captures)
    for (capture, scorer), (point, scores) in zip(captures, increases):
        assert 0 <= point - capture <= 3, (film, capture, point)
        assert scores == {scorer: 1}, (film, capture, scorer, scores)


# ---------------------------------------------------------------------------------------------------------------------------------
# Events
# ---------------------------------------------------------------------------------------------------------------------------------

@ALL
def test_engine_event_sequence(film):
    """Covers: Event.Sequence, Event.EngineSequence, Event.Key.
    Sequence goes 1, 2, 3... without gaps in the order of the log, EngineSequence of the events of the engine goes up, the Kind of each engine event is the one of
    its Key (event_kind of Semantic.xml: the keys of EVENT_KEYS, the statistics 0x78xxxx, the rest medals) and the events that Artemis makes (flags) have Key 0 and
    EngineSequence 0. The events of each Kind are EVENT_KINDS. Only cgb (theater) has no events of the engine (Q88)."""
    s = Film(film)
    s.Clean("event_sequence_not_continuous")
    s.Clean("event_tick_goes_back")
    s.Clean("event_kind_unknown")
    s.Clean("event_engine_sequence_not_increasing")
    s.Clean("event_engine_key_not_in_eventkeys")
    s.Clean("event_artemis_not_a_flag_event")
    assert (s.values["engine_events"] > 0) == (film in FILMS_WITH_ENGINE_EVENTS)
    assert s.values["event_kinds"] == {int(k): n for k, n in EVENT_KINDS[film].items()}


@pytest.mark.parametrize("film", FILMS_WITH_ENGINE_EVENTS)
def test_engine_deaths_match_the_bodies(film):
    """Covers: Event.CausePlayer, Event.EffectPlayer.
    Each Death or Kill has its EffectPlayer going from HasBody 1 to 0, each Spawn its CausePlayer from 0 to 1, both within 3 ticks of the event (measured 1), and each
    HasBody 1 to 0 without a Death is the recreation of a round (the tick RoundState goes from Between to InPlay: ctf_c 11563 and 23519, ctf_d 8161 and 12716)."""
    s = Film(film)
    s.Clean("event_death_without_a_body_lost")
    s.Clean("event_spawn_without_a_body_gained")
    lost = s.values["body_lost_without_death"]
    assert {tick for tick, _ in lost} <= set(s.values["new_round_ticks"]), lost
    deaths, spawns = EVENT_KINDS[film].get(sem.Event.Death, 0), EVENT_KINDS[film].get(sem.Event.Spawn, 0)
    assert s.values["bodies_lost"] == deaths + len(lost)
    assert s.values["bodies_gained"] == spawns


@ALL
def test_engine_event_objects(film):
    """Covers: Event.CauseObject, Event.EffectObject, Event.DamageReportingType, Event.IsHeadshot, Event.IsStuck, Event.GameTick, Event.Kind.
    CauseObject and EffectObject are NoHandle or an entity of the tick of the event or of the previous one (the three FlagReset or FlagReturned of ctf_c name a flag the engine
    had already recreated). CausePlayer and EffectPlayer are NoHandle or a player of that tick or the previous. IsHeadshot never happens in the films, IsStuck in armas_a only
    (13 events, the stuck plasma grenade 22 and the needles 14), and the DamageReportingType of the events are the measured ones (EVENT_REPORTING_TYPES)."""
    s = Film(film)
    s.Clean("event_object_unknown_CauseObject")
    s.Clean("event_object_unknown_EffectObject")
    s.Clean("event_player_unknown_CausePlayer")
    s.Clean("event_player_unknown_EffectPlayer")
    s.Clean("event_flags_not_0_or_1")
    s.Exactly("event_flag_object_gone", FLAG_EVENT_OBJECTS_GONE.get(film, 0))
    assert s.values["event_headshots"] == 0
    assert s.values["event_stuck"] == EVENT_STUCK.get(film, 0)
    assert s.values["event_reporting_types"] == EVENT_REPORTING_TYPES[film]
    stuck = s.EV[s.EV["IsStuck"] == 1]
    assert set(stuck["DamageReportingType"].tolist()) <= set(STUCK_REPORTING_TYPES)


# ---------------------------------------------------------------------------------------------------------------------------------
# Lifts
# ---------------------------------------------------------------------------------------------------------------------------------

@ALL
def test_lift_stage_rest(film):
    """Covers: LiftStage.Position, LiftStage.AxisAcceleration, LiftStage.AxisMaximumVelocity, LiftStage.Affects.
    The Position of each stage is within 4 wu of the WorldOrigin of its lift (3.947 at most), Affects has the bit of the players in every stage (Forge lifts and the
    map ones: 11, 15 or 7), the axis values are finite and constant over the film, and in the lifts film the stages of the Forge lifts are LIFTS_FILM_TABLE."""
    s = Film(film)
    s.Clean("lift_stage_far_from_the_lift")
    s.Clean("lift_stage_does_not_affect_players")
    s.Clean("lift_stage_axis_not_finite")
    s.Clean("lift_stage_changes")
    s.Clean("lift_stage_count_too_big")
    assert s.values["lift_stage_distance_max"] <= LIFT_STAGE_DISTANCE_MAX
    assert s.values["lift_rows"] > 0
    if film == "lifts":
        table = {k: v for k, v in s.values["lift_stage_tuples"].items() if k in LIFTS_FILM_TABLE}
        assert table == LIFTS_FILM_TABLE


# ---------------------------------------------------------------------------------------------------------------------------------
# Coverage: every field of the Tick is asserted (F2)
# ---------------------------------------------------------------------------------------------------------------------------------

TICK_STRUCTS = ("BridgeTickHeader", "BridgeMatch", "BridgeEvent", "BridgePlayerTraits", "BridgePlayer", "BridgeEntity", "BridgeObjective", "BridgeStand",
                "BridgeWeapon", "BridgeMagazine", "BridgeProjectile", "BridgeExplosive", "BridgeLiftStage", "BridgeLift")
REPLAY = "ReplayTests."
COVERED = {}


def Cover(struct, test, *fields):
    for field in fields:
        COVERED[f"{struct}.{field}"] = test


Cover("TickHeader", "test_header_counts_match_the_rows", "PlayerCount", "EntityCount", "ObjectiveCount", "StandCount", "WeaponCount", "ProjectileCount",
      "ExplosiveCount", "LiftCount", "Flags", "LastEventSequence")
Cover("TickHeader", REPLAY + "MapIdIsTheSameNonZeroValueInEachFilm", "MapId")
Cover("Match", REPLAY + "CtfFlagEventsMatchTheExpectedList", "GameTick")
Cover("Match", "test_match_phase_and_round_state", "Phase", "RoundState")
Cover("Match", "test_match_scores", "PlayerScores", "TeamRoundScores", "TeamTotalScores")
Cover("Match", REPLAY + "CtfObjectivesFollowTheRecordedMatch", "RoundIndex")
Cover("Match", REPLAY + "CtfDRoundCountsDownFromTheTimeLimit", "RoundClockTicks", "RoundTicksRemaining", "RoundTimeLimitTicks", "RoundLimit", "ScoreToWin")
Cover("Match", REPLAY + "VariantSettingsOfCtfCAndCgbMatchTheirGametypes", "RespawnTimeTicks", "SuddenDeathTicks", "GracePeriodTicks", "PowerupDurationTicks",
      "LivesPerRound", "TeamLivesPerRound", "RoundsToWin")
Cover("Event", "test_engine_event_sequence", "Sequence", "EngineSequence", "Key")
Cover("Event", "test_engine_deaths_match_the_bodies", "CausePlayer", "EffectPlayer")
Cover("Event", "test_engine_event_objects", "CauseObject", "EffectObject", "DamageReportingType", "IsHeadshot", "IsStuck", "GameTick", "Kind")
Cover("PlayerTraits", "test_player_traits_base_rows", "DamageResistance", "HealthMultiplier", "HealthRegenRate", "ShieldMultiplier", "ShieldRegenRate",
      "OvershieldRegenRate", "ShieldVampirism", "DamageMultiplier", "MeleeMultiplier", "MovementSpeed", "Gravity", "JumpHeight", "RadarRange", "HeadshotImmune",
      "AssassinationImmune", "CannotDieFromDamage", "GrenadeRegen", "WeaponPickup", "AbilitiesDropOnDeath", "InfiniteAbilities", "InfiniteAmmo", "AbilityUsage",
      "VehicleUse", "VisibleWaypoint", "VisibleName", "RadarMode", "FragGrenades", "PlasmaGrenades", "ActiveCamo", "PrimaryWeapon", "SecondaryWeapon", "Ability")
Cover("Player", "test_player_body_and_biped_agree", "Handle", "BipedHandle", "IsAlive", "HasBody")
Cover("Player", REPLAY + "RespawnTicksCountDownOnePerGameTickOnlyWithoutBody", "RespawnTicks")
Cover("Player", REPLAY + "PowerupsFilmCountsDownOnePerTickAndTheTraitsFollowOneTickLater", "PowerupTicks", "Powerup")
Cover("Player", "test_player_connection_and_local_index", "Team", "LocalIndex", "Connection", "AppliedDecision")
Cover("Player", "test_player_zoom", "Zoom")
Cover("Player", "test_player_shield_and_ability_energy_ranges", "SelectedGrenade", "FragGrenades", "PlasmaGrenades", "IsAbilityActive", "AbilityEnergy", "Shield")
Cover("Player", REPLAY + "SueloGroundAndCrouchFollowTheRecordedFilm", "Ground", "IsGrounded", "IsCrouching", "AirborneTicks", "GroundObject", "CrouchFraction",
      "OwnVelocity")
Cover("Player", "test_player_camera_and_aim", "CameraPosition", "CameraForward", "Aim")
Cover("Player", "test_player_ticks_alive_count_up", "TicksAlive")
Cover("Player", "test_player_loadout_handles", "PrimaryWeapon", "SecondaryWeapon", "EquippedWeapon", "Ability")
Cover("Player", "test_player_objective_is_the_flag_it_carries", "Objective")
Cover("Player", "test_player_damage_fields", "LastDamagerPlayer", "TicksSinceDamage")
Cover("Player", "test_player_traits_base_rows", "Traits")
Cover("Entity", REPLAY + "ExplosivesCarryTheBlastOfTheirTags", "Handle", "Function", "Subtype")
Cover("Entity", REPLAY + "CtfObjectivesFollowTheRecordedMatch", "Kind")
Cover("Entity", "test_entity_team_of_a_player_biped", "Team")
Cover("Entity", "test_entity_enums_are_known", "Group", "Lifecycle", "Binding", "Physics", "Capabilities")
Cover("Entity", "test_entity_links_point_to_live_entities", "BoundTo", "ParentHandle")
Cover("Entity", "test_entity_bounds_are_sane", "BoundingRadius", "BoundingCenter")
Cover("Entity", "test_entity_vitality_is_a_fraction", "Vitality")
Cover("Entity", "test_entity_velocity_matches_the_motion", "LinearVelocity", "WorldOrigin")
Cover("Entity", "test_entity_frames_are_orthonormal", "Forward", "Up")
Cover("Entity", "test_entity_angular_velocity_is_finite_and_turns_somewhere", "AngularVelocity")
Cover("Objective", REPLAY + "CtfObjectivesFollowTheRecordedMatch", "Handle", "ResetTicks", "BaseTeam", "IsResetRunning", "AlliesInZone")
Cover("Objective", "test_flag_carrier_biped", "CarrierPlayer", "CarrierBiped", "State")
Cover("Objective", "test_flag_home_stand", "HomeStand")
Cover("Objective", "test_owner_teams", "OwnerTeam")
Cover("Stand", REPLAY + "CtfObjectivesFollowTheRecordedMatch", "Handle", "BaseTeam")
Cover("Stand", "test_owner_teams", "OwnerTeam")
Cover("Magazine", REPLAY + "ArmasBMagazinesReservesAndBatteryFollowTheRecordedFilm", "Loaded", "Reserve")
Cover("Weapon", REPLAY + "ArmasAWeaponsAndProjectilesFollowTheRecordedFilm", "Handle", "TrackedObject", "Energy", "Heat", "ChargeFraction", "Magazines", "MagazineCount",
      "IsEnergyWeapon", "IsOverheated", "IsReloading", "ChargeState", "IsLocked", "CanFireNow")
Cover("Projectile", REPLAY + "ArmasAWeaponsAndProjectilesFollowTheRecordedFilm", "Handle", "OwnerPlayer", "OwnerBiped", "ShooterRoot", "StuckTo", "StuckNode", "InHand")
Cover("Explosive", REPLAY + "ExplosivesCarryTheBlastOfTheirTags", "Handle", "RadiusMin", "RadiusMax", "Damage")
Cover("LiftStage", REPLAY + "LiftsCarryTheStagesOfTheirTagsAndTheMeasuredExitOfTheLiftsFilm", "Direction", "Acceleration", "MaximumVelocity")
Cover("LiftStage", "test_lift_stage_rest", "Position", "AxisAcceleration", "AxisMaximumVelocity", "Affects")
Cover("Lift", REPLAY + "LiftsCarryTheStagesOfTheirTagsAndTheMeasuredExitOfTheLiftsFilm", "Handle", "StageCount", "HasExit", "ExitTick", "ExitBiped", "ExitVelocity", "Stages")

# Fields with no assertion on purpose, with the reason: the identity and the clock of the bridge, which differ on every run (tickdump.py IGNORED lists them in every diff).
EXEMPT = {
    "TickHeader.MatchId": "Engine starts since Artemis loaded: it changes with every recording.",
    "TickHeader.ActionTick": "Clock of the bridge: the action tick of the run that recorded the film.",
    "TickHeader.Generation": "Id of the Tick snapshot: not game time.",
    "TickHeader.PublishQpc": "QueryPerformanceCounter of the machine that recorded.",
    "TickHeader.EventCount": "Window of events in use in the bridge region: the dump keeps only the new events of each record, so there is nothing to compare with.",
}


def BridgeFields():
    """{"Struct.Field": ...} of the structs of the Tick in Data/Bridge/Bridge.xml (the name of the struct without Bridge)."""
    root = ElementTree.parse(os.path.join(Repo(), "Data", "Bridge", "Bridge.xml")).getroot()
    out = {}
    for struct in root.findall("struct"):
        if struct.get("name") in TICK_STRUCTS:
            for field in struct.findall("field"):
                out[f"{struct.get('name')[len('Bridge'):]}.{field.get('name')}"] = field
    return out


def test_every_tick_field_is_asserted():
    """Every field of the structs of the Tick (Bridge.xml) has an assertion: it is in COVERED with the test that asserts it (here, and its docstring says so, or
    ReplayTests, which must have that test and name the field) or in EXEMPT with its reason. A field added to Bridge.xml without an entry fails."""
    fields = BridgeFields()
    missing = sorted(f for f in fields if f not in COVERED and f not in EXEMPT)
    assert not missing, "Fields of the Tick with no assertion (add them to COVERED or EXEMPT): " + ", ".join(missing)
    both = sorted(f for f in fields if f in COVERED and f in EXEMPT)
    assert not both, "Fields in COVERED and EXEMPT: " + ", ".join(both)
    unknown = sorted(f for f in list(COVERED) + list(EXEMPT) if f not in fields)
    assert not unknown, "Entries of COVERED or EXEMPT that are not fields of Bridge.xml: " + ", ".join(unknown)
    source = open(os.path.join(Repo(), "Artemis", "7_Tests", "Tests", "Perception", "ReplayTests.cpp"), encoding="utf-8", errors="replace").read()
    bodies = {name: body for name, body in re.findall(r"TEST_F\(ReplayTests, (\w+)\)(.*?)(?=\nTEST_F\(|\Z)", source, re.S)}
    here = sys.modules[__name__]
    wrong = []
    for field, test in COVERED.items():
        name = field.split(".")[1]
        if test.startswith(REPLAY):
            if test[len(REPLAY):] not in bodies or not re.search(r"\b" + name + r"\b", source):
                wrong.append(f"{field}: {test} does not exist in ReplayTests.cpp or the file does not name the field")
        else:
            function = getattr(here, test, None)
            if function is None or field not in (function.__doc__ or ""):
                wrong.append(f"{field}: {test} does not exist here or its docstring does not say 'Covers' that field")
    assert not wrong, "\n".join(wrong)
    assert all(reason for reason in EXEMPT.values())
