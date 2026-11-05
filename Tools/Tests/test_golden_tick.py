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

import tickchecks
import tickdump
from tdump_writer import Repo

sys.path.insert(0, os.path.join(Repo(), "Agent"))
sys.path.insert(0, os.path.join(Repo(), "Tools"))
from artemis import bridge_layout as bl  # noqa: E402
from artemis import semantic as sem  # noqa: E402
from tickchecks import (
    BODY_HANDLES,
    BOUNDS_MARGIN,
    FIRST_TICKS_ALIVE,
    GrenadeType,
    LIFT_STAGE_DISTANCE_MAX,
    NOT_LOCAL,
    ORTHO_TOLERANCE,
    VITALITY_FLOOR,
    ZoomLevel,
)  # noqa: E402

FILMS = ("armas_a", "armas_b", "cgb_displace_1min", "ctf_c", "ctf_d", "explosivos", "lifts", "powerups", "suelo")


def RecordingsFolder():
    """ARTEMIS_RECORDINGS if it is set, else Ignore/Regresion (the rule of Tests.Support.FilmPass)."""
    return os.environ.get("ARTEMIS_RECORDINGS") or os.path.join(Repo(), "Ignore", "Regresion")




# ---------------------------------------------------------------------------------------------------------------------------------
# The pass of one film
# ---------------------------------------------------------------------------------------------------------------------------------


@functools.lru_cache(maxsize=None)
def Survey(film):
    """Findings of the film, or None when it has no golden.tdump."""
    path = os.path.join(RecordingsFolder(), film, "golden.tdump")
    if not os.path.isfile(path):
        return None
    return tickchecks.Collect(film, tickdump.Dump(path))



# ---------------------------------------------------------------------------------------------------------------------------------
# Analysis of what the pass gathered
# ---------------------------------------------------------------------------------------------------------------------------------

# Vitality goes below 0 once the entity is dead (damage beyond 0): only Dead bipeds, vehicles (and turrets) and crates (explosives) show it. Rows measured.
NEGATIVE_VITALITY_ROWS = {"armas_a": 1164, "armas_b": 230, "cgb_displace_1min": 1485, "ctf_c": 0, "ctf_d": 0, "explosivos": 814, "lifts": 910, "powerups": 0, "suelo": 152}


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
# Steps of TicksAlive that are not +1 in a tick: (tick, step). suelo 8248 is the tick the biped leaves the Warthog seat (8032-8248). It is a counter of
# the engine, not a bug: +2 at leaving a seat (cgb 2146, 2821 too).
TICKS_ALIVE_STEPS = {"suelo": [(8248, 2)], "cgb_displace_1min": [(2146, 2), (2821, 2)]}
# First TicksAlive of a body that is not 1: the player that appears in the last records of cgb (GameTick 3579 three times, D174).
FIRST_TICKS_ALIVE_ODD = {"cgb_displace_1min": [(3579, 0)]}
# Hits (TicksSinceDamage 0) after which neither the shield nor the health dropped in the next HIT_WINDOW_TICKS ticks.
HITS_WITHOUT_A_DROP = {"cgb_displace_1min": 6}
# Rows of PrimaryWeapon and SecondaryWeapon that point to a weapon that is not with the biped: the player carries the flag (the flag is the
# equipped weapon and the weapons are on the floor), is seated in a vehicle, or it is the tick of a capture. Not a bug: the engine keeps the weapons.
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
# (GameTick of the record, Phase, RoundState, RoundIndex) at each change of any of the three. The Phase is InGame from the start until the GameOver event of the
# engine and Ended from the record that reads it (armas_a 17224, ctf_c 32671, ctf_d 15981: the event is stamped one tick earlier, GAME_OVER_READ_LAG_TICKS).
# The end of a round (RoundState Ended, Between) and the death of the local player (ctf_c 4281) do not change it: the raw phase of the engine does, so it
# is not read (HALLAZGO 4, fixed). ctf_c 11203 is the tick after the capture of 11202, 11564 the one after the recreation of 11563.
MATCH_TRANSITIONS = {
    "armas_a": [
        (1, INGAME, PLAY, 0), (17224, ENDED, OVER, 0),
    ],
    "armas_b": [
        (1, INGAME, PLAY, 0),
    ],
    "cgb_displace_1min": [
        (0, INGAME, PLAY, 0),
    ],
    "ctf_c": [
        (1, INGAME, PLAY, 0), (11203, INGAME, OVER, 0), (11503, INGAME, BETWEEN, 1), (11563, INGAME, PLAY, 1), (23159, INGAME, OVER, 1),
        (23459, INGAME, BETWEEN, 2), (23519, INGAME, PLAY, 2), (32671, ENDED, OVER, 2),
    ],
    "ctf_d": [
        (1, INGAME, PLAY, 0), (7801, INGAME, OVER, 0), (8101, INGAME, BETWEEN, 1), (8161, INGAME, PLAY, 1), (12356, INGAME, OVER, 1),
        (12656, INGAME, BETWEEN, 2), (12716, INGAME, PLAY, 2), (15981, ENDED, OVER, 2),
    ],
    "explosivos": [
        (1, INGAME, PLAY, 0),
    ],
    "lifts": [
        (1, INGAME, PLAY, 0),
    ],
    "powerups": [
        (1, INGAME, PLAY, 0),
    ],
    "suelo": [
        (1, INGAME, PLAY, 0),
    ],
}
# The record that reads the GameOver event is this many ticks after the GameTick of the event (measured 1 in the 3 films that end).
GAME_OVER_READ_LAG_TICKS = 1
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
    0 or less: the fraction goes negative with the damage beyond 0 that killed it (suelo and lifts -22.2, a fall), only for Dead bipeds, vehicles and turrets and
    explosive crates (NEGATIVE_VITALITY_ROWS), never an Active entity. The biped of a player with Body has more than 0."""
    s = Film(film)
    for name in ("entity_vitality_not_finite", "entity_active_vitality_not_a_fraction", "entity_dead_vitality_above_zero", "entity_vitality_below_the_floor",
                 "player_alive_biped_vitality", "entity_negative_vitality_unexpected"):
        s.Clean(name)
    assert s.values["negative_vitality_rows"] == NEGATIVE_VITALITY_ROWS[film]
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
    body, GroundObject and TicksSinceDamage are NoHandle, Zoom is 0xFF (none, test_player_zoom_without_body_is_none) and the numbers of the body are zero, BipedHandle is NoHandle (before the first spawn: the engine
    stores 0 and Artemis reads it as NoHandle, F2.A), the dead biped that the table still has, or the handle of a biped that is gone."""
    s = Film(film)
    s.Clean("player_alive_vs_body")
    s.Clean("player_body_biped_not_alive")
    s.Clean("player_no_body_biped_found_but_alive")
    for name in BODY_HANDLES + ("TicksSinceDamage",):
        s.Clean("player_no_body_" + name)
    for name in ("SelectedGrenade", "FragGrenades", "PlasmaGrenades", "IsAbilityActive", "TicksAlive", "AbilityEnergy", "Shield"):
        s.Clean("player_no_body_" + name + "_not_zero")
    s.Clean("player_no_body_aim_not_zero")
    s.Clean("player_handle_zero_BipedHandle")
    kinds = s.values["no_body_biped"]
    assert kinds["no_handle"] + kinds["dead"] + kinds["gone"] == s.values["player_rows"] - s.values["body_rows"]
    assert s.values["body_rows"] > 0


