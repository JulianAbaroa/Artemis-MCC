"""
test_setups.py
Tests of the match setup keys of the bench scenarios (LS.2, D286): how Tools/game.py compiles the preset, the map and the roster of a
match into one setup at load, and "setups" into one setup per next round.
"""
import pytest

import game


def compile_one(match, defaults=None, configs=None):
    scenario = {"matches": [match]}
    if defaults is not None:
        scenario["defaults"] = defaults
    return game.Compile(scenario, configs)[0]


def test_preset_map_and_roster_of_a_match_are_the_setup_at_load():
    text = compile_one({"config": "c", "preset": "slayer", "map": "reach bus", "roster": {"0": 7, "1": 8}})
    assert "config c\npreset slayer\nmap reach bus\nroster 7 8 0 0 0 0 0 0 auto 0\n" in text


def test_none_in_the_setup_at_load_is_written_as_none():
    text = compile_one({"config": "c", "preset": None, "map": None})
    assert "preset none\nmap none\n" in text


def test_the_bots_shortcut_is_part_of_the_setup_at_load():
    text = compile_one({"config": "c", "preset": "slayer", "bots": [0, 1, "auto"]})
    assert "preset slayer\nroster 1 1 0 0 0 0 0 0 auto 1\n" in text


def test_setups_are_one_line_per_round_in_order():
    text = compile_one({"config": "c", "setups": [
        {"preset": "slayer", "map": "reach bus", "roster": {"0": 1, "auto": 2}},
        {"preset": None},
        {"roster": {"1": 4}},
    ]})
    assert [line for line in text.splitlines() if line.startswith("round_")] == [
        "round_setup preset=slayer; map=reach bus; roster=1 0 0 0 0 0 0 0 auto 2",
        "round_setup preset=none",
        "round_setup roster=0 4 0 0 0 0 0 0 auto 0"]


def test_a_field_left_out_of_a_setup_is_not_written():
    text = compile_one({"config": "c", "setups": [{"map": "reach bus"}]})
    assert "round_setup map=reach bus\n" in text
    assert "preset=" not in text and "roster=" not in text


def test_setups_do_not_need_a_roster_at_load():
    text = compile_one({"config": "c", "setups": [{"preset": "slayer"}]})
    assert "round_setup preset=slayer\n" in text


def test_rosters_become_setups_that_only_change_the_roster():
    by_rosters = compile_one({"config": "c", "roster": {"0": 2}, "rosters": [{"0": 1}, {"1": 3}]})
    by_setups = compile_one({"config": "c", "roster": {"0": 2}, "setups": [{"roster": {"0": 1}}, {"roster": {"1": 3}}]})
    assert by_rosters == by_setups


def test_setups_and_rosters_together_are_an_error():
    with pytest.raises(ValueError, match="setups and rosters"):
        compile_one({"config": "c", "roster": {"0": 1}, "setups": [{"preset": "a"}], "rosters": [{"0": 1}]})


def test_setups_and_rosters_together_across_the_defaults_are_an_error():
    with pytest.raises(ValueError, match="setups and rosters"):
        compile_one({"config": "c", "roster": {"0": 1}, "rosters": [{"0": 1}]}, defaults={"setups": [{"preset": "a"}]})


@pytest.mark.parametrize("setup", [{}, {"preset": "Bad Name"}, {"map": ""}, {"roster": {"8": 1}}, {"colour": "red"}, "slayer"])
def test_a_wrong_setup_is_an_error(setup):
    with pytest.raises(ValueError):
        compile_one({"config": "c", "setups": [setup]})


def test_a_map_that_is_not_recorded_is_an_error_in_a_setup_too():
    with pytest.raises(ValueError, match="not a recorded config"):
        compile_one({"config": "c", "setups": [{"map": "nowhere"}]}, configs={"c": 1})


def test_setups_must_be_a_list():
    with pytest.raises(ValueError, match="setups must be a list"):
        compile_one({"config": "c", "setups": {"preset": "a"}})


def test_setup_check_goes_with_its_match():
    text = compile_one({"config": "c", "setups": [{"preset": "a"}], "checks": ["no_errors", "setup_ok"]})
    assert "check setup_ok\n" in text
