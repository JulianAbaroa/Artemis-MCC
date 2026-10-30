"""
test_components.py
Boundaries of the components that already are projects (redesign D153, D165; contracts D248): each one imports
only the modules of the components it may depend on, references only those projects, and lists every source it owns.
Written by Opus before the code (D161) in M3 (D177-D181) and extended in M4a, M4b (D182-D188), M5a (D189-D197), M5b (D198-D201) and M6a (D210-D215); each later step
that creates a component adds it to COMPONENTS.
They read the files of the repo (git ls-files), they do not build.
"""
import os
import re
import subprocess
import sys

import pytest

from tdump_writer import Repo

REPO = Repo()
sys.path.insert(0, os.path.join(REPO, "Tools"))
from components import PROJECTS, SOLUTION, Folder as ProjectFolder, Project as ProjectPath  # noqa: E402

ENGINE = {"Engine.Base", "Engine.Capture", "Engine.Control", "Engine.Match", "Engine.Render"}
# What Viewer and Workbench may import and reference (Workbench adds itself and Viewer; Gui is much narrower, below).
DEVTOOLS_IMPORTS = {"Common", "Services", "Blam.Map", "Blam.MapModel", "Blam.Layout", "Blam.Runtime", "Perception", "Views", "Bridge.Codec",
                    "Contract", "Control", "Bridge.Transport", "DevTools.Gui", "DevTools.Viewer"} | ENGINE
DEVTOOLS_REFERENCES = {"Common", "Contract", "Services", "Blam.Layout", "Blam.Map", "Blam.MapModel", "Blam.Runtime", "Control", "Perception",
                       "Views", "Bridge.Codec", "Bridge.Transport", "Engine.Base", "Engine.Capture", "Engine.Control", "Engine.Match",
                       "Engine.Render"}

# Folder of the component -> (project, components whose modules it may import, components it may reference). std is always allowed.
# Every module name starts with the name of its component (D233), so ImportRoots gives components.
COMPONENTS = {
    "Artemis/1_Shared/Common": ("Artemis.Common.vcxproj", {"Common"}, set()),
    "Artemis/1_Shared/Services": ("Artemis.Services.vcxproj", {"Common", "Services"}, {"Common"}),
    "Artemis/2_Blam/Map": ("Artemis.Blam.Map.vcxproj", {"Common", "Blam.Map"}, {"Common"}),
    "Artemis/2_Blam/MapModel": ("Artemis.Blam.MapModel.vcxproj", {"Common", "Blam.Map", "Blam.MapModel"}, {"Common", "Blam.Map"}),
    "Artemis/2_Blam/Layout": ("Artemis.Blam.Layout.vcxproj", {"Common", "Blam.Layout"}, {"Common"}),
    "Artemis/2_Blam/Runtime": ("Artemis.Blam.Runtime.vcxproj", {"Common", "Blam.Layout", "Blam.MapModel", "Blam.Runtime"},
                             {"Common", "Blam.Map", "Blam.MapModel", "Blam.Layout"}),
    "Artemis/1_Shared/Contract": ("Artemis.Contract.vcxproj", {"Common", "Contract"}, {"Common"}),
    "Artemis/3_Domain/Perception": ("Artemis.Perception.vcxproj", {"Common", "Blam.Layout", "Blam.MapModel", "Blam.Runtime", "Perception"},
                           {"Common", "Blam.Map", "Blam.MapModel", "Blam.Layout", "Blam.Runtime"}),
    "Artemis/3_Domain/Views": ("Artemis.Views.vcxproj",
                      {"Common", "Blam.Layout", "Blam.MapModel", "Blam.Runtime", "Perception", "Views"},
                      {"Common", "Blam.Map", "Blam.MapModel", "Blam.Layout", "Blam.Runtime", "Perception"}),
    "Artemis/4_Bridge/Codec": ("Artemis.Bridge.Codec.vcxproj",
                             {"Common", "Blam.Layout", "Blam.MapModel", "Blam.Runtime", "Perception", "Views", "Bridge.Codec", "Contract"},
                             {"Common", "Blam.Map", "Blam.MapModel", "Blam.Layout", "Blam.Runtime", "Perception", "Views", "Contract"}),
    "Artemis/3_Domain/Control": ("Artemis.Control.vcxproj", {"Common", "Blam.Layout", "Contract", "Control"}, {"Common", "Blam.Layout", "Contract"}),
    # It imports no engine module (test_bridge_transport_imports_no_engine_module).
    "Artemis/4_Bridge/Transport": ("Artemis.Bridge.Transport.vcxproj", {"Common", "Services", "Contract", "Control", "Blam.Layout", "Bridge.Codec",
                                                                      "Bridge.Transport"},
                                 {"Common", "Services", "Contract", "Control", "Blam.Layout", "Bridge.Codec", "Blam.Map", "Blam.MapModel",
                                  "Blam.Runtime", "Perception", "Views"}),
    # The engine in five projects (M6c.2, D216). Engine folders each may import: ENGINE_PLATFORM.
    "Artemis/5_Engine/Base": ("Artemis.Engine.Base.vcxproj", {"Common", "Services", "Engine.Base"}, {"Common", "Services"}),
    "Artemis/5_Engine/Capture": ("Artemis.Engine.Capture.vcxproj", {"Common", "Services", "Blam.Layout", "Engine.Base", "Engine.Capture"},
                               {"Common", "Services", "Engine.Base", "Blam.Layout"}),
    "Artemis/5_Engine/Control": ("Artemis.Engine.Control.vcxproj", {"Common", "Services", "Control", "Blam.Layout", "Engine.Base", "Engine.Control"},
                               {"Common", "Services", "Engine.Base", "Control", "Blam.Layout"}),
    "Artemis/5_Engine/Match": ("Artemis.Engine.Match.vcxproj", {"Common", "Services", "Engine.Base", "Engine.Match"}, {"Common", "Services", "Engine.Base"}),
    "Artemis/5_Engine/Render": ("Artemis.Engine.Render.vcxproj", {"Common", "Services", "Engine.Base", "Engine.Render"}, {"Common", "Services", "Engine.Base"}),
    # The development tools (M7.2 D228, M8 A2 D238): Gui (ImGui backend and widgets), Viewer and Workbench (ex UI); the tools
    # are composed by Mod.Layer. They use the engine only through DevTools.EngineAccess (in Viewer).
    "Artemis/6_DevTools/Gui": ("Artemis.DevTools.Gui.vcxproj", {"Common", "Services", "Engine.Render", "DevTools.Gui"},
                               {"Common", "Services", "Engine.Base", "Engine.Render"}),
    "Artemis/6_DevTools/Viewer": ("Artemis.DevTools.Viewer.vcxproj", DEVTOOLS_IMPORTS, DEVTOOLS_REFERENCES | {"DevTools.Gui"}),
    "Artemis/6_DevTools/Workbench": ("Artemis.DevTools.Workbench.vcxproj", DEVTOOLS_IMPORTS | {"DevTools.Workbench"},
                                     DEVTOOLS_REFERENCES | {"DevTools.Gui", "DevTools.Viewer"}),
}

# The pure projects of M5b (D198): what was the Domain after Blam.Runtime. None of them touches the process, the wiring or a layer above.
PURE_PROJECTS = ("Artemis/1_Shared/Contract", "Artemis/3_Domain/Perception", "Artemis/3_Domain/Views", "Artemis/4_Bridge/Codec", "Artemis/3_Domain/Control")
NOT_PURE_ROOTS = {"Services", "Bridge.Transport", "Mod", "DevTools.Gui", "DevTools.Viewer", "DevTools.Workbench"} | ENGINE
NOT_PURE_WORDS = ("windows.h", "MinHook", "reinterpret_cast", "OnCleanup(", "OnEngineInitialized(", "OnUnhook(")
LAYERS = (("Perception", "Artemis/3_Domain/Perception/Perception.Layer.ixx",
           ("Log& log", "Blam::MapModel::Layer& resolved", "Blam::Runtime::Layer& tables")),
          ("Views", "Artemis/3_Domain/Views/Views.Layer.ixx",
           ("Log& log", "Blam::MapModel::Layer& resolved", "Blam::Runtime::Layer& tables", "Perception::Layer& environment")),
          ("Bridge.Codec", "Artemis/4_Bridge/Codec/Bridge.Codec.Layer.ixx",
           ("Blam::MapModel::Layer& resolved", "Blam::Runtime::Layer& tables", "Perception::Layer& environment",
            "Views::Layer& egocentric")))
TICK_DETOURS = ("ExecuteGameFrameDetour", "SimulationTicksDetour")

