"""
test_control_check.py
Tests of the pure part of `bridge_check.py controls` (VW.9c.3 E1): the table of phases, the AgentInput of a phase, the reading of a
Sample from a published tick and the verdict of every check (PASS, FAIL and SKIP of its precondition) with samples made by hand,
plus the imitation check. No MCC: the bridge client is only used for the order of its CONTROLS.
"""
import math
import os
import sys
from types import SimpleNamespace

import numpy as np
import pytest

from conftest import Repo
from test_bridge_setup import modules  # noqa: F401  (fixture: artemis.bridge and artemis.bridge_layout with a stand-in for kernel32)

sys.path.insert(0, os.path.join(Repo(), "Agent"))
from artemis import control_check as cc  # noqa: E402


def S(**kw):
    return cc.Sample(**kw)


def Run(name, samples, before, context="foot", **extras):
    return cc.CheckPhase(cc._BY_NAME[name], samples, before, context, extras)


def Walk(n, step, yaw=0.0, base=(0.0, 0.0, 0.0)):
    """Samples of an agent that moves `step` m per decision along the direction `yaw`."""
    return [S(pos=(base[0] + step * i * math.cos(yaw), base[1] + step * i * math.sin(yaw), 0.0), yaw=0.0) for i in range(1, n + 1)]


# ------------------------------------------------------------------------------------------------- the table of phases

def test_the_foot_table_has_a_phase_for_every_control_of_the_bridge(modules):  # noqa: F811
    bridge, _ = modules
    used = {name for phase in cc.FOOT for name, _ in phase.controls}
    assert used == set(bridge.CONTROLS)
    assert len(bridge.CONTROLS) == 13


def test_every_context_names_phases_that_exist_and_every_check_exists():
    for context, names in cc.CONTEXTS.items():
        assert names and all(n in cc._BY_NAME for n in names), context
    for phase in cc.FOOT:
        for context in cc.CONTEXTS:
            assert cc.CheckName(phase, context) in cc.CHECKS


def test_phases_of_filters_by_name_and_refuses_unknown_names():
    assert [p.name for p in cc.PhasesOf("foot", ["use"])] == ["use_edge", "use_level"]
    assert [p.name for p in cc.PhasesOf("foot", ["jump", "reload"])] == ["jump", "reload"]
    with pytest.raises(ValueError):
        cc.PhasesOf("vehicle", ["jump"])


def test_the_input_of_a_phase_is_a_tap_on_every_decision_or_a_hold(modules):  # noqa: F811
    controls = modules[0].CONTROLS
    jump, fire = cc._BY_NAME["jump"], cc._BY_NAME["fire"]
    for first in (True, False):
        assert cc.PhaseInput(jump, first, controls)["edge"] == 1 and cc.PhaseInput(jump, first, controls)["level"] == 0
    bit = 1 << controls.index("Fire")
    assert (cc.PhaseInput(fire, True, controls)["level"], cc.PhaseInput(fire, True, controls)["edge"]) == (bit, bit)
    assert (cc.PhaseInput(fire, False, controls)["level"], cc.PhaseInput(fire, False, controls)["edge"]) == (bit, 0)
    walk = cc.PhaseInput(cc._BY_NAME["forward_ability"], False, controls)
    assert walk["forward"] == 1.0 and walk["level"] == 1 << controls.index("Ability")
    assert cc.PhaseInput(None, True, controls) == {"forward": 0.0, "left": 0.0, "yaw_delta": 0.0, "pitch_delta": 0.0, "level": 0, "edge": 0}


