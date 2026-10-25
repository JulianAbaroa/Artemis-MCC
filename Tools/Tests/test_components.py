"""
test_components.py
Boundaries of the components that already are projects (redesign D153; Ignore Rediseno/02 sec. 3.5, D156, D165): each one imports
only the modules of the components it may depend on, references only those projects, and lists every source it owns.
Written by Opus before the code (D161) in M3 (D177-D181); each later step that creates a component adds it to COMPONENTS.
They read the files of the repo (git ls-files), they do not build.
"""
import os
import re
import subprocess

import pytest

from tdump_writer import Repo

REPO = Repo()

# Folder of the component -> (project, roots of the modules it may import, components it may reference). std is always allowed.
COMPONENTS = {
    "Artemis/Common": ("Artemis.Common.vcxproj", {"Common"}, set()),
    "Artemis/Services": ("Artemis.Services.vcxproj", {"Common", "Service"}, {"Common"}),
    "Artemis/Blam.Map": ("Artemis.Blam.Map.vcxproj", {"Common", "Map"}, {"Common"}),
    "Artemis/Blam.MapModel": ("Artemis.Blam.MapModel.vcxproj", {"Common", "Map", "Resolved"}, {"Common", "Blam.Map"}),
}

# Outside Blam.Map and Blam.MapModel, the modules of Blam.Map (Map.*) are imported only by the wiring of the DLL (future Mod),
# the development tools (future DevTools) and the tests (02 sec. 3.5, contract of Blam.Map).
MAP_IMPORTERS = ("Artemis/Core/Core.ixx", "Artemis/Core/Core.cpp", "Artemis/Core/Runtime/", "Artemis/Core/UI/", "Artemis/Tests/")

# Group queries of the old TagResolverService that nobody called (removed in M3, D180).
REMOVED_NAMES = ("HasBipd", "HasBloc", "HasCtrl", "HasEqip", "HasMach", "HasProj", "HasScen", "HasVehi", "HasWeap")

IMPORT = re.compile(r"^\s*(?:export\s+)?import\s+([A-Za-z_][\w.]*)\s*;", re.MULTILINE)
REFERENCE = re.compile(r'<ProjectReference\s+Include="([^"]+)"')
COMPILE = re.compile(r'<ClCompile\s+Include="([^"]+)"')


def Tracked(*paths):
    out = subprocess.run(["git", "-C", REPO, "ls-files", "--cached", "--others", "--exclude-standard", "--", *paths],
                         capture_output=True, text=True, check=True).stdout
    return [line for line in out.splitlines() if line]


def Sources(folder):
    return [p for p in Tracked(folder) if p.endswith((".ixx", ".cpp")) and "/x64/" not in p]


def Read(path):
    with open(os.path.join(REPO, path), encoding="utf-8-sig", errors="replace") as f:
        return f.read()


def ImportRoots(path):
    return {name.split(".")[0] for name in IMPORT.findall(Read(path)) if name != "std"}


def ProjectName(include):
    name = os.path.basename(include.replace("\\", "/"))
    name = name[:-len(".vcxproj")] if name.endswith(".vcxproj") else name
    return name[len("Artemis."):] if name.startswith("Artemis.") else name


@pytest.mark.parametrize("folder", sorted(COMPONENTS))
def test_component_imports_only_what_it_may_depend_on(folder):
    _, allowed, _ = COMPONENTS[folder]
    sources = Sources(folder)
    assert sources, f"{folder} has no sources"
    wrong = {path: sorted(ImportRoots(path) - allowed) for path in sources if ImportRoots(path) - allowed}
    assert not wrong, f"imports outside {sorted(allowed)}: {wrong}"


@pytest.mark.parametrize("folder", sorted(COMPONENTS))
def test_component_references_only_its_dependencies(folder):
    project, _, references = COMPONENTS[folder]
    found = {ProjectName(include) for include in REFERENCE.findall(Read(f"{folder}/{project}"))}
    assert found == references


@pytest.mark.parametrize("folder", sorted(COMPONENTS))
def test_component_project_lists_every_source(folder):
    project, _, _ = COMPONENTS[folder]
    for listing in (project, project + ".filters"):
        listed = {include.replace("\\", "/") for include in COMPILE.findall(Read(f"{folder}/{listing}"))}
        missing = sorted(p[len(folder) + 1:] for p in Sources(folder) if p[len(folder) + 1:] not in listed)
        assert not missing, f"not in {listing}: {missing}"


def test_dll_project_references_the_components():
    found = {ProjectName(include) for include in REFERENCE.findall(Read("Artemis/Artemis.vcxproj"))}
    assert {"Common", "Services", "Blam.Map", "Blam.MapModel"} <= found


def test_old_map_folders_are_gone():
    assert Tracked("Artemis/Core/Domain/1_Map", "Artemis/Core/Domain/2_Resolved") == []


def test_only_wiring_tools_and_tests_import_blam_map():
    inside = ("Artemis/Blam.Map/", "Artemis/Blam.MapModel/")
    wrong = [path for path in Sources("Artemis") if not path.startswith(inside) and not path.startswith(MAP_IMPORTERS)
             and "Map" in ImportRoots(path)]
    assert not wrong, f"import Map.* outside the wiring, the tools and the tests: {wrong}"


def test_tag_resolver_split():
    in_map = [p for p in Sources("Artemis/Blam.Map") if "TagResolverService" in Read(p)]
    in_model = [p for p in Sources("Artemis/Blam.MapModel") if "TagReferenceService" in Read(p) and "TagResolver" in p]
    assert not in_map, f"TagResolverService is of Blam.MapModel: {in_map}"
    assert not in_model, f"the resolver of Blam.MapModel does not use TagReferenceService: {in_model}"
    definitions = [p for p in Sources("Artemis") if re.search(r"\bclass\s+TagResolverService\b", Read(p))]
    assert len(definitions) == 1 and definitions[0].startswith("Artemis/Blam.MapModel/"), definitions


def test_unused_group_queries_are_removed():
    result = subprocess.run(["git", "-C", REPO, "grep", "--untracked", "-n", "-w", "-E", "|".join(REMOVED_NAMES), "--",
                             "Artemis"], capture_output=True, text=True)
    assert result.stdout == ""


def test_the_engine_hook_of_the_map_lives_in_platform():
    hooks = [p for p in Sources("Artemis") if os.path.basename(p).startswith("OpenMapDetour.")]
    assert sorted(os.path.basename(p) for p in hooks) == ["OpenMapDetour.cpp", "OpenMapDetour.ixx"]
    assert all(p.startswith("Artemis/Core/Infrastructure/Platform/Lifecycle/Hook/") for p in hooks), hooks
    for path in Sources("Artemis/Blam.Map") + Sources("Artemis/Blam.MapModel"):
        text = Read(path)
        assert "windows.h" not in text and "GetModuleFileName" not in text, path
