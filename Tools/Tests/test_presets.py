"""
test_presets.py
Tests of the gametype presets (Q97, D264, D265): the catalog of the fields of the game variant, the compiler Tools/presets.py (fields, bits,
traits, indices, inheritance, errors with file:line, --check) and the fixture shared with the C++ tests of Blam.Layout.
The fixture (Artemis/7_Tests/Tests/Blam/Fixtures/presets) is what the compiler gives for Tools/Tests/presets/fixture.xml with the real
catalog. After a change of the catalog or of the format regenerate it:
    python Tools/presets.py compile --presets Tools/Tests/presets --catalog Data/Presets/Catalog.xml --eol repo --out Artemis/7_Tests/Tests/Blam/Fixtures/presets
"""
import math
import os
import re
import shutil
import subprocess
import sys

import pytest

from tdump_writer import Repo

import game
import presets

REPO = Repo()
TOOL = os.path.join(REPO, "Tools", "presets.py")
CATALOG = os.path.join(REPO, "Data", "Presets", "Catalog.xml")
SOURCES = os.path.join(REPO, "Tools", "Tests", "presets")
FIXTURE = os.path.join(REPO, "Artemis", "7_Tests", "Tests", "Blam", "Fixtures", "presets")
GAME_OFFSET = os.path.join(REPO, "Artemis", "2_Blam", "Layout", "Game", "Type", "GameOffset.ixx")
PLAYER_TRAITS = os.path.join(REPO, "Artemis", "2_Blam", "Layout", "Player", "Type", "PlayerTraits.ixx")


def Catalog():
    return presets.Catalog(CATALOG)


def Folder(tmp_path, **files):
    """A folder with the real catalog and the given presets (name -> XML text)."""
    folder = tmp_path / f"presets{len(list(tmp_path.iterdir()))}"
    folder.mkdir()
    shutil.copy(CATALOG, folder / "Catalog.xml")
    for name, text in files.items():
        (folder / (name + ".xml")).write_text(text, encoding="utf-8")
    return str(folder)


def Preset(body, base=""):
    return f'<?xml version="1.0"?>\n<preset{f" base={chr(34)}{base}{chr(34)}" if base else ""}>\n{body}\n</preset>\n'


def Writes(text):
    """{(offset, mask hex): (when, bytes hex)} of a compiled preset."""
    out = {}
    for line in text.splitlines():
        words = line.split()
        if words[0] == "write":
            out[(int(words[1], 16), words[4])] = (words[2], words[3])
    return out


def Compiled(tmp_path, **files):
    folder = Folder(tmp_path, **files)
    compiled, errors, _ = presets.Compile(folder)
    return compiled, errors


def Cli(*args):
    return subprocess.run([sys.executable, TOOL, *args], capture_output=True, text=True, errors="replace",
                          env=dict(os.environ, PYTHONDONTWRITEBYTECODE="1"))


# --- catalog ------------------------------------------------------------------------------------------------------------------

def test_the_catalog_parses_and_every_field_has_a_when_and_a_known_enum():
    catalog = Catalog()
    assert {"general", "respawn", "respawn_traits", "social", "map", "base_traits", "red_traits", "blue_traits", "yellow_traits",
            "teams", "team", "loadout", "trait_set", "megalo"} <= set(catalog.Groups)
    assert catalog.Enums["weapon"]["rocket_launcher"] == 8
    for group, values, field, offset, when in catalog.Expanded:
        assert when in presets.WHENS
        assert offset + field.Size <= 0xF842, (group.Name, field.Name)
    # 6 traits blocks plus the 16 trait sets, each with the same subfields
    traits = len(catalog.Groups["base_traits"].Fields)
    assert sum(1 for g, *_ in catalog.Expanded if g.Name == "trait_set") == 16 * traits