def test_a_sample_is_read_from_a_published_tick(modules):  # noqa: F811
    _, layout = modules
    players = np.zeros(1, dtype=layout.BridgePlayer)
    players["Handle"], players["BipedHandle"], players["IsAlive"], players["HasBody"] = 0x10005, 0x20001, 1, 1
    players["EquippedWeapon"], players["FragGrenades"], players["Zoom"] = 0x30001, 2, 1
    players["Aim"]["X"], players["Aim"]["Y"] = 0.0, 1.0
    ents = np.zeros(2, dtype=layout.BridgeEntity)
    ents["Handle"] = (0x20001, 0x40001)
    ents[0]["WorldOrigin"]["X"], ents[0]["Binding"], ents[0]["BoundTo"] = 3.0, cc.SEATED, 0x40001
    ents[1]["WorldOrigin"]["X"] = 7.0
    weapons = np.zeros(1, dtype=layout.BridgeWeapon)
    weapons["Handle"], weapons["MagazineCount"], weapons["Energy"] = 0x30001, 2, math.nan
    weapons["Magazines"]["Loaded"][0] = (12, 5)
    weapons["Magazines"]["Reserve"][0] = (30, 0)
    shots = np.zeros(2, dtype=layout.BridgeProjectile)
    shots["Handle"], shots["OwnerPlayer"], shots["InHand"] = (0x50001, 0x50002), 0x10005, (0, 1)
    objectives = np.zeros(1, dtype=layout.BridgeObjective)
    objectives["CarrierPlayer"] = 0x10005
    published = SimpleNamespace(players=players, entities=ents, weapons=weapons, projectiles=shots, objectives=objectives,
                                action_tick=8, possessed_player=0x10005)
    s = cc.SampleOf(published, 0x10005)
    assert (s.tick, s.alive, s.zoom, s.frag, s.possessed) == (8, True, 1, 2, True)
    assert math.isclose(s.yaw, math.pi / 2) and s.pos[0] == 3.0
    assert (s.loaded, s.reserve, s.energy) == (17, 30, 0.0)
    assert s.projectiles == {0x50001} and s.carrying
    assert s.vehicle == 0x40001 and s.vehicle_pos[0] == 7.0
    assert cc.SampleOf(published, 0x99) is None


# ------------------------------------------------------------------------------------------------------------- the checks

def test_quiet():
    before = S()
    assert Run("quiet", [S(pos=(0.1, 0, 0), yaw=0.001)] * 4, before).status == "PASS"
    assert Run("quiet", [S(pos=(0.1, 0, 0), yaw=0.2)] * 4, before).status == "FAIL"   # turned
    assert Run("quiet", [S(pos=(1.0, 0, 0))] * 4, before).status == "FAIL"            # moved
    # note: with more samples than STILL_SETTLE the first ones are a slide from the phase before and do not count.
    slide = [S(pos=(0.1 * i, 0, 0)) for i in range(8)] + [S(pos=(0.8, 0, 0))] * 10
    assert Run("quiet", slide, before).status == "PASS"


def test_forward_and_left_are_projected_on_the_look():
    assert Run("forward", Walk(5, 1.0), S()).status == "PASS"
    assert Run("forward", Walk(5, 0.1), S()).status == "FAIL"
    assert Run("forward", Walk(5, 1.0, yaw=math.pi / 2), S()).status == "FAIL"          # walked to the left, not forward
    assert Run("left", Walk(5, 1.0, yaw=math.pi / 2), S()).status == "PASS"
    assert Run("left", Walk(5, 1.0, yaw=math.pi / 2), S(yaw=math.pi / 2)).status == "FAIL"  # looking left, walking left = back


def Turning(axis, per, n=12, start=0.0):
    out = []
    for i in range(1, n + 1):
        out.append(S(yaw=per * i) if axis == "yaw" else S(pitch=start + per * i))
    return out


def test_yaw_and_pitch_turn_what_was_asked_each_decision():
    assert Run("yaw", Turning("yaw", 0.1), S()).status == "PASS"
    assert Run("yaw", Turning("yaw", 0.3), S()).status == "FAIL"
    assert Run("yaw", Turning("yaw", 0.1, 2), S()).status == "SKIP"
    assert Run("pitch_up", Turning("pitch", 0.05), S()).status == "PASS"
    assert Run("pitch_down", Turning("pitch", 0.05), S()).status == "FAIL"             # the sign is wrong
    assert Run("pitch_down", Turning("pitch", -0.05, start=0.0), S()).status == "PASS"