# The interpretation of the live objects (02 sec. 3.2): a project since M5a (D189), it reads only the Snapshot (D184).
RUNTIME = "Artemis/2_Blam/Runtime"

# The three locators of the engine tables (Engine.Capture).
LOCATORS = ("ObjectTableLocator", "PlayerTableLocator", "InteractionTableLocator")
CAPTURE_HOOK = "Artemis/5_Engine/Capture/Hook/"

# Outside Blam.Map and Blam.MapModel, the modules of Blam.Map (Blam.Map.*) are imported only by the wiring of the DLL (future Mod),
# the development tools (future DevTools) and the tests (02 sec. 3.5, contract of Blam.Map).
MAP_IMPORTERS = ("Artemis/8_Mod/Mod/Mod.Layer.ixx", "Artemis/8_Mod/Mod/Mod.Layer.cpp", "Artemis/8_Mod/Mod/Thread/", "Artemis/6_DevTools/", "Artemis/7_Tests/Tests/")

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


def PureSources():
    return [path for folder in PURE_PROJECTS for path in Sources(folder)]


def Read(path):
    with open(os.path.join(REPO, path), encoding="utf-8-sig", errors="replace") as f:
        return f.read()


def Component(module):
    """Project of a module: the longest project name that is a dotted prefix of it ("Blam.Map.Reader.Type" -> "Blam.Map")."""
    names = [name for _, _, name in PROJECTS if module == name or module.startswith(name + ".")]
    return max(names, key=len) if names else module.split(".")[0]


def ImportRoots(path):
    return {Component(name) for name in IMPORT.findall(Read(path)) if name != "std"}


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
    found = {ProjectName(include) for include in REFERENCE.findall(Read("Artemis/8_Mod/Mod/Artemis.Mod.vcxproj"))}
    assert {"Common", "Services", "Blam.Map", "Blam.MapModel", "Blam.Layout", "Blam.Runtime", "Contract", "Perception", "Views",
            "Bridge.Codec", "Control", "Bridge.Transport", "Engine.Base", "Engine.Capture", "Engine.Control", "Engine.Match",
            "Engine.Render"} <= found


def test_tests_project_references_the_blam_components():
    found = {ProjectName(include) for include in REFERENCE.findall(Read("Artemis/7_Tests/Tests/Artemis.Tests.vcxproj"))}
    assert {"Blam.Map", "Blam.MapModel", "Blam.Layout", "Blam.Runtime", "Contract", "Perception", "Views", "Bridge.Codec",
            "Control", "Bridge.Transport"} <= found


def test_old_map_folders_are_gone():
    assert Tracked("Artemis/Core/Domain/1_Map", "Artemis/Core/Domain/2_Resolved") == []


def test_only_wiring_tools_and_tests_import_blam_map():
    inside = ("Artemis/2_Blam/Map/", "Artemis/2_Blam/MapModel/")
    wrong = [path for path in Sources("Artemis") if not path.startswith(inside) and not path.startswith(MAP_IMPORTERS)
             and "Blam.Map" in ImportRoots(path)]
    assert not wrong, f"import Blam.Map.* outside the wiring, the tools and the tests: {wrong}"


def test_tag_resolver_split():
    in_map = [p for p in Sources("Artemis/2_Blam/Map") if "TagResolverService" in Read(p)]
    in_model = [p for p in Sources("Artemis/2_Blam/MapModel") if "TagReferenceService" in Read(p) and "TagResolver" in p]
    assert not in_map, f"TagResolverService is of Blam.MapModel: {in_map}"
    assert not in_model, f"the resolver of Blam.MapModel does not use TagReferenceService: {in_model}"
    definitions = [p for p in Sources("Artemis") if re.search(r"\bclass\s+TagResolverService\b", Read(p))]
    assert len(definitions) == 1 and definitions[0].startswith("Artemis/2_Blam/MapModel/"), definitions


def test_unused_group_queries_are_removed():
    result = subprocess.run(["git", "-C", REPO, "grep", "--untracked", "-n", "-w", "-E", "|".join(REMOVED_NAMES), "--",
                             "Artemis"], capture_output=True, text=True)
    assert result.stdout == ""


def test_the_engine_hook_of_the_map_lives_in_platform():
    hooks = [p for p in Sources("Artemis") if os.path.basename(p).startswith("OpenMapDetour.")]
    assert sorted(os.path.basename(p) for p in hooks) == ["OpenMapDetour.cpp", "OpenMapDetour.ixx"]
    assert all(p.startswith("Artemis/5_Engine/Base/Lifecycle/Hook/") for p in hooks), hooks
    for path in Sources("Artemis/2_Blam/Map") + Sources("Artemis/2_Blam/MapModel"):
        text = Read(path)
        assert "windows.h" not in text and "GetModuleFileName" not in text, path


def test_old_datum_folder_is_gone():
    assert Tracked("Artemis/Core/Domain/3_Datum") == []


def test_engine_offsets_left_tables():
    moved = [p for p in Sources(RUNTIME)
             if os.path.basename(p) in ("PlayerOffset.ixx", "PlayerMapping.ixx", "PlayerSize.ixx", "InteractionOffset.ixx",
                                        "GameOffset.ixx")]
    assert not moved, moved
    old = re.compile(r"Tables::(Player|Interaction|Game)::Type::(Offset|Mapping|Size)::")
    wrong = [p for p in Sources("Artemis") if old.search(Read(p))]
    assert not wrong, f"old namespace of the offsets: {wrong}"


def test_raw_snapshot_lives_in_blam_layout():
    wrong = [p for p in Sources("Artemis") if "Blam::Layout::Object::Type::Raw" in Read(p) or "DatumStore" in Read(p)]
    assert not wrong, wrong
    definitions = [p for p in Sources("Artemis") if re.search(r"\bstruct\s+RawObject\b", Read(p))]
    assert len(definitions) == 1 and definitions[0].startswith("Artemis/2_Blam/Layout/"), definitions


def test_old_tables_folder_is_gone():
    assert Tracked("Artemis/Core/Domain/4_Tables") == []


def test_blam_runtime_does_not_touch_the_process():
    sources = Sources(RUNTIME)
    assert sources, f"{RUNTIME} has no sources"
    for path in sources:
        text = Read(path)
        for word in ("MemoryReaderService", "reader.Read", "windows.h", "reinterpret_cast", "OnCleanup"):
            assert word not in text, f"{path}: {word}"
        assert not re.search(r"(?<![\w:])(Engine|Services|Mod)::", text), f"{path}: the DLL wiring"


def test_tables_hold_no_process_address():
    member = re.compile(r"std::uintptr_t\s+\w+\s*\{\s*\}")
    for path in Sources(RUNTIME):
        text = Read(path)
        assert not member.search(text), f"{path}: a member of type std::uintptr_t"
        assert not re.search(r"\b(Address|BaseAddress)\b", text), path
    wrong = [p for p in PureSources() if re.search(r"\.Address\b", Read(p))]
    assert not wrong, f"the Domain checks an address of the process: {wrong}"


def test_dev_screens_take_the_address_from_the_snapshot():
    objects = Read("Artemis/6_DevTools/Workbench/ObjectTable/System/ObjectTableUIService.cpp")
    players = Read("Artemis/6_DevTools/Workbench/PlayerTable/System/PlayerTableUIService.cpp")
    assert "RawObjects" in objects and "object.Address" not in objects
    assert "OriginalAddress" in players and "player.Address" not in players


def test_wide_to_utf8_is_portable_and_lives_in_common():
    definitions = [p for p in Sources("Artemis") if p.endswith(".ixx") and re.search(r"\bauto\s+WideToUtf8\s*\(", Read(p))]
    assert definitions == ["Artemis/1_Shared/Common/Text/System/Common.Text.System.ixx"], definitions
    wrong = [p for p in Sources("Artemis") if "WideCharToMultiByte" in Read(p) and not p.startswith("Artemis/7_Tests/Tests/")]
    assert not wrong, wrong


def test_tables_layer_is_pure_and_core_cleans_it():
    layer = Read(f"{RUNTIME}/Blam.Runtime.Layer.ixx")
    assert re.search(r"\bLayer\(\s*Log&\s+log\s*,\s*Blam::MapModel::Layer&\s+resolved\s*\)", layer)
    assert re.search(r"auto\s+Cleanup\(\)\s*->\s*void", layer)
    assert "m_Tables.Cleanup()" in Read("Artemis/8_Mod/Mod/Mod.Layer.cpp")


