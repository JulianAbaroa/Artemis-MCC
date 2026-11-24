"""
test_ctf_golden.py
F4.9 (D317): the observation of the policy from the View (Observe of Agent/artemis/env/ctf.py) against the one the environment computed
from the omniscient tick before (Players, Entities, Objectives and Stands), over every tick and every computed View with a body of the
goldens ctf_c and ctf_d (v7). If the View carried the same information, both are the same vector: 1e-4 in what is continuous, equality in
the flags (obs 6, 11, 12, 13 and 18) and in whether the shaping potential exists. The count of comparisons and the worst error by entry are
printed (pytest -s). A film without golden.tdump skips its test.
"""
import math
import os
import sys

import numpy as np
import pytest

import tickdump
from tdump_writer import Repo
from test_ctf import ctf  # noqa: F401  (the fixture: artemis.env.ctf with a stand-in for the bridge)

sys.path.insert(0, os.path.join(Repo(), "Agent"))

slow = pytest.mark.slow
FILMS = ("ctf_c", "ctf_d")
TOLERANCE = 1e-4
FLAG_ENTRIES = (6, 11, 12, 13, 18)
NO_HANDLE = 0xFFFFFFFF


def GoldenPath(film):
    folder = os.environ.get("ARTEMIS_RECORDINGS") or os.path.join(Repo(), "Ignore", "Regresion")
    return os.path.join(folder, film, "golden.tdump")


def ReferenceObserve(ctf, players, table, objectives, stands, match, player, last_action, nvec, max_seconds):
    """Reference of the F4.9, not used by the environment: a copy of CtfEnv._observe as it was before the View (Players, Entities,
    Objectives and Stands of the omniscient tick). Returns (obs, phi)."""
    def Entity(handle):
        rows = table[table["Handle"] == handle]
        return rows[0] if len(rows) else None

    def SideOf(team):
        owned = stands[stands["OwnerTeam"] == team]
        return int(owned["BaseTeam"][0]) if len(owned) else None

    def AimAngles(p):
        aim = p["Aim"]
        x, y, z = float(aim["X"]), float(aim["Y"]), float(aim["Z"])
        return math.atan2(y, x) % (2 * math.pi), math.atan2(z, math.hypot(x, y))

    def TimeLeft():
        cap = max_seconds * 60
        left, horizon = cap - int(match["RoundClockTicks"]), cap
        remaining, limit = int(match["RoundTicksRemaining"]), int(match["RoundTimeLimitTicks"])
        if remaining >= 0:
            left = min(left, remaining)
            if limit > 0:
                horizon = min(horizon, limit)
        return min(1.0, max(0.0, left / horizon)) if horizon > 0 else 0.0

    def ScoreLeft(team):
        to_win = int(match["ScoreToWin"])
        if to_win <= 0:
            return 0.0, 0.0
        scores = match["TeamRoundScores"]
        enemies = [int(t) for t in stands["OwnerTeam"] if int(t) != team]

        def Left(t):
            if t is None or not 0 <= t < len(scores):
                return 0.0
            return max(0, to_win - int(scores[t])) / to_win
        return Left(team), Left(enemies[0] if enemies else None)

    obs = np.zeros(ctf.STATE_SIZE + sum(nvec), dtype=np.float32)
    offset = ctf.STATE_SIZE
    for branch, n in zip(last_action, nvec):
        obs[offset + int(branch)] = 1.0
        offset += n
    obs[19] = TimeLeft()
    obs[20:22] = ScoreLeft(int(player["Team"]))
    if not (int(player["IsAlive"]) and int(player["HasBody"])):
        return obs, None
    body = Entity(int(player["BipedHandle"]))
    if body is None:
        return obs, None
    side = SideOf(int(player["Team"]))
    position = ctf.Vec(body["WorldOrigin"])
    yaw, pitch = AimAngles(player)
    cos, sin = math.cos(-yaw), math.sin(-yaw)

    def Local(v):
        return np.array([cos * v[0] - sin * v[1], sin * v[0] + cos * v[1], v[2]], dtype=np.float32)

    def PositionOf(handle):
        entity = Entity(int(handle))
        return ctf.Vec(entity["WorldOrigin"]) if entity is not None else None

    enemy_flag = own_flag = own_stand = None
    if side is not None:
        for flag in objectives:
            if int(flag["BaseTeam"]) == side:
                own_flag = flag
            else:
                enemy_flag = flag
        for stand in stands:
            if int(stand["BaseTeam"]) == side:
                own_stand = stand
    is_carrying = int(player["Objective"]) != NO_HANDLE
    obs[0:3] = Local(ctf.Vec(body["LinearVelocity"])) / 10.0
    obs[3] = pitch
    vitality = float(body["Vitality"])
    obs[4] = 0.0 if math.isnan(vitality) else vitality
    obs[5] = float(player["Shield"])
    obs[6] = float(is_carrying)
    phi = None
    stand_position = PositionOf(own_stand["Handle"]) if own_stand is not None else None
    flag_position = PositionOf(enemy_flag["Handle"]) if enemy_flag is not None else None
    if flag_position is not None:
        to_flag = flag_position - position
        obs[7:10] = Local(to_flag) / ctf.METERS
        obs[10] = np.linalg.norm(to_flag) / ctf.METERS
        flag_state = int(enemy_flag["State"])
        obs[11] = float(flag_state == ctf.FlagState.Home)
        obs[12] = float(flag_state == ctf.FlagState.Dropped)
        obs[13] = float(is_carrying)
        if stand_position is not None:
            if is_carrying:
                phi = -np.linalg.norm(stand_position - position) / ctf.METERS
            else:
                phi = -(np.linalg.norm(to_flag) + np.linalg.norm(stand_position - flag_position)) / ctf.METERS
    if stand_position is not None:
        to_stand = stand_position - position
        obs[14:17] = Local(to_stand) / ctf.METERS
        obs[17] = np.linalg.norm(to_stand) / ctf.METERS
    if own_flag is not None:
        obs[18] = float(int(own_flag["State"]) == ctf.FlagState.Home)
    return obs, (float(phi) if phi is not None else None)