def test_pitch_that_reaches_the_engine_stop_is_not_a_fail():
    stuck = [S(pitch=cc.PITCH_STOP + 0.05)] * 12
    result = Run("pitch_up", stuck, S(pitch=cc.PITCH_STOP))
    assert result.status == "PASS" and "stop" in result.detail
    mixed = Turning("pitch", 0.05, 8, start=1.2) + [S(pitch=1.5)] * 6
    assert Run("pitch_up", mixed, S(pitch=1.2)).status != "FAIL"


def test_jump_counts_the_ground_flanks():
    air = [S(), S(grounded=False, airborne=3), S(), S(airborne=2), S(grounded=False)]
    result = Run("jump", air, S())
    assert result.status == "PASS" and result.detail.startswith("2 jumps")
    assert Run("jump", [S()] * 5, S()).status == "FAIL"


def test_crouch_needs_most_samples_crouched():
    assert Run("crouch", [S(crouching=True)] * 4 + [S()], S()).status == "PASS"
    assert Run("crouch", [S(crouch_fraction=0.95)] * 5, S()).status == "PASS"
    assert Run("crouch", [S(crouching=True)] + [S()] * 4, S()).status == "FAIL"


def test_fire_looks_at_the_magazine_or_the_heat_and_energy():
    gun = S(equipped=7, loaded=12, reserve=40)
    assert Run("fire", [S(equipped=7, loaded=12), S(equipped=7, loaded=9)], gun).status == "PASS"
    assert Run("fire", [S(equipped=7, loaded=12)] * 3, gun).status == "FAIL"
    assert Run("fire", [S(equipped=7)] * 3, S(equipped=7, loaded=0)).status == "SKIP"
    assert Run("fire", [S()] * 3, S()).status == "SKIP"
    beam = S(equipped=7, is_energy=True, heat=0.0, energy=1.0)
    assert Run("fire", [S(equipped=7, is_energy=True, heat=0.2, energy=1.0)], beam).status == "PASS"
    assert Run("fire", [S(equipped=7, is_energy=True, heat=0.0, energy=0.8)], beam).status == "PASS"
    assert Run("fire", [S(equipped=7, is_energy=True, heat=0.0, energy=1.0)], beam).status == "FAIL"


def test_zoom_checks():
    # note: zoom NO_ZOOM (-1, 255 on the bridge) is no zoom; 0 is the first level (K1 2026-11-23).
    assert Run("zoom", [S(zoom=-1), S(zoom=0)], S()).status == "PASS"
    assert Run("zoom", [S(zoom=-1)] * 3, S()).status == "SKIP"
    assert Run("zoom_in", [S(zoom=0)], S()).status == "PASS"
    assert Run("zoom_in", [S(zoom=1)], S(zoom=0)).status == "PASS"
    assert Run("zoom_in", [S(zoom=0)] * 3, S(zoom=0)).status == "SKIP"
    assert Run("zoom_out", [S(zoom=0), S(zoom=-1)], S(zoom=1)).status == "PASS"
    assert Run("zoom_out", [S(zoom=0)] * 3, S(zoom=1)).status == "FAIL"
    assert Run("zoom_out", [S(zoom=-1)] * 3, S()).status == "SKIP"


def test_ability_and_forward_ability():
    ability = S(ability=3, ability_energy=1.0)
    assert Run("ability", [S(ability_active=True)], ability).status == "PASS"
    assert Run("ability", [S(ability=3, ability_energy=0.7)], ability).status == "PASS"
    assert Run("ability", [S(ability=3, ability_energy=1.0)], ability).status == "FAIL"
    assert Run("ability", [S()] * 3, S()).status == "SKIP"
    sprint = S(ability=3, ability_energy=1.0, ability_kind=1)
    assert Run("ability", [sprint] * 3, sprint).status == "SKIP"   # sprint standing still: forward_ability covers it
    both = Walk(5, 1.0)
    for sample in both:
        sample.ability_active = True
    assert Run("forward_ability", both, ability).status == "PASS"
    assert Run("forward_ability", Walk(5, 0.0), ability).status == "FAIL"
    assert Run("forward_ability", Walk(5, 1.0), S()).status == "SKIP"