def test_locators_live_in_platform_capture():
    found = [p for p in Sources("Artemis") if os.path.basename(p).split(".")[0] in LOCATORS]
    assert sorted(os.path.basename(p) for p in found) == sorted(f"{name}.{ext}" for name in LOCATORS for ext in ("cpp", "ixx"))
    assert all(p.startswith(CAPTURE_HOOK) for p in found), found


def test_max_local_users_has_one_owner():
    definitions = [p for p in Sources("Artemis") if re.search(r"\bk_MaxLocalUsers\s*\{", Read(p))]
    assert definitions == ["Artemis/1_Shared/Common/Engine/Type/Common.Engine.Type.ixx"], definitions
    wrong = [p for p in Sources("Artemis") if "Engine::Base::Host::Type::k_MaxLocalUsers" in Read(p)]
    assert not wrong, wrong


def test_blam_layout_does_not_touch_the_process():
    for path in Sources("Artemis/2_Blam/Layout"):
        text = Read(path)
        assert "windows.h" not in text and "std::uintptr_t*" not in text and "reinterpret_cast" not in text, path


def test_shadow_mode_is_gone():
    assert Tracked("Artemis/Core/Domain/4_Tables/Shadow") == []
    wrong = [p for p in Sources("Artemis") if re.search(r"\bShadow(Service|PlayerTableService|InteractionService|GameService)\b",
                                                         Read(p))]
    assert not wrong, wrong


def test_the_domain_does_not_read_the_game():
    wrong = [p for p in PureSources() if "MemoryReaderService" in Read(p)]
    assert not wrong, wrong


def test_tick_trace_reads_its_clock_from_the_snapshot():
    for path in Sources("Artemis/6_DevTools/Workbench/TickTrace"):
        text = Read(path)
        assert "ReadValue" not in text and "GetThreadContext" not in text and "reinterpret_cast" not in text, path


def test_read_window_ends_right_after_the_copy():
    text = Read("Artemis/8_Mod/Mod/Thread/System/AI/AIThread.cpp")
    capture = text.index("m_CaptureService.Capture()")
    end_read = text.index("EndRead(m_Last)")
    # The first table update is the first stage of the pipeline (M5c, D205).
    first_table = text.index("m_PipelineService.Run(")
    assert capture < end_read < first_table


# ---- M5b: Perception, Views, Bridge.Codec and Contract (D198-D201)

def test_old_domain_folder_is_gone():
    assert Tracked("Artemis/Core/Domain") == []
    assert not os.path.exists(os.path.join(REPO, "Artemis/Core/Domain"))


def test_pure_projects_do_not_touch_the_process_or_the_wiring():
    sources = PureSources()
    assert sources
    for path in sources:
        text = Read(path)
        assert not (ImportRoots(path) & NOT_PURE_ROOTS), f"{path}: imports {sorted(ImportRoots(path) & NOT_PURE_ROOTS)}"
        for word in NOT_PURE_WORDS:
            assert word not in text, f"{path}: {word}"


def test_contract_is_the_generated_bridge_layout():
    sources = Sources("Artemis/1_Shared/Contract")
    assert sources == ["Artemis/1_Shared/Contract/Contract.Bridge.Type.ixx"], sources
    text = Read(sources[0])
    assert text.splitlines()[0].startswith("// Generated by"), text.splitlines()[0]
    assert re.search(r"^export module Contract\.Bridge\.Type;", text, re.MULTILINE)
    assert ImportRoots(sources[0]) <= {"Common"}


def test_bridge_type_reexports_the_contract_and_the_clock_moved_to_flat():
    bridge = Read("Artemis/4_Bridge/Transport/Type/Bridge.Transport.Type.ixx")
    assert "export import Contract.Bridge.Type;" in bridge
    assert not re.search(r"\bstruct\s+BridgeClock\b", bridge)
    assert re.search(r"\bstruct\s+BridgeClock\b", Read("Artemis/4_Bridge/Codec/Flat/Type/Bridge.Codec.Flat.Type.ixx"))
    assert not Tracked("Artemis/4_Bridge/Transport/Type/BridgeLayout.ixx")


def test_tick_detours_live_in_platform_capture_hook():
    found = [p for p in Sources("Artemis") if os.path.basename(p).split(".")[0] in TICK_DETOURS]
    assert sorted(os.path.basename(p) for p in found) == sorted(f"{name}.{ext}" for name in TICK_DETOURS for ext in ("cpp", "ixx"))
    assert all(p.startswith(CAPTURE_HOOK) for p in found), found
    for name, part in (("ExecuteGameFrameDetour", "ExecuteGameFrame"), ("SimulationTicksDetour", "SimulationTicks")):
        assert Read(f"{CAPTURE_HOOK}{name}.ixx").startswith(f"export module Engine.Capture.Hook:{part};")
        assert f"export import :{part};" in Read(CAPTURE_HOOK + "../Engine.Capture.Hook.ixx")
    layer = Read("Artemis/8_Mod/Mod/Mod.Layer.cpp")
    for name in TICK_DETOURS:
        assert f"m_{name}.Install();" in layer and f"m_{name}.Uninstall();" in layer
    assert not [p for p in Tracked("Artemis") if os.path.basename(p) == "Bridge.Codec.Tick.Hook.ixx"]
    for path in Sources("Artemis/4_Bridge/Codec"):
        text = Read(path)
        for word in ("ExecuteGameFrameFunction", "SimulationTicksFunction", "EndRoundFunction"):
            assert word not in text, f"{path}: {word}"


@pytest.mark.parametrize("name,path,parameters", LAYERS)
def test_layers_are_pure_with_a_cleanup(name, path, parameters):
    text = Read(path)
    constructor = r"\bLayer\(\s*" + r"\s*,\s*".join(re.escape(p).replace(r"\ ", r"\s+") for p in parameters) + r"\s*\)"
    assert re.search(constructor, text), f"{name}: constructor"
    assert re.search(r"auto\s+Cleanup\(\)\s*->\s*void", text), f"{name}: Cleanup"
    assert "Services::Layer" not in text and "Platform::Layer" not in text and "OnCleanup" not in text, name


def test_core_cleans_the_seven_layers_in_order():
    text = Read("Artemis/8_Mod/Mod/Mod.Layer.cpp")
    order = ["m_Environment", "m_Egocentric", "m_Export", "m_Tables", "m_Map", "m_Resolved"]
    positions = [text.index(f"{member}.Cleanup()") for member in order]
    assert positions == sorted(positions), positions


def test_generate_bridge_writes_the_contract():
    text = Read("Tools/TagTranspiler/GenerateBridge.py")
    assert '"Artemis", "1_Shared", "Contract", "Contract.Bridge.Type.ixx"' in text
    assert '"export module Contract.Bridge.Type;"' in text
    assert "BridgeLayout.ixx" not in text


def test_the_bridge_layout_has_the_flags_the_stands_and_the_round_of_b4h():
    from tdump_writer import RealLayout
    layout, _ = RealLayout()
    structs = {s["name"]: {f[0]: f for f in s["fields"]} for s in layout["structs"]}
    assert {"Handle", "CarrierPlayer", "CarrierBiped", "HomeStand", "ResetTicks", "BaseTeam", "OwnerTeam", "State", "IsResetRunning",
            "AlliesInZone"} == set(structs["BridgeObjective"])
    assert {"Handle", "BaseTeam", "OwnerTeam"} == set(structs["BridgeStand"])
    assert {"ObjectiveCount", "StandCount"} <= set(structs["BridgeTickHeader"])
    assert structs["BridgeTick"]["Objectives"][1:4:2] == ["BridgeObjective", 8]
    assert structs["BridgeTick"]["Stands"][1:4:2] == ["BridgeStand", 8]
    assert [structs["BridgeMatch"][n][3] for n in ("TeamRoundScores", "TeamTotalScores")] == [8, 8]
    assert {"RoundIndex", "RoundState"} <= set(structs["BridgeMatch"])
    assert "EngineSequence" in structs["BridgeEvent"]
    # The counts of the arrays are the ones of the C++ constants (Common.Engine k_MaxTeams) and the contract.
    contract = Read("Artemis/1_Shared/Contract/Contract.Bridge.Type.ixx")
    assert "k_BridgeMaxObjectives{ 8 }" in contract and "k_MaxTeams == 8" in contract


# --- M5c (D202-D207) ------------------------------------------------------------------------------------------------------------