def Compare(ctf, film, known):
    """Observe from the View against the reference over the film. known: skip the two classes of difference that the F4.9 found (HALLAZGO
    F4.9-1 and F4.9-2, see below). Returns (comparisons, worst error by entry, the cases that differ, the counts of each class)."""
    nvec = ctf.ActionNvec("stage0")
    last = np.array([2, 1, 4, 2, 0, 0])
    worst = np.zeros(ctf.STATE_SIZE)
    count, classes, cases = 0, {"carried": 0, "no_own_flag": 0}, []
    for r, table in tickdump.Dump(GoldenPath(film)).states():
        for row, view in enumerate(r.views):
            head = view["Head"]
            if not int(head["IsComputed"]) or not int(head["HasBody"]):
                continue
            expected, expected_phi = ReferenceObserve(ctf, r.players, table, r.objectives, r.stands, r.match, r.players[row], last, nvec, 120.0)
            got, phi = ctf.Observe(view, r.match, last, nvec, 120.0)
            count += 1
            skip, skip_phi = [], False
            flags = view["Flags"]
            # HALLAZGO F4.9-1: the View puts a carried flag at the position of its carrier (Position of Flags, CONTRACT), the old observation
            # at the flag entity, which hangs from the carrier up to about a metre away: obs 7-10 (and the potential, if the agent is not the
            # carrier) differ by up to 0.02 (1 m / METERS).
            if int(head["Side"]) != ctf.NEUTRAL_TEAM and int(flags[1]["State"]) == 1 and ctf.IsFlag(flags[1]):
                classes["carried"] += 1
                skip += [7, 8, 9, 10]
                skip_phi = skip_phi or not int(view["Self"]["CarriesFlag"])
            # HALLAZGO F4.9-2: while the own flag does not exist (the engine recreates it after a capture or a return) Flags[0] is all zero and
            # the View has no StandPosition: obs 14-17 and the potential are lost (the reference finds the stand in Stands).
            if int(head["Side"]) != ctf.NEUTRAL_TEAM and not ctf.IsFlag(flags[0]):
                classes["no_own_flag"] += 1
                skip += [14, 15, 16, 17]
                skip_phi = True
            error = np.abs(got[:ctf.STATE_SIZE].astype(np.float64) - expected[:ctf.STATE_SIZE])
            worst = np.maximum(worst, error)
            bad = [i for i in range(ctf.STATE_SIZE) if (error[i] != 0 if i in FLAG_ENTRIES else error[i] > TOLERANCE) and not (known and i in skip)]
            if not (known and skip_phi) and ((phi is None) != (expected_phi is None) or (phi is not None and abs(phi - expected_phi) > TOLERANCE)):
                bad.append(-1)
            if bad:
                cases.append((r.game_tick, row, [(i, float(got[i]) if i >= 0 else phi, float(expected[i]) if i >= 0 else expected_phi) for i in bad[:3]]))
    return count, worst, cases, classes


def Report(film, count, worst, cases, classes):
    print(f"\n{film}: {count} comparisons, {len(cases)} different, classes {classes}; worst error by entry:")
    print("  " + " ".join(f"{i}:{w:.1e}" for i, w in enumerate(worst)))


@slow
@pytest.mark.parametrize("film", FILMS)
def test_the_observation_from_the_view_is_the_one_of_the_omniscient_tick_but_for_two_known_classes(ctf, film, capsys):
    if not os.path.isfile(GoldenPath(film)):
        pytest.skip(f"{film} has no golden.tdump")
    count, worst, cases, classes = Compare(ctf, film, known=True)
    with capsys.disabled():
        Report(film, count, worst, cases, classes)
    assert count > 0 and classes["carried"] > 0 and classes["no_own_flag"] > 0
    assert not cases, f"{len(cases)} differences outside the known classes; first: {cases[:3]} (tick, row, [(entry, view, reference)])"


@slow
@pytest.mark.xfail(strict=True, reason="HALLAZGO F4.9-1 and F4.9-2 of the report: the View carries a carried flag at its carrier and no stand while the own "
                                       "flag does not exist; Opus decides whether that is a View bug (it passes when both are fixed)")
@pytest.mark.parametrize("film", FILMS)
def test_the_observation_from_the_view_is_exactly_the_one_of_the_omniscient_tick(ctf, film):
    if not os.path.isfile(GoldenPath(film)):
        pytest.skip(f"{film} has no golden.tdump")
    count, worst, cases, classes = Compare(ctf, film, known=False)
    assert count > 0
    assert not cases
