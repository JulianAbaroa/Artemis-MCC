"""
test_tickdump.py
Tests of Tools/tickdump.py diff and approve (M2, D170-D174; Ignore Referencia/Diseno/15 sec. 6 and 7). Written
by Opus before the code (D161): they fix the interface and the behaviour. They are not changed to pass; if one looks wrong, stop and
report. The input files are written by tdump_writer.py, never by tickdump.py. Updated for D174 (repeated GameTicks).
"""
import datetime

import numpy as np
import pytest

import tickdump
from tdump_writer import ChangedLayout, RealLayout, Writer

H1, H2, H3 = 0xE2340012, 0xE2350013, 0xE2360014
P1, P2 = 0xEC700000, 0xEC710001
NOW = datetime.datetime(2026, 10, 23, 14, 5)
EVENTS = ((1, 2), (2, 6))


def Entity(handle, **fields):
    return {"Handle": handle, "Kind": 2, "Vitality": 1.0, "WorldOrigin": (1.0, 2.0, 3.0), **fields}


def Player(handle, **fields):
    return {"Handle": handle, "Team": 1, "Aim": (1.0, 0.0, 0.0), **fields}


def Event(sequence, tick, **fields):
    return {"Sequence": sequence, "GameTick": tick, "Kind": 3, "CausePlayer": P1, **fields}


def Film(ticks=range(10), entity=None, player=None, events=None, header=None, match=None, extra=None, writer=None,
         **writer_args):
    """A short recording: players P1 and P2, entities H1 and H2 that move one unit per tick, event 1 on tick 2 and event 2 on
    tick 6. entity(tick, handle, row) and player(tick, handle, row) return the row (changed or not) or None to drop it.
    extra(tick) returns more entity rows. events(tick, rows) returns the new events the record of that tick writes; the header
    keeps the EventCount and LastEventSequence of the film (as the BridgeTick does: which record writes an event depends on when
    the recording started, the header does not). header(tick) and match(tick) return fields to change."""
    w = writer or Writer(**writer_args)
    for t in ticks:
        entities = []
        for h in (H1, H2):
            row = Entity(h, WorldOrigin=(float(t), 2.0, 3.0))
            row = entity(t, h, row) if entity else row
            if row is not None:
                entities.append(row)
        entities += extra(t) if extra else []
        players = []
        for h in (P1, P2):
            row = player(t, h, Player(h)) if player else Player(h)
            if row is not None:
                players.append(row)
        new = [Event(s, at) for s, at in EVENTS if at == t]
        new = events(t, new) if events else new
        seen = sum(1 for _, at in EVENTS if at <= t)
        head = {"EventCount": seen, "LastEventSequence": seen, **(header(t) if header else {})}
        w.Tick(t, players=players, entities=entities, events=new, header=head, match=match(t) if match else None)
    return w


def Save(tmp_path, name, writer, **kwargs):
    return writer.Save(tmp_path / name, **kwargs)


def DiffOf(tmp_path, golden, run, overlap=False):
    return tickdump.Diff(tickdump.Dump(Save(tmp_path, "golden.tdump", golden)), tickdump.Dump(Save(tmp_path, "run.tdump", run)),
                         overlap=overlap)


def Changed(field, value, at):
    """entity callback: field = value for the (tick, handle) pairs of at."""
    def Change(t, h, row):
        if (t, h) in at:
            row[field] = value
        return row
    return Change


# --- the test support itself, read back with the reader of F1a (validated in-game) ---------------------------------------------

def test_writer_round_trip_with_the_existing_reader(tmp_path):
    def Entities(t, h, row):
        if h == H2 and t >= 3:
            return None
        return row
    w = Film(range(6), entity=Entities, keyframe_interval=2)
    w.Tick(6, players=[Player(P1)], entities=[Entity(H1), Entity(H3)])
    d = tickdump.Dump(Save(tmp_path, "a.tdump", w))
    assert [r.game_tick for r in d.records] == list(range(7))
    assert d.footer is not None and int(d.footer["RecordedTicks"]) == 7
    handles = [list(table["Handle"]) for _, table in d.states()]
    assert handles == [[H1, H2]] * 3 + [[H1]] * 3 + [[H1, H3]]
    assert [float(table["WorldOrigin"]["X"][0]) for _, table in d.states()][:6] == [0.0, 1.0, 2.0, 3.0, 4.0, 5.0]
    assert [len(r.events) for r in d.records] == [0, 0, 1, 0, 0, 0, 0]