# Calls that run one step of the tick: only PipelineService.Run does it (D205).
PIPELINE_STEP = re.compile(r"\.m_\w+Service\.(?:Update\w*|Assemble)\(")
SNAPSHOT_CODEC = "Artemis/2_Blam/Layout/Snapshot/System/Blam.Layout.Snapshot.System"
DUMP_CODEC = "Artemis/4_Bridge/Codec/Dump/"
PIPELINE_SERVICE = "Artemis/4_Bridge/Codec/Pipeline/System/PipelineService"


def test_common_hash_does_not_touch_windows():
    sources = Sources("Artemis/1_Shared/Common/Hash")
    assert sorted(os.path.basename(p) for p in sources) == ["Common.Hash.System.cpp", "Common.Hash.System.ixx"], sources
    for path in sources:
        assert "windows.h" not in Read(path), path
        assert ImportRoots(path) == {"Common"} or ImportRoots(path) == set(), path
    assert re.search(r"^export module Common\.Hash\.System;", Read("Artemis/1_Shared/Common/Hash/System/Common.Hash.System.ixx"), re.MULTILINE)


def test_tests_support_maps_takes_the_crc_from_common():
    text = Read("Artemis/7_Tests/Tests/Support/Tests.Support.Maps.ixx")
    assert "import Common.Hash.System;" in text
    assert "k_Crc32Polynomial" not in text and "MakeCrc32Table" not in text


def test_snapshot_codec_imports_only_what_its_project_allows():
    paths = [SNAPSHOT_CODEC + ".ixx", SNAPSHOT_CODEC + ".cpp"]
    assert sorted(Tracked(*paths)) == sorted(paths)
    assert "Artemis/2_Blam/Layout/Snapshot/Type/SnapshotRecord.ixx" in Tracked("Artemis/2_Blam/Layout")
    for path in paths + ["Artemis/2_Blam/Layout/Snapshot/Type/SnapshotRecord.ixx"]:
        assert ImportRoots(path) <= {"Common", "Blam.Layout"}, (path, ImportRoots(path))


def test_dump_codec_and_pipeline_import_only_what_their_project_allows():
    dump = [p for p in Sources("Artemis/4_Bridge/Codec") if p.startswith(DUMP_CODEC)]
    assert sorted(os.path.basename(p) for p in dump) == ["Bridge.Codec.Dump.System.cpp", "Bridge.Codec.Dump.System.ixx", "Bridge.Codec.Dump.Type.ixx"], dump
    for path in dump:
        assert ImportRoots(path) <= {"Common", "Contract", "Bridge.Codec"}, (path, ImportRoots(path))
    pipeline = [PIPELINE_SERVICE + ".ixx", PIPELINE_SERVICE + ".cpp"]
    assert sorted(Tracked(*pipeline)) == sorted(pipeline)
    for path in pipeline:
        assert ImportRoots(path) <= {"Blam.Layout", "Blam.Runtime", "Perception", "Views", "Bridge.Codec"}, (path, ImportRoots(path))
    assert "export module Bridge.Codec.Pipeline.System;" in Read(pipeline[0])
    assert re.search(r"\bPipelineService\s+m_PipelineService\b", Read("Artemis/4_Bridge/Codec/Bridge.Codec.Layer.ixx"))


def test_the_order_of_the_tick_lives_in_the_pipeline_service():
    thread = Read("Artemis/8_Mod/Mod/Thread/System/AI/AIThread.cpp")
    assert not PIPELINE_STEP.search(thread), PIPELINE_STEP.search(thread).group(0)
    assert "m_PipelineService.Run(" in thread
    tests = Read("Artemis/7_Tests/Tests/Perception/PipelineTests.cpp")
    assert not PIPELINE_STEP.search(tests), PIPELINE_STEP.search(tests).group(0)
    assert "m_PipelineService.Run(" in tests
    # Tables 4, Relations 2, Environment 12 and Egocentric 3. The Tick is assembled through TickService.
    assert len(PIPELINE_STEP.findall(Read(PIPELINE_SERVICE + ".cpp"))) == 21


def test_the_tdump_format_lives_in_bridge_codec_not_in_the_ui():
    ui = Read("Artemis/6_DevTools/Workbench/TickDump/Type/DevTools.Workbench.TickDump.Type.ixx")
    assert not re.search(r"\bstruct\s+(?:TickRecordHeader|TickDumpFileHeader|TickDumpFooter)\b", ui)
    assert "TickRecordHeader" not in ui
    assert re.search(r"\bstruct\s+TickRecordHeader\b", Read("Artemis/4_Bridge/Codec/Dump/Type/Bridge.Codec.Dump.Type.ixx"))
    for path in Sources("Artemis/6_DevTools/Workbench/TickDump"):
        assert "DevTools::Workbench::TickDump::Type::TickRecordHeader" not in Read(path), path


# ---- M6a: Control as a project (D210-D215)

INPUT_TYPE = "Artemis/5_Engine/Control/Input/Type/Engine.Control.Input.Type.ixx"
SIMULATION_TYPE = "Artemis/5_Engine/Control/Simulation/Type/Engine.Control.Simulation.Type.ixx"
DETOUR_CPP = "Artemis/5_Engine/Control/Input/Hook/PlayerActionDetour.cpp"
BRIDGE_SERVICE_CPP = "Artemis/4_Bridge/Transport/System/BridgeService.cpp"
BOT_SERVICE_IXX = "Artemis/5_Engine/Control/Simulation/System/BotService.ixx"
CONTROL_NOT_PURE_WORDS = ("std::mutex", "std::thread", "std::chrono")


def Defines(path_text, kind, name):
    return re.search(rf"\b{kind}\s+{name}\b\s*(?:\{{|$)", path_text, re.MULTILINE) is not None


def test_the_action_format_lives_in_blam_layout():
    sources = Sources("Artemis/2_Blam/Layout/Action")
    assert sorted(sources) == ["Artemis/2_Blam/Layout/Action/Blam.Layout.Action.Type.ixx", "Artemis/2_Blam/Layout/Action/Type/ActionBits.ixx",
                               "Artemis/2_Blam/Layout/Action/Type/ActionOffset.ixx", "Artemis/2_Blam/Layout/Action/Type/ActionSize.ixx"]
    for partition, name in (("Offset", "ActionOffset"), ("Size", "ActionSize"), ("Bits", "ActionBits")):
        assert Read(f"Artemis/2_Blam/Layout/Action/Type/{name}.ixx").startswith(f"export module Blam.Layout.Action.Type:{partition};")
        assert f"export import :{partition};" in Read("Artemis/2_Blam/Layout/Action/Blam.Layout.Action.Type.ixx")
    for path in (INPUT_TYPE, SIMULATION_TYPE):
        text = Read(path)
        for name in ("k_PlayerActionSize", "k_ActionButtonsOffset", "k_HoldLanes", "k_NoZoomLevel"):
            assert not re.search(rf"\b{name}\s*\{{", text), f"{path} defines {name}"
        assert not Defines(text, "struct", "HoldLane"), path


@pytest.mark.parametrize("name", ["ActionOverride", "ClientRequests", "WeaponRequest", "AppliedFacing", "UnitClientState",
                                  "PlayerActionHistory", "BridgeControlState", "BridgeHookState"])
def test_control_types_are_defined_only_under_control(name):
    found = [path for path in Sources("Artemis") if Defines(Read(path), "struct", name)]
    assert found and all(path.startswith("Artemis/3_Domain/Control/") for path in found), found


def test_ticks_per_second_is_defined_only_in_common_engine_type():
    found = [path for path in Sources("Artemis") if re.search(r"\bk_TicksPerSecond\s*\{", Read(path))]
    assert found == ["Artemis/1_Shared/Common/Engine/Type/Common.Engine.Type.ixx"], found


def test_the_detour_builds_the_action_with_control():
    text = Read(DETOUR_CPP)
    assert "BuildAction(" in text
    for word in ("TurnToward", "MoveToward", "WriteLanes", "WriteClientRequests", "IsUseTestDown", "k_WeaponRequestTicks"):
        assert word not in text, word


def test_the_bridge_service_takes_its_decisions_from_control():
    text = Read(BRIDGE_SERVICE_CPP)
    for name in ("DecideControl", "BuildOverride", "Finite", "Clamp"):
        assert not re.search(rf"\bauto\s+(?:\w+::)?{name}\s*\(", text), f"defines {name}"
    for name in ("GetDueDecision", "RecordWait", "ReadClock"):
        assert f"{name}(" in text, name


def test_the_bot_service_answers_the_unit_rules():
    assert re.search(r"class\s+BotService\s*:\s*public\s+(?:::)?Control::Action::System::UnitRules\b", Read(BOT_SERVICE_IXX))


