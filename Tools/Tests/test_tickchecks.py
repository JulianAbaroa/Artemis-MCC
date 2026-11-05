"""
test_tickchecks.py
`tickdump.py assert` and the rules of Tools/tickchecks.py (F1 sub-commit 2): a golden passes, a dump changed in memory fails with the expected rule, the
exit codes are 0 pass, 1 fail, 2 error. The rules themselves are asserted film by film in test_golden_tick.py, which imports the same module.
"""
import copy
import json
import os
import re

import numpy as np
import pytest

import tickchecks
import tickdump
from tdump_writer import Repo, Writer

GOLDEN = os.path.join(os.environ.get("ARTEMIS_RECORDINGS") or os.path.join(Repo(), "Ignore", "Regresion"), "cgb_displace_1min", "golden.tdump")
needs_golden = pytest.mark.skipif(not os.path.isfile(GOLDEN), reason="the cgb_displace_1min golden is not in this checkout")


@pytest.fixture(scope="module")
def golden():
    return tickdump.Dump(GOLDEN)


def Altered(golden, change):
    """A copy of the dump in memory (the rows of a record are read-only views of the file) with change(dump) applied."""
    dump = copy.copy(golden)
    dump.records = [copy.copy(r) for r in golden.records]
    change(dump)
    return dump


@needs_golden
def test_a_golden_passes_in_text_and_json(capsys):
    assert tickdump.Main(["assert", GOLDEN]) == 0
    assert capsys.readouterr().out.startswith("PASS golden.tdump")
    assert tickdump.Main(["assert", GOLDEN, "--json"]) == 0
    report = json.loads(capsys.readouterr().out)
    assert report["result"] == "PASS" and report["rules"] == [] and report["records"] > 0


@needs_golden
def test_a_header_flag_set_in_memory_fails_with_its_rule(golden, capsys):
    def Change(dump):
        r = dump.records[7]
        r.header = r.header.copy()
        r.header["Flags"] = 1
    altered = Altered(golden, Change)
    tick = altered.records[7].game_tick
    assert tickdump.Assert(altered) == 1
    out = capsys.readouterr().out
    assert out.startswith("FAIL") and "header_flags: 1 case(s)" in out and f"tick {tick} " in out
    assert tickdump.Assert(altered, as_json=True) == 1
    report = json.loads(capsys.readouterr().out)
    assert report["result"] == "FAIL"
    assert [(r["name"], r["count"], r["cases"][0]["tick"]) for r in report["rules"]] == [("header_flags", 1, tick)]


@needs_golden
def test_dropped_ticks_are_a_rule_and_a_cut_file_too(golden):
    def Drop(dump):
        dump.records[3].dropped = 4
    _, failures = tickchecks.Evaluate(Altered(golden, Drop))
    assert [(name, count) for name, count, _ in failures] == [("dropped_ticks", 1)]
    assert failures[0][2][0][2] == "4 dropped before the record"

    def Cut(dump):
        dump.footer = None
    _, failures = tickchecks.Evaluate(Altered(golden, Cut))
    assert [name for name, _, _ in failures] == ["dump_cut"]


@needs_golden
def test_an_empty_dump_fails_and_a_missing_file_is_an_error(golden, tmp_path, capsys):
    def Empty(dump):
        dump.records = []
    _, failures = tickchecks.Evaluate(Altered(golden, Empty))
    assert [name for name, _, _ in failures] == ["no_records"]
    assert tickdump.Main(["assert", str(tmp_path / "missing.tdump")]) == 2
    assert "ERROR" in capsys.readouterr().err


def test_the_rule_names_are_unique():
    assert len(set(tickchecks.ALL_RULES)) == len(tickchecks.ALL_RULES)
    assert set(tickchecks.FILE_RULES).isdisjoint(tickchecks.GENERAL_RULES)


def test_parent_kinds_mixes_the_sentinel_only_after_a_signed_cast():
    # The Kind column of a BridgeEntity is unsigned: numpy 2 (NEP 50) refuses -1 next to uint16.
    rows = np.zeros(3, np.dtype([("Kind", "<u2")]))
    rows["Kind"] = [5, 10, 65535]
    kinds = tickchecks.ParentKinds(np.array([True, False, True]), rows)
    assert kinds.dtype == np.int16
    assert kinds.tolist() == [5, -1, -1]
    strict = np.where(np.array([True, False]), np.array([7, 7], np.uint16).astype(np.int16), np.int16(-1))
    assert strict.tolist() == [7, -1]


def test_no_np_where_or_fill_mixes_a_negative_literal_with_a_column():
    with open(tickchecks.__file__, encoding="utf-8") as handle:
        source = handle.read()
    assert not re.search(r"np\.where\([^\n]*,\s*-1\s*\)", source)
    assert not re.search(r"np\.(?:full|where)\([^\n]*rows\[\"Kind\"\](?!\.astype)", source)


def test_a_link_to_a_handle_that_is_not_in_the_tick_is_found_without_an_exception(tmp_path):
    # The case of the live crash: an entity bound to a handle that no entity of the tick has.
    writer = Writer()
    for tick in range(3):
        writer.Tick(tick, entities=[{"Handle": 0xE2340012, "Kind": 2, "Vitality": 1.0, "BoundTo": 0xE2999999, "ParentHandle": 0xE2999999}])
    dump = tickdump.Dump(writer.Save(tmp_path / "linked.tdump"))
    _, failures = tickchecks.Evaluate(dump)
    assert "entity_bound_to_not_in_the_tick" in [name for name, _, _ in failures]


def test_an_exception_inside_assert_is_exit_2_with_the_message_on_stderr(tmp_path, monkeypatch, capsys):
    path = Writer().Save(tmp_path / "x.tdump")

    def Boom(*args, **kwargs):
        raise OverflowError("Python integer -1 out of bounds for uint16")
    monkeypatch.setattr(tickchecks, "Evaluate", Boom)
    assert tickdump.Main(["assert", path, "--json"]) == 2
    captured = capsys.readouterr()
    assert captured.out == ""
    assert "ERROR: assert failed inside the tool: OverflowError: Python integer -1 out of bounds for uint16" in captured.err
    assert "Traceback" in captured.err
    # Failing rules still exit 1, and only the assert command turns exceptions into 2.
    monkeypatch.undo()
    assert tickdump.Main(["assert", path]) in (0, 1)
