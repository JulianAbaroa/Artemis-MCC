"""
test_training.py
Tests of the training lists (LS.2, D285, D286): the catalog of the maps (Data/Training/Maps.json), load_list and its checks (map, roster,
rounds, weight, order, preset that is not the compiled one any more), the sequence and weighted executors, and save_list.
"""
import json
import os
import shutil
import sys

import pytest

from tdump_writer import Repo

AGENT = os.path.join(Repo(), "Agent")
if AGENT not in sys.path:
    sys.path.insert(0, AGENT)

from artemis import training  # noqa: E402
from artemis.training import ListError, StaleListError, TrainingEntry, TrainingList, load_catalog, load_list, save_list  # noqa: E402

PRESETS = os.path.join(Repo(), "Data", "Presets")
EXAMPLE = os.path.join(Repo(), "Data", "Training", "ctf_curriculo_inicial.json")


@pytest.fixture
def presets(tmp_path):
    """A copy of Data/Presets the test can change."""
    folder = tmp_path / "presets"
    shutil.copytree(PRESETS, folder)
    return str(folder)


def Entry(**kwargs):
    entry = {"map": "Map CTF 1v1 First", "preset": None, "roster": {"auto": 1, "rival": 1}, "rounds": 1, "weight": 1.0}
    entry.update(kwargs)
    return entry


def Write(tmp_path, entries, order="sequence", presets=None, name="t"):
    path = tmp_path / "list.json"
    path.write_text(json.dumps({"name": name, "order": order, "entries": entries, "presets": presets or {}}))
    return str(path)


def test_the_catalog_has_the_26_maps_with_unique_names_and_valid_sizes():
    maps = load_catalog()
    assert len(maps) == 26
    assert len({info.name for info in maps.values()}) == 26
    assert {info.stage for info in maps.values()} == {"First", "Final"}
    assert all(info.size[1] == "v" and info.size[0] == info.size[2] for info in maps.values())
    assert maps["Map Kings Rest 8v8"].name == "King's Rest (8v8)"
    assert maps["Map CTF 1v1 First"].stage == "First"
    assert [info.stage for info in maps.values() if info.size in ("4v4", "6v6", "8v8")] == ["Final"] * 23


def test_the_names_of_the_catalog_are_the_ones_of_the_reference_file():
    reference = os.path.join(Repo(), "Ignore", "Referencia", "Mapas_Entrenamiento.txt")
    if not os.path.exists(reference):
        pytest.skip("the reference file is not in this checkout")
    text = open(reference, encoding="utf-8").read()
    for config in load_catalog():
        assert config in text


def test_a_catalog_with_a_repeated_config_or_a_bad_size_is_refused(tmp_path):
    good = {"config": "Map A 1v1", "name": "A", "size": "1v1", "stage": "First"}
    for rows, message in (([good, dict(good, name="B")], "repeats"), ([good, dict(good, config="Map B 1v1")], "repeats"),
                          ([dict(good, size="big")], "size"), ([dict(good, stage="Middle")], "stage"), ([{"config": "x"}], "keys")):
        path = tmp_path / "maps.json"
        path.write_text(json.dumps(rows))
        with pytest.raises(ValueError, match=message):
            load_catalog(str(path))


def test_the_example_list_loads_and_is_the_curriculum():
    result = load_list(EXAMPLE)
    assert result.order == "sequence" and len(result.entries) == 3
    assert [entry.map for entry in result.entries] == ["Map CTF 1v1 First", "Map CTF 1v1 First", "Map CTF 2v2 Final"]
    assert all(entry.preset == "artemis_ctf_rounds" for entry in result.entries)
    assert [entry.roster for entry in result.entries] == [{"rival": 1}, {"auto": 1, "rival": 1}, {"auto": 2, "rival": 2}]


def test_a_valid_list_loads(tmp_path):
    result = load_list(Write(tmp_path, [Entry(rounds=2), Entry(map="Map Vex 8v8", roster={"auto": 7, "rival": 8})]))
    assert [entry.rounds for entry in result.entries] == [2, 1]


