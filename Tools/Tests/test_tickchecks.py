"""
test_tickchecks.py
`tickdump.py assert` and the rules of Tools/tickchecks.py (F1 sub-commit 2): a golden passes, a dump changed in memory fails with the expected rule, the
exit codes are 0 pass, 1 fail, 2 error. The rules themselves are asserted film by film in test_golden_tick.py, which imports the same module.
"""
import copy
import json
import os

import pytest

import tickchecks
import tickdump
from tdump_writer import Repo

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