# --- interface ----------------------------------------------------------------------------------------------------------------

def test_tables_and_ignored_fields_exist_in_bridge_xml():
    assert tickdump.TABLES == {"Header": "BridgeTickHeader", "Match": "BridgeMatch", "Players": "BridgePlayer",
                               "Entities": "BridgeEntity", "Events": "BridgeEvent", "Objectives": "BridgeObjective",
                               "Stands": "BridgeStand"}
    assert {t: set(f) for t, f in tickdump.IGNORED.items()} == {
        "Header": {"MatchId", "ActionTick", "Generation", "PublishQpc"},
        "Players": {"LocalIndex", "Connection", "CameraPosition", "CameraForward"}}
    layout, _ = RealLayout()
    fields = {s["name"]: {f[0] for f in s["fields"]} for s in layout["structs"]}
    for table, names in tickdump.IGNORED.items():
        assert set(names) <= fields[tickdump.TABLES[table]]


def test_reader_errors_raise_instead_of_exiting(tmp_path):
    bad = tmp_path / "bad.tdump"
    bad.write_bytes(b"NOTADUMP" + bytes(300))
    with pytest.raises(tickdump.TickDumpError):
        tickdump.Dump(str(bad))
    data = bytearray(Film(range(2)).Bytes())
    data[8] = 3
    other = tmp_path / "v3.tdump"
    other.write_bytes(bytes(data))
    with pytest.raises(tickdump.TickDumpError):
        tickdump.Dump(str(other))


# --- equal --------------------------------------------------------------------------------------------------------------------

def test_a_file_against_itself_is_equal(tmp_path):
    d = tickdump.Dump(Save(tmp_path, "a.tdump", Film()))
    r = tickdump.Diff(d, d)
    assert r.is_equal
    assert (r.tick_range, r.ticks_compared, r.only_golden_ticks, r.only_run_ticks) == ((0, 9), 10, [], [])
    assert (r.schema, r.fields, r.existence) == ([], {}, {})


def test_two_recordings_differ_only_in_ignored_fields(tmp_path):
    golden = Film(header=lambda t: {"Generation": 100 + t, "MatchId": 3, "ActionTick": 0, "PublishQpc": 0})
    run = Film(map_name="other", source="film x", start_time="2026-10-24 09:00:00",
               header=lambda t: {"Generation": 7000 + 2 * t, "MatchId": 9, "ActionTick": t, "PublishQpc": 123456 + t},
               player=lambda t, h, row: {**row, "CameraPosition": (float(t), 5.0, 1.0), "CameraForward": (0.0, 1.0, 0.0),
                                         "LocalIndex": 0, "Connection": 2})
    r = DiffOf(tmp_path, golden, run)
    assert r.is_equal, tickdump.FormatDiff(r)
    assert set(r.ignored) == {"Header.MatchId", "Header.ActionTick", "Header.Generation", "Header.PublishQpc",
                              "Players.LocalIndex", "Players.Connection", "Players.CameraPosition", "Players.CameraForward"}


def test_records_without_match_are_skipped(tmp_path):
    run = Writer()
    run.Tick(0, has_match=False, entities=[Entity(H3)])
    run.Tick(0, has_match=False)
    r = DiffOf(tmp_path, Film(), Film(writer=run))
    assert r.is_equal, tickdump.FormatDiff(r)


def test_an_event_written_by_a_later_record_is_the_same_event(tmp_path):
    def Late(t, rows):
        return [] if t == 2 else ([Event(1, 2)] + rows if t == 3 else rows)
    r = DiffOf(tmp_path, Film(), Film(events=Late))
    assert r.is_equal, tickdump.FormatDiff(r)