@pytest.mark.parametrize("entry, message", [
    ({"map": "Map Nowhere 1v1"}, "not in the catalog"),
    ({"roster": {"auto": 8, "rival": 8}}, "does not fit"),
    ({"roster": {"auto": 1, "rival": 15}}, "does not fit"),
    ({"roster": {"auto": 3}}, "rival of at least 1"),
    ({"roster": {"auto": 3, "rival": 0}}, "rival of at least 1"),
    ({"roster": {}}, "rival of at least 1"),
    ({"roster": {"0": 1, "rival": 1}}, "keys"),
    ({"roster": {"9": 1}}, "keys"),
    ({"roster": {"auto": -1, "rival": 1}}, "at least 0"),
    ({"roster": {"auto": 1.5, "rival": 1}}, "integer"),
    ({"roster": [1, 1]}, "keys"),
    ({"rounds": 0}, "rounds"),
    ({"rounds": 1.5}, "rounds"),
    ({"weight": 0}, "weight"),
    ({"weight": -1.0}, "weight"),
])
def test_an_entry_that_is_not_valid_is_refused(tmp_path, entry, message):
    with pytest.raises(ListError, match=message):
        load_list(Write(tmp_path, [Entry(**entry)]))


def test_the_local_players_count_in_the_16(tmp_path):
    path = Write(tmp_path, [Entry(roster={"auto": 7, "rival": 8})])
    assert load_list(path, locals=1)
    with pytest.raises(ListError, match="does not fit"):
        load_list(path, locals=2)


def test_an_order_that_is_not_valid_or_no_entries_is_refused(tmp_path):
    with pytest.raises(ListError, match="order"):
        load_list(Write(tmp_path, [Entry()], order="random"))
    with pytest.raises(ListError, match="entries"):
        load_list(Write(tmp_path, []))


def test_a_preset_that_changed_since_the_list_was_saved_is_stale(tmp_path, presets):
    path = str(tmp_path / "list.json")
    save_list(path, "t", "sequence", [Entry(preset="artemis_ctf_rounds")], presets_folder=presets)
    assert load_list(path, presets_folder=presets)

    xml = os.path.join(presets, "artemis_ctf_rounds.xml")
    text = open(xml, encoding="utf-8").read().replace('round_limit="4"', 'round_limit="5"')
    open(xml, "w", encoding="utf-8").write(text)
    with pytest.raises(StaleListError, match="lista desactualizada: preset artemis_ctf_rounds"):
        load_list(path, presets_folder=presets)


def test_a_preset_the_list_does_not_hold_is_stale(tmp_path, presets):
    path = Write(tmp_path, [Entry(preset="artemis_ctf_fast")])
    with pytest.raises(StaleListError, match="artemis_ctf_fast"):
        load_list(path, presets_folder=presets)


def test_a_preset_that_does_not_exist_is_an_error(tmp_path, presets):
    with pytest.raises(ListError, match="does not compile"):
        save_list(str(tmp_path / "list.json"), "t", "sequence", [Entry(preset="no_such_preset")], presets_folder=presets)


def test_no_preset_and_the_base_values_need_nothing_saved(tmp_path):
    result = load_list(Write(tmp_path, [Entry(preset=None), Entry(preset="")]))
    assert [entry.preset for entry in result.entries] == [None, ""]
    assert [setup.preset for setup in (result.next_setup(), result.next_setup())] == [None, ""]


def test_sequence_plays_each_entry_for_its_rounds_and_starts_again():
    entries = [TrainingEntry("Map A", rounds=2), TrainingEntry("Map B", rounds=1), TrainingEntry("Map C", rounds=3)]
    result = TrainingList("t", "sequence", entries)
    maps = [result.next_setup().map for _ in range(14)]
    assert maps == ["Map A"] * 2 + ["Map B"] + ["Map C"] * 3 + ["Map A"] * 2 + ["Map B"] + ["Map C"] * 3 + ["Map A"] * 2


def test_the_setup_carries_the_preset_the_map_and_a_copy_of_the_roster():
    entry = TrainingEntry("Map A", "p", {"rival": 1})
    setup = TrainingList("t", "sequence", [entry]).next_setup()
    assert (setup.preset, setup.map, setup.roster) == ("p", "Map A", {"rival": 1})
    setup.roster["rival"] = 9
    assert entry.roster == {"rival": 1}