def test_catalog_offsets_match_the_variant_constants_of_blam_layout():
    with open(GAME_OFFSET, encoding="utf-8") as handle:
        constants = dict((name, int(value, 16)) for name, value in re.findall(r"(k_Variant\w+)\{ (0x[0-9A-Fa-f]+) \}", handle.read()))
    names = {"k_VariantRoundTimeLimit": ("general", "time_limit_minutes"), "k_VariantRoundLimit": ("general", "round_limit"),
             "k_VariantRoundsToWin": ("general", "rounds_to_win"), "k_VariantSuddenDeath": ("general", "sudden_death_seconds"),
             "k_VariantGracePeriod": ("general", "grace_period_seconds"), "k_VariantLivesPerRound": ("respawn", "lives_per_round"),
             "k_VariantTeamLivesPerRound": ("respawn", "team_lives_per_round"), "k_VariantRespawnTime": ("respawn", "time_seconds"),
             "k_VariantPowerupDuration": ("map", "red_duration"), "k_VariantScoreToWin": ("megalo", "score_to_win"),
             "k_VariantMegaloTraitSets": ("trait_set", None)}
    catalog = Catalog()
    for constant, (group, field) in names.items():
        assert constant in constants, constant
        entry = catalog.Groups[group]
        assert constants[constant] == entry.Offset + (entry.ByName[field].Offset if field else 0), constant
    assert catalog.Groups["trait_set"].Indices == [("index", 16, constants["k_VariantMegaloTraitSetStride"])]


def test_no_two_fields_overlap_in_offset_and_mask(tmp_path):
    text = open(CATALOG, encoding="utf-8").read().replace('<field name="friendly_fire" offset="0x2FC" type="bit" bit="0"',
                                                           '<field name="friendly_fire" offset="0x2FC" type="bit" bit="1"')
    path = tmp_path / "Catalog.xml"
    path.write_text(text, encoding="utf-8")
    with pytest.raises(presets.PresetError, match=r"Catalog.xml:\d+: field 'social.betrayal_booting' overlaps"):
        presets.Catalog(str(path))


def PercentTable(name):
    with open(PLAYER_TRAITS, encoding="utf-8") as handle:
        text = handle.read()
    body = re.search(rf"{name}\{{ ([^}}]*) \}}", text).group(1)
    return [float("inf") if item.strip() == "k_Infinity" else float(item.strip().rstrip("f")) for item in body.split(",")]


def test_the_enums_of_the_catalog_match_the_decode_tables_of_player_traits():
    tables = {"damage_resist": "k_DamageResistanceTable", "health_multiplier": "k_HealthShieldMultiplierTable",
              "health_rate": "k_HealthRegenTable", "shield_rate": "k_ShieldRegenTable", "vampirism_rate": "k_VampirismTable",
              "damage_multiplier": "k_DamageMultiplierTable", "movement_speed": "k_MovementSpeedTable",
              "player_gravity": "k_GravityTable", "radar_range": "k_RadarRangeTable"}
    enums = Catalog().Enums
    for enum, table in tables.items():
        values = PercentTable(table)
        by_value = {value: name for name, value in enums[enum].items()}
        assert max(by_value) == len(values), enum
        for raw, expected in enumerate(values, start=1):
            name = by_value[raw]
            if name in ("invulnerable", "instant_kill"):
                assert math.isinf(expected), (enum, name)
            elif name.endswith("%"):
                assert float(name[:-1]) / 100 == pytest.approx(expected), (enum, name)
            else:
                assert name.endswith("m") and float(name[:-1]) == pytest.approx(expected), (enum, name)


# --- compiler -----------------------------------------------------------------------------------------------------------------

def test_simple_field_bit_traits_subfield_and_numbers(tmp_path):
    compiled, errors = Compiled(tmp_path, a=Preset('<general round_limit="0x5" /><social friendly_fire="1" />'
                                                  '<base_traits gravity="75%" jump_height="150" />'))
    assert errors == []
    writes = Writes(compiled["a.txt"])
    assert writes[(0x2BF, "ff")] == ("round_end", "05")
    assert writes[(0x2FC, "01")] == ("tick", "01")
    assert writes[(0x304 + 0x19, "ff")] == ("tick", "02")
    assert writes[(0x304 + 0x1C, "ffffffff")] == ("tick", "96000000")
    assert len(writes) == 4


def test_bit_writes_only_its_mask_and_a_wide_bit_field_uses_its_width(tmp_path):
    compiled, errors = Compiled(tmp_path, a=Preset('<map powerups="0" turrets="true" /><teams species="all_elites" />'))
    assert errors == []
    writes = Writes(compiled["a.txt"])
    assert writes[(0x300, "08")] == ("round_start", "00")
    assert writes[(0x300, "10")] == ("round_start", "10")
    assert writes[(0x3BC, "07")] == ("tick", "02")