def test_control_does_not_touch_the_process_the_wiring_or_the_clock():
    sources = Sources("Artemis/3_Domain/Control")
    assert sources
    for path in sources:
        text = Read(path)
        assert not (ImportRoots(path) & NOT_PURE_ROOTS), f"{path}: imports {sorted(ImportRoots(path) & NOT_PURE_ROOTS)}"
        for word in NOT_PURE_WORDS + CONTROL_NOT_PURE_WORDS:
            assert word not in text, f"{path}: {word}"


# ---- M6b: Bridge.Transport as a project and the ports between the engine and the bridge (D215, D217)

TRANSPORT = "Artemis/4_Bridge/Transport"
PLATFORM = "Artemis/8_Mod/Mod/Port"
ENGINE_ROOTS = ("Engine.", "Mod.", "DevTools.")
VIRTUAL_METHOD = re.compile(r"\bvirtual\s+auto\s+(\w+)\s*\(")


def Imports(path):
    return [name for name in IMPORT.findall(Read(path)) if name != "std"]


def test_the_old_bridge_folder_of_platform_is_gone():
    old = "Artemis/8_Mod/Mod/Infrastructure/Platform/Bridge"
    assert not Tracked(old)
    assert not os.path.exists(os.path.join(REPO, old))


def test_bridge_transport_imports_no_engine_module():
    sources = Sources(TRANSPORT)
    assert sources
    for path in sources:
        engine = [name for name in Imports(path) if name.startswith(ENGINE_ROOTS)]
        assert not engine, f"{path}: imports {engine}"


def test_only_the_layer_and_the_port_import_the_bridge_in_platform():
    allowed = (f"{PLATFORM}/",)
    found = [path for path in Sources(PLATFORM) if any(name.startswith("Bridge.Transport.") for name in Imports(path))]
    assert found, "nobody imports the bridge"
    wrong = [path for path in found if not path.startswith(allowed)]
    assert not wrong, wrong


def test_the_bridge_service_and_the_detour_know_only_the_ports():
    service = Read(f"{TRANSPORT}/System/BridgeService.ixx")
    for name in ("LifecycleStore", "MatchCycleStore", "PlayerActionStore"):
        assert not re.search(rf"\b{name}\b", service), name
    assert "BridgeEngine& engine" in service
    assert not re.search(r"\bBridgeService\b", Read("Artemis/5_Engine/Control/Input/Hook/PlayerActionDetour.ixx"))
    assert "m_ActionSource->Prepare(" in Read(DETOUR_CPP)


def test_the_ports_declare_exactly_their_methods():
    engine = Read(f"{TRANSPORT}/System/BridgeEngine.ixx")
    assert re.search(r"^export module Bridge\.Transport\.System:Engine;", engine, re.MULTILINE)
    assert re.search(r"\bclass\s+BridgeEngine\b", engine)
    assert set(VIRTUAL_METHOD.findall(engine)) == {"GetPhase", "GetEngineStarts", "GetPausedFrames", "GetRequestedTimeScale",
                                                   "SetRequestedTimeScale", "IsPauseRequested", "SetPauseRequested", "SetEndRoundRequested",
                                                   "SetOverride", "RequestClient"}
    source = Read("Artemis/5_Engine/Control/Input/System/ActionSource.ixx")
    assert re.search(r"^export module Engine\.Control\.Input\.System:ActionSource;", source, re.MULTILINE)
    assert re.search(r"\bclass\s+ActionSource\b", source)
    assert set(VIRTUAL_METHOD.findall(source)) == {"Prepare"}


# ---- M6c.1: the old input is retired (L.1, D220) and the match and window modules leave Lifecycle and Input (D219)

REMOVED_INPUT_NAMES = ("GetButtonState", "InputStore", "MouseDeltaStore", "InputAction", "SetAIControlActive", "InjectMouseDelta",
                       "SetActionRequested", "AdvanceInputTick", "InputThread", "IsAIControlActive", "ConsumePendingDelta",
                       "AddPendingDelta", "KnownDeviceHandle")
CONTROL_INPUT = "Artemis/5_Engine/Control/Input"
REMOVED_INPUT_FILES = (f"{CONTROL_INPUT}/Hook/GetButtonStateDetour.ixx", f"{CONTROL_INPUT}/Hook/GetButtonStateDetour.cpp",
                       f"{CONTROL_INPUT}/State/InputStore.ixx", f"{CONTROL_INPUT}/State/InputStore.cpp",
                       f"{CONTROL_INPUT}/State/MouseDeltaStore.ixx", f"{CONTROL_INPUT}/State/MouseDeltaStore.cpp")
CORE_LIFECYCLE = "Artemis/5_Engine/Base/Lifecycle"
MATCH = "Artemis/5_Engine/Match"
WINDOW = "Artemis/5_Engine/Render/Window"
MATCH_CLASSES = {"EngineInterfaceDetour", "LifecycleUpdateDetour", "MatchConfigStore", "MatchCycleStore", "MatchConfigService"}
WINDOW_CLASSES = {"WndProcDetour", "GetRawInputDataDetour", "InputService"}
CLASS_DECLARATION = re.compile(r"^\s*class\s+(\w+)", re.MULTILINE)


def DeclaredClasses(folder):
    return {name for path in Sources(folder) for name in CLASS_DECLARATION.findall(Read(path))}


def test_the_old_input_is_gone():
    result = subprocess.run(["git", "-C", REPO, "grep", "--untracked", "-n", "-w", "-E", "|".join(REMOVED_INPUT_NAMES), "--",
                             "Artemis"], capture_output=True, text=True)
    assert result.stdout == ""
    for relative in REMOVED_INPUT_FILES:
        assert not Tracked(relative), relative
        assert not os.path.exists(os.path.join(REPO, relative)), relative
    assert not Tracked("Artemis/8_Mod/Mod/Runtime/Input")
    assert not os.path.exists(os.path.join(REPO, "Artemis/8_Mod/Mod/Runtime/Input"))


def test_the_match_and_window_modules_declare_their_classes():
    assert DeclaredClasses(MATCH) - {"Layer"} == MATCH_CLASSES
    assert DeclaredClasses(WINDOW) == WINDOW_CLASSES
    assert not DeclaredClasses(CORE_LIFECYCLE) & MATCH_CLASSES
    assert not DeclaredClasses(CONTROL_INPUT) & WINDOW_CLASSES
    for root, prefix, name in ((MATCH, "Engine.Match", "Hook"), (MATCH, "Engine.Match", "State"), (MATCH, "Engine.Match", "System"),
                               (MATCH, "Engine.Match", "Type"), (WINDOW, "Engine.Render.Window", "Hook"),
                               (WINDOW, "Engine.Render.Window", "System"), (WINDOW, "Engine.Render.Window", "Type")):
        module = f"{prefix}.{name}"
        path = f"{root}/{name}/{module}.ixx" if name == "Type" else f"{root}/{module}.ixx"
        assert re.search(rf"^export module {re.escape(module)};", Read(path), re.MULTILINE), path


def test_the_types_of_match_and_window_left_lifecycle_and_input():
    lifecycle = Read(f"{CORE_LIFECYCLE}/Type/Engine.Base.Lifecycle.Type.ixx")
    match = Read(f"{MATCH}/Type/Engine.Match.Type.ixx")
    for name in ("MatchConfig", "k_TransplantBlocks", "k_EngineConfigurationSize", "LifecycleUpdateFunction", "k_MaxMatchConfigNameLength"):
        assert not re.search(rf"\b{name}\b", lifecycle), name
        assert re.search(rf"\b{name}\b", match), name
    assert re.search(r"\bStatus\b", lifecycle)
    input_types = Read(f"{CONTROL_INPUT}/Type/Engine.Control.Input.Type.ixx")
    window = Read(f"{WINDOW}/Type/Engine.Render.Window.Type.ixx")
    for name in ("WindowMessage", "RawMouse", "WindowMessageHandler", "RawMouseHandler", "GetRawInputDataFunction"):
        assert not re.search(rf"\b{name}\b", input_types), name
        assert re.search(rf"\b{name}\b", window), name


def test_the_match_window_and_input_modules_import_in_one_direction():
    for path in Sources(CORE_LIFECYCLE):
        assert not [n for n in Imports(path) if n.startswith("Engine.Match.")], path
    for path in Sources(CONTROL_INPUT):
        assert not [n for n in Imports(path) if n.startswith("Engine.Render.Window.")], path
    for path in Sources(WINDOW):
        assert not [n for n in Imports(path) if n.startswith("Engine.Control.Input.")], path