def test_throw_grenade_in_foot_and_in_nogren():
    two = S(frag=1, plasma=1)
    assert Run("throw_grenade", [S(frag=0, plasma=1)], two).status == "PASS"
    assert Run("throw_grenade", [S(frag=1, plasma=1, projectiles=frozenset({9}))], two).status == "PASS"
    assert Run("throw_grenade", [S(frag=1, plasma=1)] * 3, two).status == "FAIL"
    assert Run("throw_grenade", [S()] * 3, S()).status == "SKIP"
    # nogren: nothing may come out
    assert Run("throw_grenade", [S()] * 3, S(), "nogren").status == "PASS"
    assert Run("throw_grenade", [S(projectiles=frozenset({9}))], S(), "nogren").status == "FAIL"
    assert Run("switch_grenade", [S(grenade=0)] * 3, S(grenade=0), "nogren").status == "PASS"
    assert Run("switch_grenade", [S(grenade=1)] * 3, S(grenade=0), "nogren").status == "FAIL"


def test_switch_weapon_and_switch_grenade():
    two = S(primary=1, secondary=2, equipped=1)
    assert Run("switch_weapon", [S(primary=1, secondary=2, equipped=2)], two).status == "PASS"
    assert Run("switch_weapon", [two] * 3, two).status == "FAIL"
    assert Run("switch_weapon", [S(primary=1, equipped=1)] * 3, S(primary=1, equipped=1)).status == "SKIP"
    both = S(frag=2, plasma=1, grenade=0)
    assert Run("switch_grenade", [S(frag=2, plasma=1, grenade=1)], both).status == "PASS"
    assert Run("switch_grenade", [both] * 3, both).status == "FAIL"
    assert Run("switch_grenade", [S(frag=2)] * 3, S(frag=2)).status == "SKIP"


def test_reload_with_and_without_reserve():
    low = S(loaded=3, reserve=20)
    pre = [S(loaded=12, reserve=20), S(loaded=3, reserve=20)]
    assert Run("reload", [S(loaded=3, reserve=20, reloading=True)], low, pre=pre).status == "PASS"
    assert Run("reload", [S(loaded=12, reserve=14)], low, pre=pre).status == "PASS"
    assert Run("reload", [low] * 3, low, pre=pre).status == "FAIL"
    assert Run("reload", [S(loaded=3)] * 3, S(loaded=3, reserve=0), pre=pre).status == "SKIP"
    assert Run("reload", [S(loaded=12, reserve=20)] * 3, S(loaded=12, reserve=20), pre=[S(loaded=12, reserve=20)] * 2).status == "SKIP"


def test_melee_and_use_are_see():
    for name in ("melee", "use_edge", "use_level"):
        assert Run(name, [S()] * 3, S()).status == "SEE"
    result = Run("use_edge", [S(primary=5)] * 3, S(primary=4))
    assert result.status == "SEE" and any("primary" in n for n in result.notes)


def test_dying_is_a_skip_not_a_fail():
    assert Run("forward", [S(), S(alive=False)], S()).status == "SKIP"
    assert Run("fire", [S(has_body=False)], S(equipped=1, loaded=5)).detail == "died"
    assert Run("forward", [], S()).status == "SKIP"


def test_a_flag_that_is_lost_is_data_not_a_fail():
    held = S(carrying=True)
    result = Run("jump", [S(carrying=True), S(carrying=False, grounded=False, airborne=2)], held, "flag")
    assert result.status == "PASS" and any(n.startswith("DATO") and "flag" in n for n in result.notes)
    assert Run("jump", [S(carrying=False, grounded=False)], held, "foot").notes == []
    assert Run("jump", [S(carrying=True, grounded=False)], held, "flag").notes == []


def test_vehicle_context_moves_with_forward_and_the_rest_is_see():
    seated = S(vehicle=9, vehicle_pos=(0.0, 0.0, 0.0))
    far = [S(vehicle=9, vehicle_pos=(4.0, 0.0, 0.0))]
    assert Run("forward", far, seated, "vehicle").status == "PASS"
    assert Run("forward", [seated] * 3, seated, "vehicle").status == "FAIL"
    assert Run("forward", far, S(), "vehicle").status == "SKIP"
    assert Run("yaw", [seated] * 3, seated, "vehicle").status == "SEE"
    assert Run("use_level", [seated] * 3, seated, "vehicle").status == "SEE"


