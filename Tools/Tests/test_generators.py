"""
test_generators.py
Tests of the generators of C++ from XML (M3a, Ignore Rediseno/Tareas/M3a.txt; D176): GenerateDatum.py and GenerateTags.py write
exactly the files that are in the repo, byte for byte, and --check says so (exit 0) or names the files that differ (exit 1).
Written by Opus before the code (D161); they are not changed to pass. M3 (D177): the tag output moved to Artemis/Blam.Map/Tag.
The tag XMLs belong to Assembly and stay out of the repo: the GenerateTags tests run when ARTEMIS_ASSEMBLY_TAGS (or the default
Documents/Tools/Assembly/Tags of the user) is a folder, and are skipped otherwise.
"""
import os
import subprocess
import sys

import pytest

from tdump_writer import Repo

REPO = Repo()
TRANSPILER = os.path.join(REPO, "Tools", "TagTranspiler")
DATUM_OUT = os.path.join(REPO, "Artemis", "Core", "Domain", "3_Datum", "Object", "Type")
TAG_OUT = os.path.join(REPO, "Artemis", "Blam.Map", "Tag")
ASSEMBLY_TAGS = os.environ.get("ARTEMIS_ASSEMBLY_TAGS") or os.path.expanduser(os.path.join("~", "Documents", "Tools", "Assembly",
                                                                                            "Tags"))
NEEDS_ASSEMBLY = pytest.mark.skipif(not os.path.isdir(ASSEMBLY_TAGS),
                                    reason="tag XMLs of Assembly not found (set ARTEMIS_ASSEMBLY_TAGS)")


def Run(script, *args):
    return subprocess.run([sys.executable, os.path.join(TRANSPILER, script), *args], capture_output=True, text=True,
                          errors="replace", env=dict(os.environ, PYTHONDONTWRITEBYTECODE="1"))


def Files(root):
    """{path relative to root: bytes} of every .ixx under root."""
    out = {}
    for folder, _, names in os.walk(root):
        for name in names:
            if name.endswith(".ixx"):
                path = os.path.join(folder, name)
                with open(path, "rb") as handle:
                    out[os.path.relpath(path, root).replace(os.sep, "/")] = handle.read()
    return out


# --- GenerateDatum ------------------------------------------------------------------------------------------------------------

def test_generate_datum_check_matches_the_repo():
    result = Run("GenerateDatum.py", "--check")
    assert result.returncode == 0, result.stdout + result.stderr


def test_generate_datum_out_writes_the_bytes_of_the_repo(tmp_path):
    result = Run("GenerateDatum.py", "--out", str(tmp_path))
    assert result.returncode == 0, result.stdout + result.stderr
    generated, repo = Files(tmp_path), Files(DATUM_OUT)
    assert len(generated) == 45
    for path, data in generated.items():
        assert data == repo.get(path), path


@pytest.mark.parametrize("change", ["edited", "missing"])
def test_check_names_the_file_that_differs(tmp_path, change):
    assert Run("GenerateDatum.py", "--out", str(tmp_path)).returncode == 0
    target = tmp_path / "Obje" / "DatumObjeStructure.ixx"
    if change == "edited":
        target.write_bytes(target.read_bytes() + b" ")
    else:
        target.unlink()
    result = Run("GenerateDatum.py", "--check", "--out", str(tmp_path))
    assert result.returncode == 1
    assert "DatumObjeStructure.ixx" in result.stdout
    assert "DatumObjeOffset.ixx" not in result.stdout
    # --check compares and writes nothing.
    if change == "edited":
        assert target.read_bytes().endswith(b" ")
    else:
        assert not target.exists()


def test_datum_structures_do_not_import_the_map_reader():
    for path, data in Files(DATUM_OUT).items():
        if not path.endswith("Structure.ixx"):
            continue
        text = data.decode("utf-8")
        assert "import Map." not in text, path
        assert "MapStructure" not in text, path
        if "MathType::" in text:
            assert "import Common.Math.Type;" in text, path
            assert "namespace MathType = Common::Math::Type;" in text, path


def test_no_vector_of_the_map_reader_is_left():
    result = subprocess.run(["git", "-C", REPO, "grep", "--untracked", "-n", "-E", r"Primitive::Vec[234]|MakeVec[34]", "--",
                             "Artemis", "Tools"], capture_output=True, text=True, errors="replace")
    assert result.returncode == 1, result.stdout


# --- GenerateTags -------------------------------------------------------------------------------------------------------------

@NEEDS_ASSEMBLY
def test_generate_tags_check_matches_the_repo():
    result = Run("GenerateTags.py", ASSEMBLY_TAGS, "--check")
    assert result.returncode == 0, result.stdout + result.stderr


@NEEDS_ASSEMBLY
def test_generate_tags_out_writes_the_bytes_of_the_repo(tmp_path):
    result = Run("GenerateTags.py", ASSEMBLY_TAGS, "--out", str(tmp_path))
    assert result.returncode == 0, result.stdout + result.stderr
    generated, repo = Files(tmp_path), Files(TAG_OUT)
    assert set(generated) == set(repo)
    assert len(generated) == 130
    for path, data in generated.items():
        assert data == repo[path], path


def test_tag_structures_import_what_they_use():
    for path, data in Files(TAG_OUT).items():
        if not path.endswith("Structure.ixx"):
            continue
        text = data.decode("utf-8")
        assert ("import Common.Math.Type;" in text) == ("MathType::" in text), path
        assert ("import Map.Reader.Type;" in text) == ("MapStructure::" in text), path
        if "import Common.Math.Type;" in text and "import Map.Reader.Type;" in text:
            assert text.index("import Common.Math.Type;") < text.index("import Map.Reader.Type;"), path


# --- safety -------------------------------------------------------------------------------------------------------------------

def RepoStatus():
    result = subprocess.run(["git", "-C", REPO, "status", "--porcelain", "--untracked-files=all", "--", DATUM_OUT, TAG_OUT],
                            capture_output=True, text=True, errors="replace")
    return result.stdout


def test_check_and_out_leave_the_repo_untouched(tmp_path):
    # A run of the generators while M3a was designed rewrote the tag files of the repo: --check and --out must never write there.
    before = RepoStatus()
    Run("GenerateDatum.py", "--check")
    Run("GenerateDatum.py", "--out", str(tmp_path / "datum"))
    if os.path.isdir(ASSEMBLY_TAGS):
        Run("GenerateTags.py", ASSEMBLY_TAGS, "--check")
        Run("GenerateTags.py", ASSEMBLY_TAGS, "--out", str(tmp_path / "tags"))
    assert RepoStatus() == before
