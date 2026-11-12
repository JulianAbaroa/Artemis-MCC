"""
test_rosters.py
Tests of the roster keys of the bench scenarios (LS.1, D284): how Tools/game.py compiles "roster", "bots" and "rosters" (a shortcut of
"setups", D286) into the line format of BenchUIService.
"""
import pytest

import game


def compile_one(match, defaults=None):
    scenario = {"matches": [match]}
    if defaults is not None:
        scenario["defaults"] = defaults
    return game.Compile(scenario, None)[0]


def test_roster_is_written_as_the_counts_of_the_eight_teams_then_auto():
    text = compile_one({"config": "c", "roster": {"0": 7, "1": 8, "auto": 0}})
    assert "config c\nroster 7 8 0 0 0 0 0 0 auto 0\n" in text


def test_auto_alone_and_the_last_team_are_accepted():
    text = compile_one({"config": "c", "roster": {"7": 2, "auto": 3}})
    assert "roster 0 0 0 0 0 0 0 2 auto 3\n" in text


def test_bots_is_a_shortcut_that_counts_the_bots_of_each_team():
    text = compile_one({"config": "c", "bots": [0, 1, 1, "auto", None, 0]})
    assert "roster 2 2 0 0 0 0 0 0 auto 2\n" in text
    assert "\nbot " not in text


def test_bots_and_roster_in_the_same_match_are_an_error():
    with pytest.raises(ValueError, match="cannot go together"):
        compile_one({"config": "c", "bots": [0], "roster": {"0": 1}})


def test_bots_and_roster_in_the_defaults_are_an_error():
    with pytest.raises(ValueError, match="cannot go together"):
        compile_one({"config": "c"}, defaults={"bots": [0], "roster": {"0": 1}})


def test_the_roster_of_a_match_replaces_the_bots_of_the_defaults_and_the_other_way_round():
    text = compile_one({"config": "c", "roster": {"1": 4}}, defaults={"bots": [0]})
    assert "roster 0 4 0 0 0 0 0 0 auto 0\n" in text
    text = compile_one({"config": "c", "bots": [0]}, defaults={"roster": {"1": 4}})
    assert "roster 1 0 0 0 0 0 0 0 auto 0\n" in text


def test_a_null_roster_removes_the_roster_of_the_defaults():
    text = compile_one({"config": "c", "roster": None}, defaults={"bots": [0]})
    assert "roster" not in text


def test_rosters_are_one_setup_line_per_round_in_order():
    text = compile_one({"config": "c", "roster": {"0": 7, "1": 8},
                        "rosters": [{"0": 1, "auto": 2}, {"0": 5, "1": 6}, {"1": 1}]})
    lines = text.splitlines()
    assert lines.index("roster 7 8 0 0 0 0 0 0 auto 0") < lines.index("round_setup roster=1 0 0 0 0 0 0 0 auto 2")
    assert [line for line in lines if line.startswith("round_")] == [
        "round_setup roster=1 0 0 0 0 0 0 0 auto 2", "round_setup roster=5 6 0 0 0 0 0 0 auto 0", "round_setup roster=0 1 0 0 0 0 0 0 auto 0"]
    assert "round_roster" not in text


def test_rosters_need_a_roster_at_load():
    with pytest.raises(ValueError, match="rosters need a roster"):
        compile_one({"config": "c", "rosters": [{"0": 1}]})


def test_roster_check_goes_with_its_match():
    text = compile_one({"config": "c", "roster": {"0": 1}, "checks": ["no_errors", "roster_ok"]})
    assert "check roster_ok\n" in text


@pytest.mark.parametrize("roster", [{"8": 1}, {"red": 1}, {"0": -1}, {"0": 1.5}, {"0": True}, [1, 2], "7 8"])
def test_a_wrong_roster_is_an_error(roster):
    with pytest.raises(ValueError, match="roster"):
        compile_one({"config": "c", "roster": roster})


@pytest.mark.parametrize("bots", [[8], [-1], ["red"]])
def test_a_wrong_bot_team_is_an_error(bots):
    with pytest.raises(ValueError, match="bots"):
        compile_one({"config": "c", "bots": bots})