def test_bridge_counters_that_grow_are_named():
    before = {"RejectedActions": 0, "LockstepTimeouts": 1, "HandleMismatches": 0}
    assert cc.GrewCounters(before, dict(before)) == []
    assert cc.GrewCounters(before, {**before, "RejectedActions": 2, "LockstepTimeouts": 1}) == ["RejectedActions"]


def test_the_summary_and_the_line():
    results = [cc.Result("PASS"), cc.Result("PASS"), cc.Result("SEE"), cc.Result("SKIP", "x", ["DATO: y"])]
    assert cc.Summarize(results) == {"PASS": 2, "FAIL": 0, "SKIP": 1, "SEE": 1}
    line = cc.FormatLine("foot", "jump", results[-1])
    assert line.splitlines()[0].split()[:3] == ["foot", "jump", "SKIP"] and "DATO: y" in line


# ----------------------------------------------------------------------------------------------------------- imitation

def Window(controls, level=(), edge=(), **axes):
    w = {"forward": 0.0, "left": 0.0, "yaw_delta": 0.0, "pitch_delta": 0.0, "level": 0, "edge": 0}
    w.update(axes)
    for name in level:
        w["level"] |= 1 << controls.index(name)
    for name in edge:
        w["edge"] |= 1 << controls.index(name)
    return w


def test_imitation_checks_bits_axes_and_other_bits(modules):  # noqa: F811
    c = modules[0].CONTROLS
    jump, crouch, yaw = cc._BY_NAME["jump"], cc._BY_NAME["crouch"], cc._BY_NAME["yaw"]
    ok = cc.CheckImitation(jump, [None, Window(c, edge=["Jump"])], c)
    assert ok.status == "PASS" and ok.notes == []
    assert cc.CheckImitation(jump, [Window(c)], c).status == "FAIL"                       # the bit is missing
    other = cc.CheckImitation(jump, [Window(c, edge=["Jump", "Melee"])], c)
    assert other.status == "PASS" and any("Melee" in n for n in other.notes)           # other bits are data
    assert cc.CheckImitation(crouch, [Window(c, edge=["Crouch"])], c).status == "FAIL"    # held: Level is missing too
    assert cc.CheckImitation(crouch, [Window(c, level=["Crouch"], edge=["Crouch"])], c).status == "PASS"
    assert cc.CheckImitation(yaw, [Window(c, yaw_delta=0.2), Window(c, yaw_delta=0.1)], c).status == "PASS"
    assert cc.CheckImitation(yaw, [Window(c, yaw_delta=-0.2)], c).status == "FAIL"        # sign
    assert cc.CheckImitation(cc._BY_NAME["left"], [Window(c, left=1.0)], c).status == "PASS"


def test_imitation_without_possession_is_a_skip_and_quiet_is_data(modules):  # noqa: F811
    c = modules[0].CONTROLS
    assert cc.CheckImitation(cc._BY_NAME["jump"], [None, None], c).status == "SKIP"
    assert cc.CheckImitation(cc._BY_NAME["quiet"], [Window(c)], c).status == "PASS"
    touched = cc.CheckImitation(cc._BY_NAME["quiet"], [Window(c, edge=["Use"])], c)
    assert touched.status == "SEE" and any("Use" in n for n in touched.notes)


def test_the_possessed_window_is_read_from_the_header(modules):  # noqa: F811
    _, layout = modules
    header = np.zeros(1, dtype=layout.BridgeTickHeader)[0]
    header["PossessedPlayer"] = 0x10001
    header["PossessedInput"]["Level"], header["PossessedInput"]["MoveForward"] = 4, 1.0
    published = SimpleNamespace(possessed_player=0x10001, possessed_input=header["PossessedInput"])
    assert cc.WindowOf(published)["level"] == 4 and cc.WindowOf(published)["forward"] == 1.0
    assert cc.WindowOf(SimpleNamespace(possessed_player=cc.NO_HANDLE, possessed_input=None)) is None
