"""
test_tools.py
Tests of the verification tools themselves: Tools/test.py Evaluate (no false green), the --update-baseline guard of check_rules.py
and the -n handling of deps.py.
"""
import hashlib
import importlib.util
import os
import re
import subprocess
import sys

TOOLS = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ROOT = os.path.dirname(TOOLS)
BASELINE = os.path.join(TOOLS, "check_rules_baseline.txt")


def LoadTool(name):
    spec = importlib.util.spec_from_file_location("artemis_" + name, os.path.join(TOOLS, name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


TestTool = LoadTool("test")


def Case(name, failed=False, skipped=False):
    case = {"name": name}
    if failed:
        case["failures"] = [{"failure": "a.cpp:1\nExpected equality"}]
    if skipped:
        case["result"] = "SKIPPED"
    return case


def Data(*cases):
    return {"time": "1ms", "testsuites": [{"name": "S", "testsuite": list(cases)}]}


def Run(script, *arguments):
    return subprocess.run([sys.executable, os.path.join(TOOLS, script)] + list(arguments), cwd=ROOT, capture_output=True, text=True)


def Digest(path):
    with open(path, "rb") as handle:
        return hashlib.sha256(handle.read()).hexdigest()


def Rows(output):
    return [line for line in output.splitlines() if re.match(r"^\s+\d+\s+\S", line)]


def test_evaluate_all_passed_is_0():
    code, lines = TestTool.Evaluate(Data(Case("A"), Case("B")), 0)
    assert code == 0
    assert lines[0].startswith("Tests: 2 passed, 0 failed, 0 skipped")


def test_evaluate_failure_is_1():
    code, lines = TestTool.Evaluate(Data(Case("A"), Case("B", failed=True)), 1)
    assert code == 1
    assert any("FAILED S.B" in line for line in lines)


def test_evaluate_no_testsuites_is_2():
    for data in ({}, {"testsuites": None}, {"testsuites": {}}, []):
        code, lines = TestTool.Evaluate(data, 0)
        assert code == 2
        assert "malformed" in lines[0] and "no testsuites" in lines[0]


def test_evaluate_zero_cases_is_2():
    code, lines = TestTool.Evaluate({"testsuites": []}, 0)
    assert code == 2 and "no test ran" in lines[0] and "filter" not in lines[0]
    code, lines = TestTool.Evaluate(Data(), 0, filter="NoSuchSuite.*")
    assert code == 2 and "the filter matched no test" in lines[0]


def test_evaluate_only_skipped_is_0():
    code, lines = TestTool.Evaluate(Data(Case("A", skipped=True)), 0, {"S.A": "no game"}, "S.*")
    assert code == 0
    assert any("SKIPPED S.A: no game" in line for line in lines)


def test_evaluate_nonzero_exit_without_failures_is_2():
    code, lines = TestTool.Evaluate(Data(Case("A")), 3)
    assert code == 2
    assert any("the exe exited with 3 but the JSON lists no failure" in line for line in lines)


def test_evaluate_nonzero_exit_with_failures_is_1():
    code, _ = TestTool.Evaluate(Data(Case("A", failed=True)), 1)
    assert code == 1
    code, _ = TestTool.Evaluate(Data(Case("A", failed=True)), 0)
    assert code == 1


def test_update_baseline_with_files_is_rejected():
    before = Digest(BASELINE)
    done = Run("check_rules.py", "--update-baseline", "--files", "Tools/deps.py")
    assert done.returncode == 2
    assert "--update-baseline needs the full run" in done.stderr
    assert Digest(BASELINE) == before


def test_update_baseline_with_only_is_rejected():
    before = Digest(BASELINE)
    done = Run("check_rules.py", "--update-baseline", "--only", "D2")
    assert done.returncode == 2
    assert "--update-baseline needs the full run" in done.stderr
    assert Digest(BASELINE) == before


def test_top_n_caps():
    done = Run("deps.py", "top", "-n", "3")
    assert done.returncode == 0
    assert len(Rows(done.stdout)) == 3


def test_top_n_zero_is_uncapped():
    default = Run("deps.py", "top")
    uncapped = Run("deps.py", "top", "-n", "0")
    assert uncapped.returncode == 0
    assert "more" not in uncapped.stdout
    assert len(Rows(uncapped.stdout)) >= len(Rows(default.stdout))
    assert len(Rows(default.stdout)) <= 20