def test_empty_tables_compare(tmp_path):
    # A recording with no events, no players and, on some ticks, no entities (menus of the theater, a map without objects).
    def Quiet(**kwargs):
        w = Writer(**kwargs)
        for t in range(6):
            w.Tick(t, entities=[Entity(H1)] if t < 3 else [])
        return w
    r = DiffOf(tmp_path, Quiet(), Quiet(keyframe_interval=2))
    assert r.is_equal, tickdump.FormatDiff(r)
    assert r.ticks_compared == 6


# --- fields -------------------------------------------------------------------------------------------------------------------

def test_an_entity_field_change_is_named_with_ticks_keys_and_first_values(tmp_path):
    golden = Film(entity=lambda t, h, row: {**row, "Vitality": 0.5})
    run = Film(entity=lambda t, h, row: {**row, "Vitality": 0.4875 if (t, h) in {(3, H1), (4, H1), (5, H1), (4, H2)} else 0.5})
    r = DiffOf(tmp_path, golden, run)
    assert not r.is_equal
    assert list(r.fields) == ["Entities.Vitality"]
    f = r.fields["Entities.Vitality"]
    assert (f.ticks, f.keys, f.first_tick, f.first_key, f.golden, f.run) == (3, 2, 3, H1, "0.5", "0.4875")
    lines = tickdump.FormatDiff(r)
    assert "Entities.Vitality: 3 ticks, 2 keys, first tick 3 key E2340012: 0.5 -> 0.4875" in lines
    assert lines[-1].startswith("DIFFERS")


def test_a_change_is_counted_on_every_tick_across_deltas_and_keyframes(tmp_path):
    at = {(t, H2) for t in range(4, 10)}
    r = DiffOf(tmp_path, Film(keyframe_interval=3), Film(entity=Changed("Kind", 7, at), keyframe_interval=3))
    f = r.fields["Entities.Kind"]
    assert (f.ticks, f.keys, f.first_tick, f.first_key, f.golden, f.run) == (6, 1, 4, H2, "2", "7")


def test_floats_compare_by_bits(tmp_path):
    nan = Film(entity=lambda t, h, row: {**row, "Vitality": np.nan})
    assert DiffOf(tmp_path, nan, Film(entity=lambda t, h, row: {**row, "Vitality": np.nan})).is_equal
    zero = Film(entity=lambda t, h, row: {**row, "Vitality": 0.0})
    r = DiffOf(tmp_path, zero, Film(entity=lambda t, h, row: {**row, "Vitality": -0.0 if t == 8 and h == H1 else 0.0}))
    f = r.fields["Entities.Vitality"]
    assert (f.ticks, f.first_tick, f.golden, f.run) == (1, 8, "0", "-0")


def test_a_vector_field_is_reported_by_its_name(tmp_path):
    r = DiffOf(tmp_path, Film(), Film(entity=lambda t, h, row: {**row, "WorldOrigin": (float(t), 2.0, 3.5)} if t == 5 else row))
    assert list(r.fields) == ["Entities.WorldOrigin"]
    assert (r.fields["Entities.WorldOrigin"].ticks, r.fields["Entities.WorldOrigin"].keys) == (1, 2)


def test_player_fields_header_and_match(tmp_path):
    golden = Film(player=lambda t, h, row: {**row, "Shield": 1.0})
    run = Film(player=lambda t, h, row: {**row, "Shield": 0.75 if (t >= 5 and h == P2) else 1.0},
               header=lambda t: {"Flags": 1 if t == 4 else 0},
               match=lambda t: {"Phase": 3 if t >= 7 else 0, "PlayerScores": [1 if (t == 9 and i == 1) else 0 for i in range(16)]})
    r = DiffOf(tmp_path, golden, run)
    assert set(r.fields) == {"Players.Shield", "Header.Flags", "Match.Phase", "Match.PlayerScores"}
    s = r.fields["Players.Shield"]
    assert (s.ticks, s.keys, s.first_tick, s.first_key, s.golden, s.run) == (5, 1, 5, P2, "1", "0.75")
    h = r.fields["Header.Flags"]
    assert (h.ticks, h.keys, h.first_tick, h.first_key) == (1, 0, 4, None)
    m = r.fields["Match.Phase"]
    assert (m.ticks, m.first_tick, m.first_key) == (3, 7, None)
    assert r.fields["Match.PlayerScores"].ticks == 1


