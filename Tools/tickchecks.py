"""
tickchecks.py
The invariants of a recorded match that do not depend on which match it is, and the pass over a .tdump that finds them (F1 sub-commit 2, F2.A D270).
One implementation for two users: Tools/Tests/test_golden_tick.py asserts them on the golden of each film, and `tickdump.py assert FILE` runs the
general ones (GENERAL_RULES) on any .tdump, so game.py can check a bench match as soon as it ends.

Collect(label, dump) is the pass: it gathers every table of the dump once and fills a Findings with the count and the first 10 cases of each rule
(tick, handle, detail), plus the measured values the golden tests compare. A rule is general when it needs no constant measured in a film, no
film.txt and no list of films: those stay in test_golden_tick.py and never run on a live match. Evaluate(dump) adds the rules of the file itself
(dropped ticks, cut file) and returns the failing general rules.
"""
import collections
import enum
import functools
import math
import os
import sys

import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "Agent"))
from artemis import bridge_layout as bl  # noqa: E402
from artemis import semantic as sem  # noqa: E402

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
        if objective != NO_HANDLE and carried_by.get(player) != objective:
            s.Add("player_objective_not_carried", tick, player, f"Objective 0x{objective:08X}")


# Frames: |Forward|, |Up| and Forward.Up measured at most 3.2e-7 in 9 films (float32 rounding), every kind, no zero vector.
ORTHO_TOLERANCE = 1e-5
# Entities with no bounding radius in the tags: weapons (Function Weapon, Kind Weapon) and projectiles.
ZERO_RADIUS_KINDS = (ObjectKind.Weapon, ObjectKind.Projectile)
# |BoundingCenter - WorldOrigin| - BoundingRadius of an entity that is not bound to a parent. Measured at most 3.40 (cgb, a vehicle), 0.05 in most films.
BOUNDS_MARGIN = 3.5
# Lowest Vitality of a dead entity: -22.22 in suelo (death by a long fall, 9441), a fraction can go below 0 when the damage is larger.
VITALITY_FLOOR = -23.0

NEGATIVE_VITALITY_KINDS = (ObjectKind.Biped, ObjectKind.Vehicle, ObjectKind.Crate)

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
    negative = known & (vitality < 0)
    s.values["negative_vitality_rows"] = int(negative.sum())
    s.Flag("entity_negative_vitality_unexpected", negative & (active | ~np.isin(E["Kind"], NEGATIVE_VITALITY_KINDS)), ticks, h, vitality)
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
    for name in ("SelectedGrenade", "FragGrenades", "PlasmaGrenades", "IsAbilityActive", "TicksAlive", "AbilityEnergy", "Shield"):
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
    holds_objective = P["Objective"] != NO_HANDLE
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
# The general rules
# ---------------------------------------------------------------------------------------------------------------------------------

def _Names(prefix, names, suffix=""):
    return tuple(prefix + name + suffix for name in names)


