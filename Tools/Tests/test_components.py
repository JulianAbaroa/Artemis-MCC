"""
test_components.py
Boundaries of the components that already are projects (redesign D153; Ignore Rediseno/02 sec. 3.5, D156, D165): each one imports
only the modules of the components it may depend on, references only those projects, and lists every source it owns.
Written by Opus before the code (D161) in M3 (D177-D181) and extended in M4a (D182-D188); each later step that creates a component
adds it to COMPONENTS.
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
    "Artemis/Blam.Layout": ("Artemis.Blam.Layout.vcxproj", {"Common", "Datum", "Layout"}, {"Common"}),
}

# The interpretation of the live objects (future Blam.Runtime, 02 sec. 3.2) reads only the Snapshot since M4a: these folders import
# nothing of Platform, Service or the DLL wiring (D184).
TABLES = ("Artemis/Core/Domain/4_Tables/Game/", "Artemis/Core/Domain/4_Tables/Interaction/", "Artemis/Core/Domain/4_Tables/Object/",
          "Artemis/Core/Domain/4_Tables/Player/")
TABLES_ROOTS = {"Common", "Datum", "Layout", "Resolved", "Tables"}

# The three locators of the engine tables (Engine.Capture in 02, Platform/Capture until M6).
LOCATORS = ("ObjectTableLocator", "PlayerTableLocator", "InteractionTableLocator")
CAPTURE_HOOK = "Artemis/Core/Infrastructure/Platform/Capture/Hook/"

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
    assert {"Common", "Services", "Blam.Map", "Blam.MapModel", "Blam.Layout"} <= found


def test_tests_project_references_blam_layout():
    found = {ProjectName(include) for include in REFERENCE.findall(Read("Artemis/Tests/Artemis.Tests.vcxproj"))}
    assert {"Blam.Map", "Blam.MapModel", "Blam.Layout"} <= found


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


def test_old_datum_folder_is_gone():
    assert Tracked("Artemis/Core/Domain/3_Datum") == []


def test_engine_offsets_left_tables():
    moved = [p for p in Sources("Artemis/Core/Domain/4_Tables")
             if os.path.basename(p) in ("PlayerOffset.ixx", "PlayerMapping.ixx", "PlayerSize.ixx", "InteractionOffset.ixx",
                                        "GameOffset.ixx")]
    assert not moved, moved
    old = re.compile(r"Tables::(Player|Interaction|Game)::Type::(Offset|Mapping|Size)::")
    wrong = [p for p in Sources("Artemis") if old.search(Read(p))]
    assert not wrong, f"old namespace of the offsets: {wrong}"


def test_raw_snapshot_lives_in_blam_layout():
    wrong = [p for p in Sources("Artemis") if "Datum::Object::Type::Raw" in Read(p) or "DatumStore" in Read(p)]
    assert not wrong, wrong
    definitions = [p for p in Sources("Artemis") if re.search(r"\bstruct\s+RawObject\b", Read(p))]
    assert len(definitions) == 1 and definitions[0].startswith("Artemis/Blam.Layout/"), definitions


@pytest.mark.parametrize("folder", TABLES)
def test_tables_read_only_the_snapshot(folder):
    sources = Sources(folder)
    assert sources, f"{folder} has no sources"
    wrong = {path: sorted(ImportRoots(path) - TABLES_ROOTS) for path in sources if ImportRoots(path) - TABLES_ROOTS}
    assert not wrong, f"imports outside {sorted(TABLES_ROOTS)}: {wrong}"
    readers = [p for p in sources if "MemoryReaderService" in Read(p) or "reader.Read" in Read(p)]
    assert not readers, f"live reads of the game left in: {readers}"


def test_locators_live_in_platform_capture():
    found = [p for p in Sources("Artemis") if os.path.basename(p).split(".")[0] in LOCATORS]
    assert sorted(os.path.basename(p) for p in found) == sorted(f"{name}.{ext}" for name in LOCATORS for ext in ("cpp", "ixx"))
    assert all(p.startswith(CAPTURE_HOOK) for p in found), found


def test_max_local_users_has_one_owner():
    definitions = [p for p in Sources("Artemis") if re.search(r"\bk_MaxLocalUsers\s*\{", Read(p))]
    assert definitions == ["Artemis/Common/Engine/Type/Common.Engine.Type.ixx"], definitions
    wrong = [p for p in Sources("Artemis") if "Platform::Host::Type::k_MaxLocalUsers" in Read(p)]
    assert not wrong, wrong


def test_blam_layout_does_not_touch_the_process():
    for path in Sources("Artemis/Blam.Layout"):
        text = Read(path)
        assert "windows.h" not in text and "std::uintptr_t*" not in text and "reinterpret_cast" not in text, path