def test_events_are_keyed_by_sequence(tmp_path):
    def Changed(t, rows):
        rows = [{**e, "Kind": 9} if e["Sequence"] == 2 else e for e in rows]
        return rows + [Event(3, 8)] if t == 8 else rows
    r = DiffOf(tmp_path, Film(), Film(events=Changed))
    assert set(r.fields) == {"Events.Kind"}
    k = r.fields["Events.Kind"]
    assert (k.ticks, k.keys, k.first_tick, k.first_key, k.golden, k.run) == (1, 1, 6, 2, "3", "9")
    e = r.existence["Events"]
    assert (e.only_golden, e.only_run, e.first_tick, e.first_key, e.first_side) == (0, 1, 8, 3, "run")


# --- existence ----------------------------------------------------------------------------------------------------------------

def test_an_entity_in_one_file_only_is_an_existence_difference(tmp_path):
    w = Film(extra=lambda t: [Entity(H3)] if t in (2, 3) else [])
    r = DiffOf(tmp_path, Film(), w)
    assert set(r.fields) == {"Header.EntityCount"}
    e = r.existence["Entities"]
    assert (e.only_golden, e.only_run, e.first_tick, e.first_key, e.first_side) == (0, 1, 2, H3, "run")
    assert "Entities: 1 only in run, 0 only in golden, first tick 2 key E2360014 (run)" in tickdump.FormatDiff(r)


def test_a_missing_player_is_an_existence_difference(tmp_path):
    r = DiffOf(tmp_path, Film(), Film(player=lambda t, h, row: None if (h == P1 and t >= 6) else row))
    e = r.existence["Players"]
    assert (e.only_golden, e.only_run, e.first_tick, e.first_key, e.first_side) == (1, 0, 6, P1, "golden")
    assert set(r.fields) == {"Header.PlayerCount"}


# --- tick ranges --------------------------------------------------------------------------------------------------------------

def test_ticks_in_one_file_only_count_unless_overlap(tmp_path):
    golden, run = Film(range(0, 10)), Film(range(3, 13))
    r = DiffOf(tmp_path, golden, run)
    assert not r.is_equal
    assert (r.tick_range, r.ticks_compared, r.only_golden_ticks, r.only_run_ticks) == ((3, 9), 7, [0, 1, 2], [10, 11, 12])
    assert (r.fields, r.existence) == ({}, {})
    o = DiffOf(tmp_path, golden, run, overlap=True)
    assert o.is_equal, tickdump.FormatDiff(o)
    assert (o.tick_range, o.ticks_compared) == ((3, 9), 7)


def test_events_before_the_compared_range_are_left_out(tmp_path):
    # A recording armed late writes the whole event window of the BridgeTick in its first record (Bridge::Codec::Dump::System::WriteRecord).
    run = Film(range(4, 10), events=lambda t, rows: [Event(1, 2)] + rows if t == 4 else rows)
    assert DiffOf(tmp_path, Film(), run, overlap=True).is_equal
    late = Film(range(4, 10), events=lambda t, rows: rows)
    assert DiffOf(tmp_path, Film(), late, overlap=True).is_equal


def test_no_common_tick_is_never_equal(tmp_path):
    r = DiffOf(tmp_path, Film(range(0, 5)), Film(range(10, 15)), overlap=True)
    assert not r.is_equal
    assert (r.tick_range, r.ticks_compared) == (None, 0)
    assert tickdump.FormatDiff(r)[-1].startswith("DIFFERS")


# --- layout changes -----------------------------------------------------------------------------------------------------------