def test_weighted_with_a_seed_is_reproducible_and_plays_each_draw_for_its_rounds():
    def Run(seed):
        entries = [TrainingEntry("Map A", rounds=2, weight=1.0), TrainingEntry("Map B", rounds=3, weight=3.0)]
        result = TrainingList("t", "weighted", entries, seed=seed)
        return [result.next_setup().map for _ in range(40)]

    assert Run(7) == Run(7)
    assert Run(7) != Run(8)
    runs = Run(7)
    # A draw is played for all its rounds: the runs of equal maps are multiples of the rounds of its entry (a draw may repeat).
    index, lengths = 0, []
    while index < len(runs) - 5:
        length = 1
        while runs[index + length] == runs[index]:
            length += 1
        lengths.append((runs[index], length))
        index += length
    assert all(length % (2 if name == "Map A" else 3) == 0 for name, length in lengths[:-1])


def test_a_weight_of_zero_is_never_drawn():
    entries = [TrainingEntry("Map A", weight=0.0), TrainingEntry("Map B", weight=1.0)]
    result = TrainingList("t", "weighted", entries, seed=1)
    assert {result.next_setup().map for _ in range(500)} == {"Map B"}


def test_weights_shape_the_draws():
    entries = [TrainingEntry("Map A", weight=1.0), TrainingEntry("Map B", weight=9.0)]
    result = TrainingList("t", "weighted", entries, seed=3)
    maps = [result.next_setup().map for _ in range(1000)]
    assert maps.count("Map B") > 800


def test_save_and_load_round_trip(tmp_path, presets):
    path = str(tmp_path / "list.json")
    entries = [Entry(preset="artemis_ctf_rounds", rounds=3), Entry(map="Map Vex 8v8", preset="", roster={"auto": 7, "rival": 8}, weight=2.5),
               Entry(map="Map CTF 2v2 Final", roster=None)]
    saved = save_list(path, "mine", "weighted", entries, presets_folder=presets)
    loaded = load_list(path, seed=5, presets_folder=presets)
    assert loaded.name == "mine" and loaded.order == "weighted" and loaded.seed == 5
    assert loaded.entries == saved.entries
    assert loaded.presets == saved.presets
    assert set(loaded.presets) == {"artemis_ctf_rounds"}
    assert loaded.presets["artemis_ctf_rounds"]["lines"][0] == "preset artemis_ctf_rounds"
    data = json.loads(open(path, encoding="ascii").read())
    assert data["presets"]["artemis_ctf_rounds"]["source"] == loaded.presets["artemis_ctf_rounds"]["source"]


def test_save_refuses_what_load_would_refuse(tmp_path):
    path = str(tmp_path / "list.json")
    with pytest.raises(ListError):
        save_list(path, "t", "sequence", [Entry(map="Map Nowhere 1v1")])
    with pytest.raises(ListError):
        save_list(path, "t", "sometimes", [Entry()])
    assert not os.path.exists(path)


def test_the_package_exports_what_the_scripts_use():
    for name in ("load_list", "save_list", "load_catalog", "TrainingList", "MapInfo", "MAPS_PATH"):
        assert hasattr(training, name)


def test_the_relative_roster_becomes_the_engine_roster_of_the_team_of_the_local_player():
    from artemis.match_setup import is_relative_roster, resolve_roster
    assert resolve_roster({"auto": 2, "rival": 3}, 0) == {"0": 2, "1": 3}
    assert resolve_roster({"auto": 2, "rival": 3}, 1) == {"1": 2, "0": 3}
    assert resolve_roster({"rival": 1}, 0) == {"0": 0, "1": 1}
    assert resolve_roster({"rival": 1}, 1) == {"1": 0, "0": 1}
    with pytest.raises(ValueError):
        resolve_roster({"rival": 1}, 2)
    assert is_relative_roster({"rival": 1}) and not is_relative_roster({"auto": 1}) and not is_relative_roster({"0": 1})