def test_indices_team_loadout_and_trait_set(tmp_path):
    compiled, errors = Compiled(tmp_path, a=Preset('<team index="1" primary_color="0x00FF00" text_color="-1" />'
                                                  '<loadout palette="2" index="3" primary="rocket_launcher" ability="none" />'
                                                  '<trait_set index="0" movement_speed="90%" />'))
    assert errors == []
    writes = Writes(compiled["a.txt"])
    assert writes[(0x3C0 + 0x58 + 0x48, "ffffffff")] == ("load", "00ff0000")
    assert writes[(0x3C0 + 0x58 + 0x50, "ffffffff")] == ("load", "ffffffff")
    assert writes[(0x681 + 2 * 30 + 3 * 6 + 2, "ff")] == ("spawn", "08")
    assert writes[(0x681 + 2 * 30 + 3 * 6 + 4, "ff")] == ("spawn", "ff")
    assert writes[(0x5DD0 + 0x18, "ff")] == ("tick", "05")
    assert "check megalo_trait_set 0\n" in compiled["a.txt"]


def test_two_levels_of_inheritance_the_child_overrides_field_by_field(tmp_path):
    compiled, errors = Compiled(
        tmp_path,
        grand=Preset('<general round_limit="3" rounds_to_win="2" /><respawn time_seconds="7" />'),
        mid=Preset('<respawn time_seconds="5" lives_per_round="1" />', base="grand"),
        leaf=Preset('<respawn time_seconds="3" />', base="mid"))
    assert errors == []
    writes = Writes(compiled["leaf.txt"])
    assert writes[(0x2C7, "ff")][1] == "03"
    assert writes[(0x2C5, "ff")][1] == "01"
    assert writes[(0x2BF, "ff")][1] == "03"
    assert writes[(0x2BD, "ff")][1] == "02"
    assert len(writes) == 4
    source = {name: re.search(r"source (\w+)", compiled[name + ".txt"]).group(1) for name in ("grand", "mid", "leaf")}
    assert len(set(source.values())) == 3


def test_a_number_and_an_enum_name_give_the_same_bytes(tmp_path):
    compiled, errors = Compiled(tmp_path, a=Preset('<map weapon_set="rocket_launchers" />'),
                                b=Preset('<map weapon_set="3" />'), c=Preset('<map weapon_set="0x3" />'))
    assert errors == []
    assert Writes(compiled["a.txt"]) == Writes(compiled["b.txt"]) == Writes(compiled["c.txt"])
    negative, _ = Compiled(tmp_path, a=Preset('<map weapon_set="-2" />'))
    assert Writes(negative["a.txt"])[(0x330, "ffff")][1] == "feff"


@pytest.mark.parametrize("body, line, text", [
    ('<nothing a="1" />', 3, "unknown element <nothing>"),
    ('<general nothing="1" />', 3, "unknown attribute 'nothing'"),
    ('<base_traits gravity="85%" />', 3, "'85%' is not a valid value for 'gravity'"),
    ('\n<general round_limit="256" />', 4, "outside the range of 'round_limit'"),
    ('<team primary_color="1" />', 3, "needs the attribute 'index'"),
    ('<team index="8" primary_color="1" />', 3, "from 0 to 7"),
    ('<general round_limit="1" /><general rounds_to_win="1" />', 3, "written twice"),
])
def test_errors_name_the_file_and_the_line(tmp_path, body, line, text):
    compiled, errors = Compiled(tmp_path, bad=Preset(body), good=Preset('<general round_limit="1" />'))
    assert len(errors) == 1 and f"bad.xml:{line}: " in errors[0] and text in errors[0], errors
    assert "bad.txt" not in compiled and "good.txt" in compiled


def test_missing_base_cycle_and_bad_file_names(tmp_path):
    compiled, errors = Compiled(tmp_path, orphan=Preset("", base="nowhere"), one=Preset("", base="two"), two=Preset("", base="one"))
    text = "\n".join(errors)
    assert "orphan.xml:2: base 'nowhere' does not exist" in text
    assert "has a cycle (one -> two -> one)" in text or "has a cycle (two -> one -> two)" in text
    assert [f for f in compiled if f != "_catalog.txt"] == []
    folder = Folder(tmp_path)
    shutil.copy(os.path.join(SOURCES, "fixture_parent.xml"), os.path.join(folder, "Bad-Name.xml"))
    shutil.copy(os.path.join(SOURCES, "fixture_parent.xml"), os.path.join(folder, "x" * 32 + ".xml"))
    _, errors, _ = presets.Compile(folder)
    assert len(errors) == 2 and all("is not a valid preset name" in e for e in errors)