# ---- M6c.2: the engine in five projects (D216, D217, D221)

ENGINE_PLATFORM = {
    "Artemis/5_Engine/Base": {"Hook", "Memory", "Host", "Lifecycle"},
    "Artemis/5_Engine/Capture": {"Hook", "Memory", "Host", "Lifecycle", "Capture"},
    "Artemis/5_Engine/Control": {"Hook", "Memory", "Host", "Lifecycle", "Input", "Simulation"},
    "Artemis/5_Engine/Match": {"Hook", "Memory", "Host", "Lifecycle", "Match"},
    "Artemis/5_Engine/Render": {"Hook", "Memory", "Host", "Lifecycle", "Render", "Window"},
}
ENGINE_FORBIDDEN_PREFIXES = ("Bridge.Transport.", "Mod.", "DevTools.", "Bridge.Codec.", "Perception.", "Views.", "Blam.Runtime.", "Blam.Map.",
                             "Blam.MapModel.")


def EngineFolder(module):
    """Folder of an engine module inside its project ("Engine.Base.Hook.System" -> "Hook", "Engine.Capture.Type" -> "Capture");
    None for the Layer of a project."""
    if module.endswith(".Layer"):
        return None
    component = Component(module)
    rest = module.split(".")[len(component.split(".")):]
    return rest[0] if len(rest) > 1 else component.split(".")[1]
MODULE_LINE = re.compile(r"^\s*(?:export\s+)?module\s+([A-Za-z_][\w.]*)", re.MULTILINE)


@pytest.mark.parametrize("folder", sorted(ENGINE_PLATFORM))
def test_an_engine_project_imports_only_its_platform_folders(folder):
    for path in Sources(folder):
        found = {EngineFolder(name) for name in Imports(path) if name.startswith("Engine.")} - {None}
        assert found <= ENGINE_PLATFORM[folder], f"{path}: {sorted(found - ENGINE_PLATFORM[folder])}"


@pytest.mark.parametrize("folder", sorted(ENGINE_PLATFORM))
def test_an_engine_project_is_a_platform_library_that_knows_no_upper_layer(folder):
    for path in Sources(folder):
        module = MODULE_LINE.search(Read(path))
        assert module and module.group(1).startswith(f"Engine.{os.path.basename(folder)}."), path
        wrong = [name for name in Imports(path) if name.startswith(ENGINE_FORBIDDEN_PREFIXES)]
        assert not wrong, f"{path}: imports {wrong}"


def test_the_dll_keeps_only_the_layer_and_the_port_of_platform():
    assert Tracked(PLATFORM)
    assert not Tracked("Artemis/8_Mod/Mod/Infrastructure")
    assert not os.path.exists(os.path.join(REPO, "Artemis/8_Mod/Mod/Infrastructure"))


def test_the_dll_and_the_tests_reference_the_five_engine_projects():
    engine = {"Engine.Base", "Engine.Capture", "Engine.Control", "Engine.Match", "Engine.Render"}
    for project in ("Artemis/8_Mod/Mod/Artemis.Mod.vcxproj", "Artemis/7_Tests/Tests/Artemis.Tests.vcxproj"):
        found = {ProjectName(include) for include in REFERENCE.findall(Read(project))}
        assert engine <= found, project
    solution = Read("Artemis-MCC.slnx")
    for name in engine:
        assert f"Artemis.{name}.vcxproj" in solution, name


def test_the_dll_does_not_list_the_sources_of_the_engine_projects():
    listed = {include.replace("\\", "/") for include in COMPILE.findall(Read("Artemis/8_Mod/Mod/Artemis.Mod.vcxproj"))}
    assert not [p for p in listed if "/" in p and p.split("/")[0] not in ("Port", "Thread", "Proxy")]


# ---- M7.1: tick observers, the scene apart from the pipeline, the Gui without Domain, EngineAccess (D222-D227)

DEVTOOLS = "Artemis/6_DevTools"
DEVTOOLS_ROOTS = {"DevTools.Gui", "DevTools.Viewer", "DevTools.Workbench"}
PURE_ROOTS = {"Blam.Map", "Blam.MapModel", "Blam.Layout", "Blam.Runtime", "Perception", "Views", "Bridge.Codec", "Contract", "Control"}
DEVTOOLS_PURE_SYSTEMS = {"Bridge.Codec.Dump.System", "Bridge.Codec.Flat.System", "Blam.Layout.Snapshot.System", "Perception.Collidable.System",
                         "Blam.MapModel.Definitions.Reflect"}


def DevToolsSources():
    return Sources(DEVTOOLS)


def test_the_ai_thread_knows_no_development_tool():
    for path in Sources("Artemis/8_Mod/Mod/Thread/System/AI"):
        assert not ImportRoots(path) & DEVTOOLS_ROOTS, path
    text = Read("Artemis/8_Mod/Mod/Thread/System/AI/AIThread.ixx") + Read("Artemis/8_Mod/Mod/Thread/System/AI/AIThread.cpp")
    for name in ("CameraStore", "TickTraceUIService", "TickDumpUIService"):
        assert name not in text, name


def test_the_pipeline_builds_no_scene_and_knows_no_viewer_state():
    names = re.search(r"k_PipelineStageNames\{(.*?)\};", Read(PIPELINE_SERVICE + ".ixx"), re.DOTALL).group(1)
    assert "Collidable" not in names
    assert "CollidableService" not in Read(PIPELINE_SERVICE + ".cpp")
    for folder in ("Artemis/4_Bridge/Codec", "Artemis/3_Domain/Perception", "Artemis/3_Domain/Views", "Artemis/7_Tests/Tests/Support"):
        for path in Sources(folder):
            assert "isViewerActive" not in Read(path), path


def test_the_collidable_scene_is_built_on_demand_and_stored_by_the_viewer():
    service = Read("Artemis/3_Domain/Perception/Collidable/System/CollidableService.ixx")
    assert "BuildScene" in service
    assert not re.search(r"\bUpdate\(", service)
    assert not re.search(r"\bCollectCollidables\b", service)
    stores = [p for p in Sources("Artemis") if re.search(r"\bclass CollidableStore\b", Read(p))]
    assert stores == ["Artemis/6_DevTools/Viewer/Collidable/State/CollidableStore.ixx"], stores
    for path in Sources("Artemis/3_Domain/Perception"):
        assert "Perception.Collidable.State" not in Read(path), path
        assert not re.search(r"\bCollidableStore\b", Read(path)), path


def test_the_gui_has_no_domain_and_no_platform_layer():
    for path in Sources("Artemis/6_DevTools/Gui"):
        names = [n for n in Imports(path)]
        assert {Component(n) for n in names} <= {"Common", "Services", "DevTools.Gui", "Engine.Render"}, (path, names)
    assert not [p for p in Tracked("Artemis") if "ObjectFormater" in p]
    assert not [p for p in Tracked("Artemis/6_DevTools/Gui") if "AffordanceFormater" in p or "InteractionFormater" in p]
    for name in ("Affordance", "Interaction"):
        assert Tracked(f"Artemis/6_DevTools/Viewer/Format/System/{name}/{name}Formater.ixx")


def test_the_development_tools_do_not_import_the_engine_the_runtime_or_the_pipeline():
    for path in DevToolsSources():
        names = Imports(path)
        wrong = [n for n in names if n.startswith(("Mod.", "Bridge.Codec.Pipeline.System"))]
        assert not wrong, f"{path}: {wrong}"
        text = Read(path)
        assert "PipelineService" not in text and "BuildForMap" not in text, path


def test_the_development_tools_import_only_the_data_of_the_pure_components():
    for path in DevToolsSources():
        for name in Imports(path):
            if Component(name) not in PURE_ROOTS or name in DEVTOOLS_PURE_SYSTEMS:
                continue
            assert name.rsplit(".", 1)[-1] in {"Type", "State", "Layer"}, f"{path}: {name}"


def test_the_tick_observer_is_wired_from_runtime_to_devtools():
    defined = [p for p in Sources("Artemis") if re.search(r"\bstruct ObservedTick\b", Read(p))]
    assert defined == ["Artemis/4_Bridge/Codec/Pipeline/Type/Bridge.Codec.Pipeline.Type.ixx"], defined
    assert re.search(r"auto OnTick\(const ObservedTick&", Read("Artemis/8_Mod/Mod/Mod.Layer.ixx"))
    assert "AddObserver(" in Read("Artemis/8_Mod/Mod/Mod.Layer.cpp")