def test_layout_changes_are_listed_and_common_fields_still_compared(tmp_path):
    real, _ = RealLayout()
    layout = ChangedLayout(real, add={"BridgeEntity": [("Foo", "u32")]}, drop={"BridgePlayer": ["Shield"]},
                           retype={"BridgeEntity": {"Kind": "u32"}})
    run = Film(entity=Changed("Vitality", 0.25, {(5, H1)}), writer=Writer(layout=layout, layout_hash=0x1))
    r = DiffOf(tmp_path, Film(), run)
    assert not r.is_equal
    assert set(r.schema) == {"+ Entities.Foo", "- Players.Shield", "~ Entities.Kind u16 -> u32"}
    assert set(r.fields) == {"Entities.Vitality"}
    assert r.fields["Entities.Vitality"].ticks == 1


# --- errors -------------------------------------------------------------------------------------------------------------------

def test_a_game_tick_going_back_is_an_error(tmp_path):
    with pytest.raises(tickdump.TickDumpError):
        DiffOf(tmp_path, Film(), Film([0, 1, 2, 1, 3, 4, 5, 6, 7, 8, 9]))


# --- repeated game ticks (D174: the theater runs 22 records on GameTick 0 while it loads, and repeats the last one) ------------

PREROLL = [0, 0, 0, 0, 1, 2, 3, 4, 5, 5, 5]


def Occurrences(ticks):
    """(GameTick, occurrence) of each record of a list of ticks."""
    seen, out = {}, []
    for t in ticks:
        out.append((t, seen.get(t, 0)))
        seen[t] = seen.get(t, 0) + 1
    return out


def Preroll(changed=None):
    """Film(PREROLL) where the entity H1 has Kind = 10 + occurrence, so every repeated record differs from the one before.
    changed = (tick, occurrence): there H1 has Kind 99."""
    keys = iter(Occurrences(PREROLL))
    def Entity(t, h, row):
        if h == H1:
            key = next(keys)
            row["Kind"] = 99 if key == changed else 10 + key[1]
        return row
    return Film(PREROLL, entity=Entity)


def test_repeated_game_ticks_align_by_occurrence(tmp_path):
    r = DiffOf(tmp_path, Preroll(), Preroll())
    assert r.is_equal, tickdump.FormatDiff(r)
    assert (r.tick_range, r.ticks_compared) == ((0, 5), len(PREROLL))


def test_a_change_in_a_repeated_tick_names_its_occurrence(tmp_path):
    r = DiffOf(tmp_path, Preroll(), Preroll(changed=(0, 2)))
    f = r.fields["Entities.Kind"]
    assert (f.ticks, f.keys, f.first_tick, f.first_occurrence, f.first_key, f.golden, f.run) == (1, 1, 0, 2, H1, "12", "99")
    assert "Entities.Kind: 1 ticks, 1 keys, first tick 0#2 key E2340012: 12 -> 99" in tickdump.FormatDiff(r)
    assert DiffOf(tmp_path, Preroll(), Preroll(changed=(1, 0))).fields["Entities.Kind"].first_occurrence == 0


def test_an_extra_occurrence_is_a_tick_in_one_file_only(tmp_path):
    r = DiffOf(tmp_path, Film([0, 0, 1, 2]), Film([0, 1, 2]))
    assert (r.ticks_compared, r.only_golden_ticks, r.only_run_ticks) == (3, [0], [])
    assert not r.is_equal
    assert DiffOf(tmp_path, Film([0, 0, 1, 2]), Film([0, 1, 2]), overlap=True).is_equal


def test_a_repeated_handle_in_a_tick_is_an_error(tmp_path):
    run = Writer(keyframe_interval=1)
    for t in range(10):
        run.Tick(t, players=[Player(P1), Player(P2)], entities=[Entity(H1), Entity(H1)])
    with pytest.raises(tickdump.TickDumpError):
        DiffOf(tmp_path, Film(), run)


# --- output -------------------------------------------------------------------------------------------------------------------