def test_the_compiled_catalog_lists_every_field_of_the_catalog():
    catalog = Catalog()
    lines = catalog.Text().splitlines()
    assert len(lines) == len(catalog.Expanded)
    assert "field 2c7 1 ff spawn" in lines and "field 2fc 1 01 tick" in lines and "field 460 4 ffffffff load" in lines
    assert all(re.fullmatch(r"field [0-9a-f]+ [124] [0-9a-f]+ (tick|spawn|pickup|round_start|round_end|load)", l) for l in lines)


def test_the_real_presets_compile():
    compiled, errors, info = presets.Compile()
    assert errors == []
    assert info["artemis_ctf_fast"][0] == "artemis_ctf_base"
    assert len(Writes(compiled["artemis_ctf_fast.txt"])) > len(Writes(compiled["artemis_ctf_base.txt"]))


# --- --check and the shared fixture -------------------------------------------------------------------------------------------

def test_check_is_zero_when_equal_and_one_when_the_xml_changes(tmp_path):
    folder = Folder(tmp_path, a=Preset('<general round_limit="1" />'))
    out = str(tmp_path / "out")
    assert Cli("compile", "--presets", folder, "--out", out).returncode == 0
    assert Cli("--check", "--presets", folder, "--out", out).returncode == 0
    with open(os.path.join(folder, "a.xml"), "w", encoding="utf-8") as handle:
        handle.write(Preset('<general round_limit="2" />'))
    result = Cli("--check", "--presets", folder, "--out", out)
    assert result.returncode == 1 and "a.txt: differs from the XML" in result.stdout
    assert not os.path.exists(os.path.join(out, "b.txt"))


def test_the_compiler_gives_exactly_the_shared_fixture(tmp_path):
    shutil.copy(CATALOG, tmp_path / "Catalog.xml")
    for name in os.listdir(SOURCES):
        shutil.copy(os.path.join(SOURCES, name), tmp_path / name)
    compiled, errors, _ = presets.Compile(str(tmp_path))
    assert errors == []
    for name, text in compiled.items():
        with open(os.path.join(FIXTURE, name), "rb") as handle:
            assert handle.read().replace(b"\r\n", b"\n").rstrip(b"\n") == text.encode("ascii").rstrip(b"\n"), name
    assert Cli("--check", "--presets", SOURCES, "--catalog", CATALOG, "--out", FIXTURE).returncode == 0


# --- bench key and example presets ----------------------------------------------------------------------------------------------

def test_the_colors_preset_writes_only_load_fields_on_top_of_its_base():
    compiled, errors, info = presets.Compile()
    assert errors == [] and info["artemis_ctf_colors"][0] == "artemis_ctf_base"
    base, colors = Writes(compiled["artemis_ctf_base.txt"]), Writes(compiled["artemis_ctf_colors.txt"])
    extra = {key: value for key, value in colors.items() if key not in base}
    assert extra == {(0x3C0 + 0x48, "ffffffff"): ("load", "00ff0000"), (0x3C0 + 0x58 + 0x48, "ffffffff"): ("load", "0080ff00")}
    assert all(colors[key] == value for key, value in base.items())


def test_game_compile_writes_the_preset_key_after_the_config_of_its_match():
    scenario = {"name": "q", "matches": [
        {"config": "c", "preset": "artemis_ctf_fast", "checks": [["preset_applied", "artemis_ctf_fast"]]},
        {"config": "c", "preset": "none"}, {"config": "c", "preset": None}, {"config": "c"}]}
    text, count = game.Compile(scenario, None)
    assert count == 4
    assert "config c\npreset artemis_ctf_fast\n" in text and "check preset_applied artemis_ctf_fast\n" in text
    assert text.count("preset none\n") == 2 and text.count("\npreset ") == 3
    assert game.presets is presets


@pytest.mark.parametrize("preset", ["Bad", "with space", "x" * 32, "", 3])
def test_game_compile_refuses_a_preset_that_is_not_a_name(preset):
    with pytest.raises(ValueError, match="preset"):
        game.Compile({"matches": [{"config": "c", "preset": preset}]}, None)