def test_only_the_wiring_and_the_devtools_import_the_development_tools():
    owned = ("Artemis/8_Mod/Mod/Mod.Layer.ixx", "Artemis/8_Mod/Mod/Mod.Layer.cpp", "Artemis/8_Mod/Mod/Thread/", DEVTOOLS + "/")
    for path in Sources("Artemis"):
        if path.startswith(owned):
            continue
        assert not ImportRoots(path) & DEVTOOLS_ROOTS, path


# ---- M7.2: the development tools in a project (D228)


def test_the_dll_keeps_only_the_wiring_the_runtime_and_the_platform_port():
    allowed_files = {"Mod.Layer.ixx", "Mod.Layer.cpp", "DllMain.cpp", "Artemis.Mod.vcxproj", "Artemis.Mod.vcxproj.filters", "CONTRACT.md"}
    tracked = {path[len("Artemis/8_Mod/Mod/"):].split("/")[0] for path in Tracked("Artemis/8_Mod/Mod")}
    assert tracked == allowed_files | {"Thread", "Port", "Proxy"}, tracked
    on_disk = sorted(n for n in os.listdir(os.path.join(REPO, "Artemis/8_Mod/Mod")) if not n.endswith(".user"))
    assert on_disk == sorted(allowed_files | {"Thread", "Port", "Proxy"}), on_disk


def test_the_dll_references_the_devtools_project_and_the_tests_do_not():
    dll = {ProjectName(include) for include in REFERENCE.findall(Read("Artemis/8_Mod/Mod/Artemis.Mod.vcxproj"))}
    tests = {ProjectName(include) for include in REFERENCE.findall(Read("Artemis/7_Tests/Tests/Artemis.Tests.vcxproj"))}
    tools = {"DevTools.Gui", "DevTools.Viewer", "DevTools.Workbench"}
    assert tools <= dll
    assert not tools & tests
    for name in tools:
        assert f"Artemis.{name}.vcxproj" in Read("Artemis-MCC.slnx"), name
    listed = {include.replace("\\", "/") for include in COMPILE.findall(Read("Artemis/8_Mod/Mod/Artemis.Mod.vcxproj"))}
    assert not [p for p in listed if p.startswith(("Gui/", "UI/", "Viewer/", "DevTools/"))]


# ---- M8 A: the solution folders mirrored on disk, in the order of the dependencies (D229-D231)


SLNX_PROJECT = re.compile(r'<Project\s+Path="([^"]+)"')


def test_every_project_is_in_its_folder_of_the_solution_and_of_the_disk():
    text = Read(SOLUTION)
    listed = []
    for folder in re.finditer(r'<Folder\s+Name="/([^/"]+)/">(.*?)</Folder>', text, re.S):
        listed += [(folder.group(1), path) for path in SLNX_PROJECT.findall(folder.group(2))]
    expected = [(group, ProjectPath(name)) for group, _, name in PROJECTS]
    assert sorted(listed) == sorted(expected)
    for _, path in expected:
        assert os.path.exists(os.path.join(REPO, path)), path
    on_disk = sorted(n for n in os.listdir(os.path.join(REPO, "Artemis")) if os.path.isdir(os.path.join(REPO, "Artemis", n)))
    groups = sorted({group for group, _, _ in PROJECTS})
    assert [n for n in on_disk if n not in ("External", "x64")] == groups, on_disk
    for group in groups:
        folders = sorted(n for n in os.listdir(os.path.join(REPO, "Artemis", group)) if os.path.isdir(os.path.join(REPO, "Artemis", group, n)))
        assert folders == sorted(folder for g, folder, _ in PROJECTS if g == group), (group, folders)


def test_the_order_of_the_solution_is_an_order_of_the_dependencies():
    # Visual Studio sorts folders and projects alphabetically: no project references one shown below it.
    position = {name: index for index, name in enumerate(
        name for _, _, name in sorted(PROJECTS, key=lambda p: (p[0], p[1].lower())))}
    for _, _, name in PROJECTS:
        for include in REFERENCE.findall(Read(ProjectPath(name))):
            target = ProjectName(include)
            assert position[target] < position[name], f"{name} references {target}, shown below it"


# ---- M8 C: one Layer per project (D234, D239-D246)

def test_perception_has_one_layer():
    layers = [p for p in Sources("Artemis/3_Domain/Perception") if p.endswith(".Layer.ixx")]
    assert [os.path.basename(p) for p in layers] == ["Perception.Layer.ixx"], layers
    offenders = [p for p in Sources("Artemis") if "Relations.Layer" in Read(p) or "Relations::Layer" in Read(p)]
    assert not offenders, offenders


ENGINE_LAYERS = (("Artemis/5_Engine/Base", "Engine.Base.Layer", "Engine::Base"),
                 ("Artemis/5_Engine/Capture", "Engine.Capture.Layer", "Engine::Capture"),
                 ("Artemis/5_Engine/Control", "Engine.Control.Layer", "Engine::Control"),
                 ("Artemis/5_Engine/Match", "Engine.Match.Layer", "Engine::Match"),
                 ("Artemis/5_Engine/Render", "Engine.Render.Layer", "Engine::Render"),
                 ("Artemis/4_Bridge/Transport", "Bridge.Transport.Layer", "Bridge::Transport"))
# The one that composes the Layers of the engine and connects their lifecycle: Platform.Layer in C.2, Mod.Layer since C.3.
COMPOSER = "Artemis/8_Mod/Mod/Mod.Layer.cpp"


@pytest.mark.parametrize("folder,module,namespace", ENGINE_LAYERS)
def test_every_engine_project_and_the_transport_have_their_layer(folder, module, namespace):
    layers = [p for p in Sources(folder) if p.endswith(".Layer.ixx")]
    assert layers == [f"{folder}/{module}.ixx"], layers
    text = Read(layers[0])
    assert re.search(rf"^export module {re.escape(module)};", text, re.MULTILINE)
    assert re.search(rf"^export namespace {re.escape(namespace)}\b", text, re.MULTILINE)
    assert re.search(r"\bclass\s+Layer\b", text)


@pytest.mark.parametrize("folder,module,namespace", ENGINE_LAYERS)
def test_the_engine_layers_import_only_base_and_services(folder, module, namespace):
    path = f"{folder}/{module}.ixx"
    layers = {name for name in Imports(path) if name.endswith(".Layer")}
    allowed = {"Services.Layer"} if module in ("Engine.Base.Layer", "Bridge.Transport.Layer") else {"Services.Layer", "Engine.Base.Layer"}
    assert layers <= allowed, (path, sorted(layers - allowed))
    text = Read(path)
    for word in ("Install(", "OnEngineInitialized", "OnUnhook", "OnCleanup", "OnShutdown"):
        assert word not in text, f"{path}: {word}"


def test_the_action_source_is_set_not_built_in():
    detour = Read("Artemis/5_Engine/Control/Input/Hook/PlayerActionDetour.ixx")
    constructor = re.search(r"\bPlayerActionDetour\(([^)]*)\)", detour)
    assert constructor and "ActionSource" not in constructor.group(1), constructor and constructor.group(1)
    assert re.search(r"auto\s+SetActionSource\(ActionSource&\s*\w+\)\s*->\s*void", detour)
    composer = Read(COMPOSER)
    assert "SetActionSource(" in composer
    assert composer.index("SetActionSource(") < composer.index("OnEngineInitialized(")


def test_no_thread_receives_the_platform_layer():
    sources = Sources("Artemis/8_Mod/Mod/Thread")
    assert sources
    for path in sources:
        assert "Platform.Layer" not in Imports(path) and "Platform::Layer" not in Read(path), path


CALL = re.compile(r"\b((?:m_\w+\.)+)(\w+)\(|this->(\w+)\(")


def Calls(text):
    names = []
    for match in CALL.finditer(text):
        if match.group(3):
            names.append(match.group(3))
            continue
        owner, method = match.group(1).rstrip(".").split(".")[-1][2:], match.group(2)
        if method in ("Install", "Uninstall"):
            names.append(f"{owner}.{method}")
        elif method in ("BeginMatch", "ResolveFunctions", "Stop", "RequestShutdown", "WaitForShutdown", "Shutdown"):
            names.append(method)
        else:
            names.append(owner)
    return names


def Body(text, opening):
    # The lambdas and the helpers of the wiring have no nested braces: the first closing brace on its own line ends them.
    pattern = re.escape(opening).replace("auto\\ ", r"auto\s+(?:\w+::)?")
    start = re.search(pattern, text).start()
    return text[start:start + re.search(r"\n\s*\}", text[start:]).start()]