def test_field_lines_are_capped(tmp_path):
    run = Film(entity=lambda t, h, row: {**row, "Vitality": 0.5, "Kind": 4, "Lifecycle": 1, "Team": 2} if t == 5 else row)
    r = DiffOf(tmp_path, Film(), run)
    assert len(r.fields) == 4
    lines = tickdump.FormatDiff(r, max_fields=2)
    assert sum(1 for line in lines if line.startswith("Entities.")) == 2
    assert "... 2 more fields" in lines
    assert lines[-1].startswith("DIFFERS")
    assert tickdump.FormatDiff(DiffOf(tmp_path, Film(), Film()))[-1].startswith("EQUAL")


# --- command line -------------------------------------------------------------------------------------------------------------

def test_cli_diff_exit_codes(tmp_path, monkeypatch, capsys):
    monkeypatch.setenv("LOCALAPPDATA", str(tmp_path / "appdata"))
    golden = Save(tmp_path, "golden.tdump", Film())
    same = Save(tmp_path, "same.tdump", Film())
    other = Save(tmp_path, "other.tdump", Film(entity=Changed("Kind", 5, {(1, H1)})))
    late = Save(tmp_path, "late.tdump", Film(range(3, 10)))
    assert tickdump.Main(["diff", golden, same]) == 0
    assert capsys.readouterr().out.strip().splitlines()[-1].startswith("EQUAL")
    assert tickdump.Main(["diff", golden, other]) == 1
    assert capsys.readouterr().out.strip().splitlines()[-1].startswith("DIFFERS")
    assert tickdump.Main(["diff", golden, late]) == 1
    assert tickdump.Main(["diff", golden, late, "--overlap"]) == 0
    assert tickdump.Main(["diff", golden, str(tmp_path / "missing.tdump")]) == 2
    (tmp_path / "bad.tdump").write_bytes(b"x" * 10)
    assert tickdump.Main(["diff", golden, str(tmp_path / "bad.tdump")]) == 2
    assert tickdump.Main(["diff", golden, Save(tmp_path, "back.tdump", Film([0, 1, 0]))]) == 2
    repeated = Save(tmp_path, "rep.tdump", Film([0, 0, 1, 1]))
    assert tickdump.Main(["diff", repeated, repeated]) == 0
    assert tickdump.Main(["show", golden]) == 0


def test_cli_diff_finds_the_golden_folder_and_the_dumps_folder(tmp_path, monkeypatch):
    monkeypatch.setenv("LOCALAPPDATA", str(tmp_path / "appdata"))
    folder = tmp_path / "Regresion" / "film_a"
    folder.mkdir(parents=True)
    Save(folder, "golden.tdump", Film())
    dumps = tmp_path / "appdata" / "Artemis" / "TickDumps"
    dumps.mkdir(parents=True)
    Save(dumps, "20261023_140500.tdump", Film())
    assert tickdump.Main(["diff", str(folder), "20261023_140500.tdump"]) == 0


# --- approve ------------------------------------------------------------------------------------------------------------------

def Golden(tmp_path):
    folder = tmp_path / "Regresion" / "film_a"
    folder.mkdir(parents=True)
    (folder / "film.txt").write_bytes(b"Film A: forge_halo, 2v2, 58 s.")
    return folder


def test_approve_copies_the_run_and_appends_to_the_manifest(tmp_path):
    folder = Golden(tmp_path)
    first = Save(tmp_path, "run1.tdump", Film())
    line = tickdump.Approve(first, str(folder), "M2: first golden", now=NOW, commit="fb35efd+dirty")
    _, layout_hash = RealLayout()
    assert line == f"2026-10-23 14:05 | layout 0x{layout_hash:016X} | commit fb35efd+dirty | ticks 0-9 | run1.tdump | M2: first golden"
    assert (folder / "golden.tdump").read_bytes() == open(first, "rb").read()
    assert (folder / "manifest.txt").read_bytes() == line.encode("ascii")

    second = Save(tmp_path, "run2.tdump", Film(range(0, 12)))
    again = tickdump.Approve(second, str(folder), "B4h: test", now=NOW, commit="abc1234")
    assert (folder / "golden_prev.tdump").read_bytes() == open(first, "rb").read()
    assert (folder / "golden.tdump").read_bytes() == open(second, "rb").read()
    assert (folder / "manifest.txt").read_bytes() == (line + "\r\n" + again).encode("ascii")
    assert again.endswith("| ticks 0-11 | run2.tdump | B4h: test")

    third = Save(tmp_path, "run3.tdump", Film(PREROLL))
    assert tickdump.Approve(third, str(folder), "D174: preroll", now=NOW, commit="abc1234").endswith("| ticks 0-5 | run3.tdump | D174: preroll")