@ALL
def test_player_handles_are_never_zero(film):
    """Covers: Player.Objective.
    No handle of a player row is 0: a handle of salt 0 is never a datum (NoHandle is 0xFFFFFFFF). The engine writes 0 in the objective (cgb 3579, the first tick of
    a body) and the loadout handles when there is none: the core reads it as NoHandle (D270)."""
    s = Film(film)
    for name in ("BipedHandle",) + BODY_HANDLES:
        s.Clean("player_handle_zero_" + name)


@ALL
def test_player_zoom_without_body_is_none(film):
    """Covers: Player.Zoom.
    Zoom says no zoom (0xFF) for a player without Body, as with Body and no zoom (ZoomLevel None), not the 0 of Zoom1."""
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
    carries the flag, is seated in a vehicle or in the tick of a capture (LOADOUT_NOT_WITH_THE_BIPED: the engine keeps the weapons then, not a bug; any other
    row fails). EquippedWeapon is NoHandle, the Primary,
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


@ALL
def test_match_phase_ends_with_the_game_over_event(film):
    """Covers: Match.Phase.
    The Phase is InGame from the first record until the GameOver event of the engine (one in armas_a, ctf_c and ctf_d, none in the others) and Ended from
    the record that reads it on: not from the end of a round, not at the death of the local player (ctf_c 4281), and never Unknown (the engine globals are captured)."""
    s = Film(film)
    M = s.matches
    over = [int(e["GameTick"]) for e in s.EV if e["Kind"] == sem.Event.GameOver]
    assert len(over) == (1 if film in ("armas_a", "ctf_c", "ctf_d") else 0)
    ended = M["Phase"] == ENDED
    assert (M["Phase"] != UNKNOWN_PHASE).all()
    if not over:
        assert not ended.any()
        return
    start = over[0] + GAME_OVER_READ_LAG_TICKS
    assert (ended == (M["GameTick"] >= start)).all()
    assert int(M["GameTick"][ended].min()) == start


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