# Rules of Findings that hold in any recording: no constant measured in a film, no list of films. Left out on purpose: the ranges measured in the films
# (shield, grenades, camera distance, bounds margin, vitality floor, lift distance and Affects), the enums measured in them (Connection, Team) and the
# counts that depend on a film (event objects gone, hits without a drop, ticks alive steps, loadout not with the biped).
GENERAL_RULES = (
    _Names("header_count_", ("PlayerCount", "EntityCount", "ObjectiveCount", "StandCount", "WeaponCount", "ProjectileCount", "ExplosiveCount", "LiftCount")) +
    ("header_flags", "header_last_event_sequence", "table_not_sorted_or_repeated") +
    _Names("entity_unknown_", ("Kind", "Function", "Group", "Lifecycle", "Binding", "Physics")) +
    ("entity_capability_bit_unknown", "entity_handle_repeated_in_a_record", "entity_forward_not_unit", "entity_up_not_unit",
     "entity_forward_not_perpendicular_to_up", "entity_binding_vs_bound_to", "entity_bound_to_vs_parent_handle", "entity_bound_to_not_in_the_tick",
     "entity_carried_is_an_item_of_a_biped", "entity_seated_is_a_biped_of_a_vehicle", "entity_attached_is_the_rest", "link_to_a_removed_entity",
     "entity_radius_not_finite", "entity_radius_not_positive", "entity_vitality_not_finite", "entity_active_vitality_not_a_fraction",
     "entity_dead_vitality_above_zero", "entity_linear_velocity_not_finite", "entity_angular_velocity_not_finite",
     "player_biped_team", "player_alive_vs_body", "player_body_biped_not_alive", "player_no_body_biped_found_but_alive", "player_alive_biped_vitality") +
    _Names("player_no_body_", BODY_HANDLES + ("TicksSinceDamage",)) +
    _Names("player_no_body_", ("SelectedGrenade", "FragGrenades", "PlasmaGrenades", "IsAbilityActive", "TicksAlive", "AbilityEnergy", "Shield"), "_not_zero") +
    ("player_no_body_aim_not_zero",) +
    _Names("player_handle_zero_", ("BipedHandle",) + BODY_HANDLES) +
    ("player_ticks_alive_stands_still", "player_ability_energy_out_of_range", "player_selected_grenade_unknown", "player_ability_active_flag",
     "player_aim_not_unit", "player_camera_forward_not_unit", "player_aim_differs_from_the_camera") +
    _Names("player_", ("PrimaryWeapon", "SecondaryWeapon"), "_not_a_weapon") +
    _Names("player_", ("PrimaryWeapon", "SecondaryWeapon"), "_not_with_the_biped_unexplained") +
    _Names("player_", ("PrimaryWeapon", "SecondaryWeapon"), "_lost") +
    ("player_equipped_is_not_a_slot", "player_ability_not_an_ability", "player_objective_not_a_flag", "player_last_damager_unknown",
     "player_ticks_since_damage_step", "player_ticks_since_damage_forgotten", "player_zoom_unknown", "player_objective_not_carried",
     "flag_state_vs_carrier", "flag_carrier_biped", "flag_carrier_objective", "flag_home_stand_missing", "flag_home_stand_base_team",
     "flag_owner_team_vs_stand", "flag_state_unknown",
     "match_phase_unknown", "match_round_state_unknown", "match_round_index_not_by_one", "match_total_score_decreased", "match_unused_team_score",
     "match_round_score_not_reset", "match_player_score_not_reset",
     "event_sequence_not_continuous", "event_tick_goes_back", "event_kind_unknown", "event_engine_sequence_not_increasing",
     "event_engine_key_not_in_eventkeys", "event_artemis_not_a_flag_event", "event_death_without_a_body_lost", "event_spawn_without_a_body_gained",
     "event_object_unknown_CauseObject", "event_object_unknown_EffectObject", "event_player_unknown_CausePlayer", "event_player_unknown_EffectPlayer",
     "event_flags_not_0_or_1", "lift_stage_axis_not_finite", "lift_stage_changes", "lift_stage_count_too_big")
)
# Rules of the file itself, not of the Findings of the pass.
FILE_RULES = ("no_records", "dropped_ticks", "dump_cut")
ALL_RULES = FILE_RULES + GENERAL_RULES


def Evaluate(dump, label="dump"):
    """(Findings, failures) of a Dump read by tickdump.py. failures lists (rule, count, cases) for every general rule or rule of the file that
    found something, in the order of ALL_RULES. Dropped ticks (DroppedBefore of the records) are a rule: 0 required."""
    if dump.records:
        s = Collect(label, dump)
    else:
        s = Findings(label)
        s.Add("no_records", 0, 0, "the file has no record")
    for r in dump.records:
        if r.dropped:
            s.Add("dropped_ticks", r.game_tick, 0, f"{r.dropped} dropped before the record")
    if dump.footer is not None and int(dump.footer["DroppedTicks"]) and not s.counts["dropped_ticks"]:
        s.Add("dropped_ticks", 0, 0, f"the footer counts {int(dump.footer['DroppedTicks'])}")
    if dump.footer is None:
        s.Add("dump_cut", 0, 0, "no footer: the recording was cut")
    failures = [(name, s.counts[name], list(s.cases[name])) for name in ALL_RULES if s.counts[name]]
    return s, failures