@pytest.mark.parametrize("case", ["no film.txt", "no footer", "dropped", "event losses", "tick going back", "empty reason",
                                  "not ascii", "no match"])
def test_approve_refuses_a_bad_golden(tmp_path, case):
    folder = Golden(tmp_path)
    reason, writer, kwargs = "M2: first golden", Film(), {}
    if case == "no film.txt":
        (folder / "film.txt").unlink()
    elif case == "no footer":
        kwargs = {"footer": False}
    elif case == "dropped":
        writer = Writer()
        Film(range(0, 5), writer=writer)
        writer.Tick(5, players=[Player(P1), Player(P2)], entities=[Entity(H1), Entity(H2)], dropped_before=2)
    elif case == "event losses":
        kwargs = {"event_losses": 1}
    elif case == "tick going back":
        writer = Film([0, 1, 2, 1, 3])
    elif case == "empty reason":
        reason = "  "
    elif case == "not ascii":
        reason = "revisi\u00f3n"
    elif case == "no match":
        writer = Writer()
        writer.Tick(0, has_match=False)
    run = Save(tmp_path, "run.tdump", writer, **kwargs)
    with pytest.raises(tickdump.TickDumpError):
        tickdump.Approve(run, str(folder), reason, now=NOW, commit="abc1234")
    assert not (folder / "golden.tdump").exists()
    assert not (folder / "manifest.txt").exists()


def test_cli_approve(tmp_path):
    folder = Golden(tmp_path)
    run = Save(tmp_path, "run.tdump", Film())
    assert tickdump.Main(["approve", run, str(folder), "--reason", "M2: first golden"]) == 0
    text = (folder / "manifest.txt").read_text("ascii")
    assert text.startswith("20") and text.endswith("| ticks 0-9 | run.tdump | M2: first golden")
    assert tickdump.Main(["approve", run, str(tmp_path / "nowhere"), "--reason", "x"]) == 2


def test_approve_copies_the_sdump_of_the_run(tmp_path):
    folder = Golden(tmp_path)
    run = Save(tmp_path, "run1.tdump", Film())
    (tmp_path / "run1.sdump").write_bytes(b"ASDUMP snapshots")

    line = tickdump.Approve(run, str(folder), "M5c: with sdump", now=NOW, commit="abc1234")

    assert (folder / "golden.sdump").read_bytes() == b"ASDUMP snapshots"
    assert (folder / "golden.tdump").read_bytes() == open(run, "rb").read()
    assert line.endswith("| ticks 0-9 | run1.tdump + run1.sdump | M5c: with sdump")


def test_approve_without_sdump_deletes_the_old_golden_sdump(tmp_path):
    folder = Golden(tmp_path)
    (folder / "golden.sdump").write_bytes(b"old snapshots")
    run = Save(tmp_path, "run2.tdump", Film())

    line = tickdump.Approve(run, str(folder), "M5c: without sdump", now=NOW, commit="abc1234")

    assert not (folder / "golden.sdump").exists()
    assert (folder / "golden.tdump").exists()
    assert line.endswith("| ticks 0-9 | run2.tdump | M5c: without sdump")


# --- flags and stands (B4h H.4, format version 2) -----------------------------------------------------------------------------

