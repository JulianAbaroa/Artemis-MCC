"""
test_components.py
Boundaries of the components that already are projects (redesign D153; Ignore Rediseno/02 sec. 3.5, D156, D165): each one imports
only the modules of the components it may depend on, references only those projects, and lists every source it owns.
Written by Opus before the code (D161) in M3 (D177-D181) and extended in M4a, M4b (D182-D188), M5a (D189-D197), M5b (D198-D201) and M6a (D210-D215); each later step
that creates a component adds it to COMPONENTS.
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
    "Artemis/Blam.Runtime": ("Artemis.Blam.Runtime.vcxproj", {"Common", "Datum", "Layout", "Resolved", "Tables"},
                             {"Common", "Blam.Map", "Blam.MapModel", "Blam.Layout"}),
    "Artemis/Contract": ("Artemis.Contract.vcxproj", {"Common", "Contract"}, {"Common"}),
    "Artemis/Perception": ("Artemis.Perception.vcxproj", {"Common", "Datum", "Layout", "Resolved", "Tables", "Relations", "Environment"},
                           {"Common", "Blam.Map", "Blam.MapModel", "Blam.Layout", "Blam.Runtime"}),
    "Artemis/Views": ("Artemis.Views.vcxproj",
                      {"Common", "Datum", "Layout", "Resolved", "Tables", "Relations", "Environment", "Egocentric"},
                      {"Common", "Blam.Map", "Blam.MapModel", "Blam.Layout", "Blam.Runtime", "Perception"}),
    "Artemis/Bridge.Codec": ("Artemis.Bridge.Codec.vcxproj",
                             {"Common", "Datum", "Layout", "Resolved", "Tables", "Relations", "Environment", "Egocentric", "Export",
                              "Contract"},
                             {"Common", "Blam.Map", "Blam.MapModel", "Blam.Layout", "Blam.Runtime", "Perception", "Views", "Contract"}),
    "Artemis/Control": ("Artemis.Control.vcxproj", {"Common", "Layout", "Contract", "Control"}, {"Common", "Blam.Layout", "Contract"}),
}

# The pure projects of M5b (D198): what was the Domain after Blam.Runtime. None of them touches the process, the wiring or a layer above.
PURE_PROJECTS = ("Artemis/Contract", "Artemis/Perception", "Artemis/Views", "Artemis/Bridge.Codec", "Artemis/Control")
NOT_PURE_ROOTS = {"Service", "Platform", "Infrastructure", "Runtime", "UI", "Viewer", "Gui", "Core"}
NOT_PURE_WORDS = ("windows.h", "MinHook", "reinterpret_cast", "OnCleanup(", "OnEngineInitialized(", "OnUnhook(")
LAYERS = (("Relations", "Artemis/Perception/Relations/Relations.Layer.ixx", ("Log& log", "Resolved::Layer& resolved", "Tables::Layer& tables")),
          ("Environment", "Artemis/Perception/Environment/Environment.Layer.ixx",
           ("Log& log", "Resolved::Layer& resolved", "Tables::Layer& tables", "Relations::Layer& relations")),
          ("Egocentric", "Artemis/Views/Egocentric.Layer.ixx",
           ("Log& log", "Resolved::Layer& resolved", "Tables::Layer& tables", "Relations::Layer& relations",
            "Environment::Layer& environment")),
          ("Export", "Artemis/Bridge.Codec/Export.Layer.ixx",
           ("Resolved::Layer& resolved", "Tables::Layer& tables", "Relations::Layer& relations", "Environment::Layer& environment",
            "Egocentric::Layer& egocentric")))
TICK_DETOURS = ("ExecuteGameFrameDetour", "SimulationTicksDetour")

# The interpretation of the live objects (02 sec. 3.2): a project since M5a (D189), it reads only the Snapshot (D184).
RUNTIME = "Artemis/Blam.Runtime"

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


def PureSources():
    return [path for folder in PURE_PROJECTS for path in Sources(folder)]


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
    assert {"Common", "Services", "Blam.Map", "Blam.MapModel", "Blam.Layout", "Blam.Runtime", "Contract", "Perception", "Views",
            "Bridge.Codec", "Control"} <= found


def test_tests_project_references_the_blam_components():
    found = {ProjectName(include) for include in REFERENCE.findall(Read("Artemis/Tests/Artemis.Tests.vcxproj"))}
    assert {"Blam.Map", "Blam.MapModel", "Blam.Layout", "Blam.Runtime", "Contract", "Perception", "Views", "Bridge.Codec",
            "Control"} <= found


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
    moved = [p for p in Sources(RUNTIME)
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


def test_old_tables_folder_is_gone():
    assert Tracked("Artemis/Core/Domain/4_Tables") == []


def test_blam_runtime_does_not_touch_the_process():
    sources = Sources(RUNTIME)
    assert sources, f"{RUNTIME} has no sources"
    for path in sources:
        text = Read(path)
        for word in ("MemoryReaderService", "reader.Read", "windows.h", "reinterpret_cast", "OnCleanup"):
            assert word not in text, f"{path}: {word}"
        assert not re.search(r"\b(Platform|Service)::", text), f"{path}: the DLL wiring"


def test_tables_hold_no_process_address():
    member = re.compile(r"std::uintptr_t\s+\w+\s*\{\s*\}")
    for path in Sources(RUNTIME):
        text = Read(path)
        assert not member.search(text), f"{path}: a member of type std::uintptr_t"
        assert not re.search(r"\b(Address|BaseAddress)\b", text), path
    wrong = [p for p in PureSources() if re.search(r"\.Address\b", Read(p))]
    assert not wrong, f"the Domain checks an address of the process: {wrong}"


def test_dev_screens_take_the_address_from_the_snapshot():
    objects = Read("Artemis/Core/UI/ObjectTable/System/ObjectTableUIService.cpp")
    players = Read("Artemis/Core/UI/PlayerTable/System/PlayerTableUIService.cpp")
    assert "RawObjects" in objects and "object.Address" not in objects
    assert "OriginalAddress" in players and "player.Address" not in players


def test_wide_to_utf8_is_portable_and_lives_in_common():
    definitions = [p for p in Sources("Artemis") if p.endswith(".ixx") and re.search(r"\bauto\s+WideToUtf8\s*\(", Read(p))]
    assert definitions == ["Artemis/Common/Text/System/Common.Text.System.ixx"], definitions
    wrong = [p for p in Sources("Artemis") if "WideCharToMultiByte" in Read(p) and not p.startswith("Artemis/Tests/")]
    assert not wrong, wrong


def test_tables_layer_is_pure_and_core_cleans_it():
    layer = Read(f"{RUNTIME}/Tables.Layer.ixx")
    assert re.search(r"\bLayer\(\s*Log&\s+log\s*,\s*Resolved::Layer&\s+resolved\s*\)", layer)
    assert re.search(r"auto\s+Cleanup\(\)\s*->\s*void", layer)
    assert "m_Tables.Cleanup()" in Read("Artemis/Core/Core.cpp")


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


def test_shadow_mode_is_gone():
    assert Tracked("Artemis/Core/Domain/4_Tables/Shadow") == []
    wrong = [p for p in Sources("Artemis") if re.search(r"\bShadow(Service|PlayerTableService|InteractionService|GameService)\b",
                                                         Read(p))]
    assert not wrong, wrong


def test_the_domain_does_not_read_the_game():
    wrong = [p for p in PureSources() if "MemoryReaderService" in Read(p)]
    assert not wrong, wrong


def test_tick_trace_reads_its_clock_from_the_snapshot():
    for path in Sources("Artemis/Core/UI/TickTrace"):
        text = Read(path)
        assert "ReadValue" not in text and "GetThreadContext" not in text and "reinterpret_cast" not in text, path


def test_read_window_ends_right_after_the_copy():
    text = Read("Artemis/Core/Runtime/AI/AIThread.cpp")
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
    sources = Sources("Artemis/Contract")
    assert sources == ["Artemis/Contract/Contract.Bridge.ixx"], sources
    text = Read(sources[0])
    assert text.splitlines()[0].startswith("// Generated by"), text.splitlines()[0]
    assert re.search(r"^export module Contract\.Bridge;", text, re.MULTILINE)
    assert ImportRoots(sources[0]) <= {"Common"}


def test_bridge_type_reexports_the_contract_and_the_clock_moved_to_flat():
    bridge = Read("Artemis/Core/Infrastructure/Platform/Bridge/Type/Platform.Bridge.Type.ixx")
    assert "export import Contract.Bridge;" in bridge
    assert not re.search(r"\bstruct\s+BridgeClock\b", bridge)
    assert re.search(r"\bstruct\s+BridgeClock\b", Read("Artemis/Bridge.Codec/Flat/Type/Export.Flat.Type.ixx"))
    assert not Tracked("Artemis/Core/Infrastructure/Platform/Bridge/Type/BridgeLayout.ixx")


def test_tick_detours_live_in_platform_capture_hook():
    found = [p for p in Sources("Artemis") if os.path.basename(p).split(".")[0] in TICK_DETOURS]
    assert sorted(os.path.basename(p) for p in found) == sorted(f"{name}.{ext}" for name in TICK_DETOURS for ext in ("cpp", "ixx"))
    assert all(p.startswith(CAPTURE_HOOK) for p in found), found
    for name, part in (("ExecuteGameFrameDetour", "ExecuteGameFrame"), ("SimulationTicksDetour", "SimulationTicks")):
        assert Read(f"{CAPTURE_HOOK}{name}.ixx").startswith(f"export module Platform.Capture.Hook:{part};")
        assert f"export import :{part};" in Read(CAPTURE_HOOK + "../Platform.Capture.Hook.ixx")
    layer = Read("Artemis/Core/Infrastructure/Platform/Platform.Layer.ixx")
    for name in TICK_DETOURS:
        assert f"m_{name}.Install();" in layer and f"m_{name}.Uninstall();" in layer
    assert not [p for p in Tracked("Artemis") if os.path.basename(p) == "Export.Tick.Hook.ixx"]
    for path in Sources("Artemis/Bridge.Codec"):
        text = Read(path)
        for word in ("ExecuteGameFrameFunction", "SimulationTicksFunction", "EndRoundFunction"):
            assert word not in text, f"{path}: {word}"


@pytest.mark.parametrize("name,path,parameters", LAYERS)
def test_layers_are_pure_with_a_cleanup(name, path, parameters):
    text = Read(path)
    constructor = r"\bLayer\(\s*" + r"\s*,\s*".join(re.escape(p).replace(r"\ ", r"\s+") for p in parameters) + r"\s*\)"
    assert re.search(constructor, text), f"{name}: constructor"
    assert re.search(r"auto\s+Cleanup\(\)\s*->\s*void", text), f"{name}: Cleanup"
    assert "Service::Layer" not in text and "Platform::Layer" not in text and "OnCleanup" not in text, name


def test_core_cleans_the_seven_layers_in_order():
    text = Read("Artemis/Core/Core.cpp")
    order = ["m_Relations", "m_Environment", "m_Egocentric", "m_Export", "m_Tables", "m_Map", "m_Resolved"]
    positions = [text.index(f"{member}.Cleanup()") for member in order]
    assert positions == sorted(positions), positions


def test_generate_bridge_writes_the_contract():
    text = Read("Tools/TagTranspiler/GenerateBridge.py")
    assert '"Artemis", "Contract", "Contract.Bridge.ixx"' in text
    assert '"export module Contract.Bridge;"' in text
    assert "BridgeLayout.ixx" not in text


# --- M5c (D202-D207) ------------------------------------------------------------------------------------------------------------

# Calls that run one step of the tick: only PipelineService.Run does it (D205).
PIPELINE_STEP = re.compile(r"\.m_\w+Service\.(?:Update\w*|Assemble)\(")
SNAPSHOT_CODEC = "Artemis/Blam.Layout/Snapshot/System/Layout.Snapshot.System"
DUMP_CODEC = "Artemis/Bridge.Codec/Dump/"
PIPELINE_SERVICE = "Artemis/Bridge.Codec/Pipeline/System/PipelineService"


def test_common_hash_does_not_touch_windows():
    sources = Sources("Artemis/Common/Hash")
    assert sorted(os.path.basename(p) for p in sources) == ["Common.Hash.System.cpp", "Common.Hash.System.ixx"], sources
    for path in sources:
        assert "windows.h" not in Read(path), path
        assert ImportRoots(path) == {"Common"} or ImportRoots(path) == set(), path
    assert re.search(r"^export module Common\.Hash\.System;", Read("Artemis/Common/Hash/System/Common.Hash.System.ixx"), re.MULTILINE)


def test_tests_support_maps_takes_the_crc_from_common():
    text = Read("Artemis/Tests/Support/Tests.Support.Maps.ixx")
    assert "import Common.Hash.System;" in text
    assert "k_Crc32Polynomial" not in text and "MakeCrc32Table" not in text


def test_snapshot_codec_imports_only_what_its_project_allows():
    paths = [SNAPSHOT_CODEC + ".ixx", SNAPSHOT_CODEC + ".cpp"]
    assert sorted(Tracked(*paths)) == sorted(paths)
    assert "Artemis/Blam.Layout/Snapshot/Type/SnapshotRecord.ixx" in Tracked("Artemis/Blam.Layout")
    for path in paths + ["Artemis/Blam.Layout/Snapshot/Type/SnapshotRecord.ixx"]:
        assert ImportRoots(path) <= {"Common", "Layout"}, (path, ImportRoots(path))


def test_dump_codec_and_pipeline_import_only_what_their_project_allows():
    dump = [p for p in Sources("Artemis/Bridge.Codec") if p.startswith(DUMP_CODEC)]
    assert sorted(os.path.basename(p) for p in dump) == ["Export.Dump.System.cpp", "Export.Dump.System.ixx", "Export.Dump.Type.ixx"], dump
    for path in dump:
        assert ImportRoots(path) <= {"Common", "Contract", "Export"}, (path, ImportRoots(path))
    pipeline = [PIPELINE_SERVICE + ".ixx", PIPELINE_SERVICE + ".cpp"]
    assert sorted(Tracked(*pipeline)) == sorted(pipeline)
    for path in pipeline:
        assert ImportRoots(path) <= {"Layout", "Tables", "Relations", "Environment", "Egocentric", "Export"}, (path, ImportRoots(path))
    assert "export module Export.Pipeline.System;" in Read(pipeline[0])
    assert re.search(r"\bPipelineService\s+m_PipelineService\b", Read("Artemis/Bridge.Codec/Export.Layer.ixx"))


def test_the_order_of_the_tick_lives_in_the_pipeline_service():
    thread = Read("Artemis/Core/Runtime/AI/AIThread.cpp")
    assert not PIPELINE_STEP.search(thread), PIPELINE_STEP.search(thread).group(0)
    assert "m_PipelineService.Run(" in thread
    tests = Read("Artemis/Tests/Perception/PipelineTests.cpp")
    assert not PIPELINE_STEP.search(tests), PIPELINE_STEP.search(tests).group(0)
    assert "m_PipelineService.Run(" in tests
    # Tables 4, Relations 2, Environment 11 and Egocentric 3. The Tick is assembled through TickService.
    assert len(PIPELINE_STEP.findall(Read(PIPELINE_SERVICE + ".cpp"))) == 20


def test_the_tdump_format_lives_in_bridge_codec_not_in_the_ui():
    ui = Read("Artemis/Core/UI/TickDump/Type/UI.TickDump.Type.ixx")
    assert not re.search(r"\bstruct\s+(?:TickRecordHeader|TickDumpFileHeader|TickDumpFooter)\b", ui)
    assert "TickRecordHeader" not in ui
    assert re.search(r"\bstruct\s+TickRecordHeader\b", Read("Artemis/Bridge.Codec/Dump/Type/Export.Dump.Type.ixx"))
    for path in Sources("Artemis/Core/UI/TickDump"):
        assert "UI::TickDump::Type::TickRecordHeader" not in Read(path), path


# ---- M6a: Control as a project (D210-D215)

INPUT_TYPE = "Artemis/Core/Infrastructure/Platform/Input/Type/Platform.Input.Type.ixx"
SIMULATION_TYPE = "Artemis/Core/Infrastructure/Platform/Simulation/Type/Platform.Simulation.Type.ixx"
DETOUR_CPP = "Artemis/Core/Infrastructure/Platform/Input/Hook/PlayerActionDetour.cpp"
BRIDGE_SERVICE_CPP = "Artemis/Core/Infrastructure/Platform/Bridge/System/BridgeService.cpp"
BOT_SERVICE_IXX = "Artemis/Core/Infrastructure/Platform/Simulation/System/BotService.ixx"
CONTROL_NOT_PURE_WORDS = ("std::mutex", "std::thread", "std::chrono")


def Defines(path_text, kind, name):
    return re.search(rf"\b{kind}\s+{name}\b\s*(?:\{{|$)", path_text, re.MULTILINE) is not None


def test_the_action_format_lives_in_blam_layout():
    sources = Sources("Artemis/Blam.Layout/Action")
    assert sorted(sources) == ["Artemis/Blam.Layout/Action/Layout.Action.Type.ixx", "Artemis/Blam.Layout/Action/Type/ActionBits.ixx",
                               "Artemis/Blam.Layout/Action/Type/ActionOffset.ixx", "Artemis/Blam.Layout/Action/Type/ActionSize.ixx"]
    for partition, name in (("Offset", "ActionOffset"), ("Size", "ActionSize"), ("Bits", "ActionBits")):
        assert Read(f"Artemis/Blam.Layout/Action/Type/{name}.ixx").startswith(f"export module Layout.Action.Type:{partition};")
        assert f"export import :{partition};" in Read("Artemis/Blam.Layout/Action/Layout.Action.Type.ixx")
    for path in (INPUT_TYPE, SIMULATION_TYPE):
        text = Read(path)
        for name in ("k_PlayerActionSize", "k_ActionButtonsOffset", "k_HoldLanes", "k_NoZoomLevel"):
            assert not re.search(rf"\b{name}\s*\{{", text), f"{path} defines {name}"
        assert not Defines(text, "struct", "HoldLane"), path


@pytest.mark.parametrize("name", ["ActionOverride", "ClientRequests", "WeaponRequest", "AppliedFacing", "UnitClientState",
                                  "PlayerActionHistory", "BridgeControlState", "BridgeHookState"])
def test_control_types_are_defined_only_under_control(name):
    found = [path for path in Sources("Artemis") if Defines(Read(path), "struct", name)]
    assert found and all(path.startswith("Artemis/Control/") for path in found), found


def test_ticks_per_second_is_defined_only_in_common_engine_type():
    found = [path for path in Sources("Artemis") if re.search(r"\bk_TicksPerSecond\s*\{", Read(path))]
    assert found == ["Artemis/Common/Engine/Type/Common.Engine.Type.ixx"], found


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
    assert re.search(r"class\s+BotService\s*:\s*public\s+Control::Action::System::UnitRules\b", Read(BOT_SERVICE_IXX))


def test_control_does_not_touch_the_process_the_wiring_or_the_clock():
    sources = Sources("Artemis/Control")
    assert sources
    for path in sources:
        text = Read(path)
        assert not (ImportRoots(path) & NOT_PURE_ROOTS), f"{path}: imports {sorted(ImportRoots(path) & NOT_PURE_ROOTS)}"
        for word in NOT_PURE_WORDS + CONTROL_NOT_PURE_WORDS:
            assert word not in text, f"{path}: {word}"