def Wiring(text):
    # The registrations: all the text, or the body of WireEngine when the composer has one.
    marker = re.search(r"auto\s+(?:\w+::)?WireEngine\(\)\s*->\s*bool\s*\{", text)
    return text[marker.start():text.index("return true;", marker.start())] if marker else text


def test_the_lifecycle_wiring_keeps_its_order():
    text = Read(COMPOSER)
    wiring = Wiring(text)
    registers = [wiring.index(f"{name}(") for name in ("OnEngineInitialized", "OnUnhook", "OnCleanup", "OnInitialized", "OnShutdown")]
    assert registers == sorted(registers), registers
    assert Calls(Body(wiring, "OnEngineInitialized(")) == [
        "BeginMatch", "PlayerActionDetour.Install", "ResolveFunctions", "SetMachinesDetour.Install", "InstallHostHooks",
        "OpenMapDetour.Install", "ObjectTableLocator", "PlayerTableLocator", "InteractionTableLocator",
        "ExecuteGameFrameDetour.Install", "SimulationTicksDetour.Install"]
    assert Calls(Body(wiring, "OnUnhook(")) == [
        "Stop", "UninstallHostHooks", "SetMachinesDetour.Uninstall", "PlayerActionDetour.Uninstall", "OpenMapDetour.Uninstall",
        "SimulationTicksDetour.Uninstall", "ExecuteGameFrameDetour.Uninstall"]
    assert Calls(Body(wiring, "OnCleanup(")) == ["LocalUserStore", "PlayerActionStore", "BotStore", "CaptureService"]
    hosts = ["LocalUserGetPlayerDetour", "LocalUserGetProfileDetour", "LocalUserGetGamepadMappingDetour", "CanAcceptAnyJoinRequestDetour",
             "SessionJoinCheckPatch", "InputUpdateDetour"]
    assert Calls(Body(text, "auto InstallHostHooks()")) == [f"{name}.Install" for name in hosts]
    assert Calls(Body(text, "auto UninstallHostHooks()")) == [f"{name}.Uninstall" for name in reversed(hosts)]
    assert Calls(Body(text, "auto ShutdownRenderAndInput()")) == [
        "WndProcDetour.Uninstall", "GetRawInputDataDetour.Uninstall", "RequestShutdown", "WaitForShutdown",
        "PresentDetour.Uninstall", "ResizeBuffersDetour.Uninstall", "Shutdown"]


# ---- M8 C.3: one root Layer for the DLL (D244)

MOD = "Artemis/8_Mod/Mod"
MOD_MEMBER = re.compile(r"^\s+[\w:<>]+\s+(m_\w+)\s*\{", re.MULTILINE)


def test_mod_has_one_root():
    assert Tracked(f"{MOD}/Mod.Layer.ixx") and Tracked(f"{MOD}/Mod.Layer.cpp")
    for old in ("Core.ixx", "Core.cpp", "DevTools.Layer.ixx", "Runtime/Runtime.Layer.ixx", "Infrastructure/Platform/Platform.Layer.ixx"):
        assert not Tracked(f"{MOD}/{old}"), old
        assert not os.path.exists(os.path.join(REPO, MOD, old)), old
    gone = {"Core", "Runtime.Layer", "DevTools.Layer", "Platform.Layer"}
    for path in Sources("Artemis"):
        module = MODULE_LINE.search(Read(path))
        assert not (module and module.group(1) in gone), path
        assert not gone & set(Imports(path)), path
    assert "Mod.Layer" in Imports(f"{MOD}/DllMain.cpp")


def test_mod_wires_the_engine_before_the_tools():
    members = MOD_MEMBER.findall(Read(f"{MOD}/Mod.Layer.ixx"))
    assert members == [
        "m_Module", "m_IsMinHookReady", "m_Service", "m_Base", "m_Capture", "m_Control", "m_Match", "m_Render", "m_BridgeEngineAdapter",
        "m_Transport", "m_BridgeActionSource", "m_IsEngineWired", "m_Preferences", "m_EngineAccess", "m_Map", "m_Resolved", "m_Tables",
        "m_Environment", "m_Egocentric", "m_Export", "m_Gui", "m_Viewer", "m_UI", "m_Main", "m_AI", "m_Bench", "m_Bridge", "m_Dump",
        "m_AIThread", "m_BenchThread", "m_BridgeThread", "m_DumpThread"], members
    assert members.index("m_IsEngineWired") > members.index("m_BridgeActionSource")
    assert members.index("m_IsEngineWired") < members.index("m_Gui")
    assert members[-4:] == ["m_AIThread", "m_BenchThread", "m_BridgeThread", "m_DumpThread"]


STATEFUL_PROJECTS = ("Services", "Blam.Map", "Blam.MapModel", "Blam.Runtime", "Perception", "Views", "Bridge.Codec", "Bridge.Transport",
                     "Engine.Base", "Engine.Capture", "Engine.Control", "Engine.Match", "Engine.Render", "DevTools.Gui", "DevTools.Viewer",
                     "DevTools.Workbench", "Mod")


@pytest.mark.parametrize("project", [name for _, _, name in PROJECTS])
def test_every_stateful_project_has_one_layer(project):
    layers = [p for p in Sources(ProjectFolder(project)) if p.endswith(".Layer.ixx")]
    if project in STATEFUL_PROJECTS:
        assert len(layers) == 1, layers
    else:
        assert not layers, layers

# Public contracts (M8 D, D235, D248): every project has a CONTRACT.md next to its .vcxproj. Its "Public interface" section names, in
# backticks, the modules other projects may import. A listed module must exist, and no project imports a module of another one that is
# not listed. The tests (Artemis.Tests) may import internal modules.
CONTRACT_SECTIONS = ("Responsibility", "Does not", "Input and output", "Public interface", "Depends on", "State between ticks", "Threads",
                     "Errors", "Requirements", "Tests")
CONTRACT_MODULE = re.compile(r"`([A-Z]\w*(?:\.\w+)+)`")
MODULE_DECLARATION = re.compile(r"^\s*export\s+module\s+([\w.]+)\s*;", re.MULTILINE)
PROJECT_NAMES = [name for _, _, name in PROJECTS]


def Contract(project):
    return Read(f"{ProjectFolder(project)}/CONTRACT.md")


def PublicModules(project):
    """Modules of the project named in the Public interface section of its contract."""
    section = Contract(project).split("\n## Public interface\n", 1)[1].split("\n## ", 1)[0]
    return {m for m in CONTRACT_MODULE.findall(section) if Component(m) == project and m != project}


def DeclaredModules(project):
    """Primary modules (no partitions) that the sources of the project export."""
    return {m for path in Sources(ProjectFolder(project)) for m in MODULE_DECLARATION.findall(Read(path))}


@pytest.mark.parametrize("project", PROJECT_NAMES)
def test_every_project_has_a_contract_with_every_section(project):
    path = f"{ProjectFolder(project)}/CONTRACT.md"
    assert Tracked(path), f"{path} is missing"
    headings = re.findall(r"^## (.+?)\s*$", Contract(project), re.MULTILINE)
    assert headings == list(CONTRACT_SECTIONS), headings
    assert Contract(project).startswith(f"# Artemis.{project}\n")
    for listing in (ProjectPath(project), ProjectPath(project) + ".filters"):  # visible in Visual Studio
        assert '<None Include="CONTRACT.md" />' in Read(listing), listing


@pytest.mark.parametrize("project", PROJECT_NAMES)
def test_the_public_interface_lists_only_modules_that_exist(project):
    missing = sorted(PublicModules(project) - DeclaredModules(project))
    assert not missing, f"listed in {project}/CONTRACT.md but not declared: {missing}"


@pytest.mark.parametrize("project", [name for name in PROJECT_NAMES if name != "Tests"])
def test_a_project_imports_only_the_public_modules_of_the_others(project):
    public = {name: PublicModules(name) for name in PROJECT_NAMES}
    wrong = {}
    for path in Sources(ProjectFolder(project)):
        for module in IMPORT.findall(Read(path)):
            owner = Component(module)
            if module != "std" and owner != project and owner in public and module not in public[owner]:
                wrong.setdefault(path, []).append(module)
    assert not wrong, f"imports modules that the contracts do not list as public: {wrong}"


def test_the_index_links_every_contract():
    index = Read("Artemis/README.md")
    links = set(re.findall(r"\]\(([^)]+/CONTRACT\.md)\)", index))
    expected = {f"{ProjectFolder(name)[len('Artemis/'):]}/CONTRACT.md" for name in PROJECT_NAMES}
    assert links == expected, sorted(links ^ expected)