def FlagFilm(state_at=None, ticks=range(6)):
    """A film with two flags and a stand. state_at: first tick from which the blue flag (BaseTeam 1) is Carried (None = Home)."""
    w = Writer()
    for t in ticks:
        carried = state_at is not None and t >= state_at
        flags = [{"Handle": 0xA1, "BaseTeam": 0, "OwnerTeam": 1, "State": 0, "CarrierPlayer": 0xFFFFFFFF},
                 {"Handle": 0xA2, "BaseTeam": 1, "OwnerTeam": 0, "State": 1 if carried else 0, "CarrierPlayer": P1 if carried else 0xFFFFFFFF,
                  "AlliesInZone": 2}]
        stands = [{"Handle": 0xB1, "BaseTeam": 1, "OwnerTeam": 0}]
        w.Tick(t, players=[Player(P1)], entities=[Entity(H1)], objectives=flags, stands=stands)
    return w


def test_flags_and_stands_round_trip_through_the_reader(tmp_path):
    d = tickdump.Dump(Save(tmp_path, "a.tdump", FlagFilm(state_at=3)))
    assert d.version == 2 and not d.is_cut
    assert [len(r.objectives) for r in d.records] == [2] * 6
    assert [len(r.stands) for r in d.records] == [1] * 6
    assert [int(r.objectives["State"][1]) for r in d.records] == [0, 0, 0, 1, 1, 1]
    assert int(d.records[4].objectives["CarrierPlayer"][1]) == P1
    assert int(d.records[0].stands["OwnerTeam"][0]) == 0
    assert int(d.records[0].header["ObjectiveCount"]) == 2 and int(d.records[0].header["StandCount"]) == 1
    assert [len(table) for _, table in d.states()] == [1] * 6


def test_a_flag_that_changes_state_is_a_difference_of_its_table(tmp_path):
    r = DiffOf(tmp_path, FlagFilm(), FlagFilm(state_at=3))
    assert not r.is_equal
    assert set(r.fields) == {"Objectives.State", "Objectives.CarrierPlayer"}
    assert (r.fields["Objectives.State"].first_tick, r.fields["Objectives.State"].first_key) == (3, 0xA2)
    assert (r.fields["Objectives.State"].golden, r.fields["Objectives.State"].run) == ("0", "1")
    assert r.existence == {} and r.schema == []


def test_a_flag_recreated_with_another_handle_is_an_existence_difference(tmp_path):
    golden, run = FlagFilm(), Writer()
    for t in range(6):
        run.Tick(t, players=[Player(P1)], entities=[Entity(H1)],
                 objectives=[{"Handle": 0xA1, "BaseTeam": 0, "OwnerTeam": 1, "CarrierPlayer": 0xFFFFFFFF},
                             {"Handle": 0xA2 if t < 4 else 0xA9, "BaseTeam": 1, "OwnerTeam": 0, "AlliesInZone": 2,
                              "CarrierPlayer": 0xFFFFFFFF}],
                 stands=[{"Handle": 0xB1, "BaseTeam": 1, "OwnerTeam": 0}])
    r = DiffOf(tmp_path, golden, run)
    assert set(r.existence) == {"Objectives"}
    e = r.existence["Objectives"]
    assert (e.only_golden, e.only_run, e.first_tick, e.first_key) == (1, 1, 4, 0xA2)


def test_a_golden_from_before_the_flags_is_read_and_reports_the_new_tables_as_schema(tmp_path):
    real, _ = RealLayout()
    old_layout = ChangedLayout(real, drop={"BridgeTickHeader": ["ObjectiveCount", "StandCount"]})
    old_layout["structs"] = [s for s in old_layout["structs"] if s["name"] not in ("BridgeObjective", "BridgeStand")]
    golden = Film(range(4), writer=Writer(layout=old_layout, layout_hash=0x1))
    gpath = Save(tmp_path, "old.tdump", golden)
    assert tickdump.Dump(gpath).version == 1
    r = tickdump.Diff(tickdump.Dump(gpath), tickdump.Dump(Save(tmp_path, "new.tdump", FlagFilm(ticks=range(4)))))
    assert not r.is_equal
    assert "+ Objectives (table of 10 fields)" in r.schema and "+ Stands (table of 3 fields)" in r.schema
    assert "+ Header.ObjectiveCount" in r.schema and "+ Header.StandCount" in r.schema
    assert not any(k.startswith(("Objectives", "Stands")) for k in r.fields) and "Objectives" not in r.existence

