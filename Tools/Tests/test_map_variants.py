"""
test_map_variants.py
Tests of the map variant key of the bench scenarios (Q98, D266): how Tools/game.py compiles "map" into the line format of BenchUIService.
"""
import pytest

import game


def test_game_compile_writes_the_map_key_after_the_preset_of_its_match():
    scenario = {"name": "q", "matches": [
        {"config": "c", "map": "Reach Bus Mountain Pass Reach Bus CTF", "preset": "artemis_ctf_base",
         "checks": [["map_applied", "Reach Bus Mountain Pass Reach Bus CTF"]]},
        {"config": "c", "map": "none"}, {"config": "c", "map": None}, {"config": "c"}]}
    text, count = game.Compile(scenario, None)
    assert count == 4
    assert "config c\npreset artemis_ctf_base\nmap Reach Bus Mountain Pass Reach Bus CTF\n" in text
    assert "check map_applied Reach Bus Mountain Pass Reach Bus CTF\n" in text
    assert text.count("map none\n") == 2 and text.count("\nmap ") == 3


def test_game_compile_takes_the_map_from_the_defaults():
    text, _ = game.Compile({"defaults": {"map": "b"}, "matches": [{"config": "c"}, {"config": "c", "map": None}]}, None)
    assert text.count("map b\n") == 1 and text.count("map none\n") == 1


@pytest.mark.parametrize("name", ["", " lead", "trail ", "bad/name", "x" * 65, 3])
def test_game_compile_refuses_a_map_that_is_not_a_config_name(name):
    with pytest.raises(ValueError, match="map"):
        game.Compile({"matches": [{"config": "c", "map": name}]}, None)


def test_game_compile_refuses_a_map_that_is_not_a_recorded_config():
    with pytest.raises(ValueError, match="not a recorded config"):
        game.Compile({"matches": [{"config": "a", "map": "b"}]}, ["a"])
    text, _ = game.Compile({"matches": [{"config": "a", "map": "a"}, {"config": "a", "map": "none"}]}, ["a"])
    assert "map a\n" in text and "map none\n" in text
