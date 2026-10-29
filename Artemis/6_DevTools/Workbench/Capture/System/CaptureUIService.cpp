module;

#include <windows.h>

#include "External/imgui/imgui.h"

module DevTools.Workbench.Capture.System;

import Common.Engine.Type;
import Common.Time.System;
import Engine.Base.Host.Type;
import Blam.MapModel.Definitions.Type;
import Blam.MapModel.Definitions.State;
import Blam.Layout.Object.Type;
import Blam.Layout.Snapshot.Type;
import DevTools.Gui.Style.Type;
import std;

namespace
{
    using std::filesystem::path;
    using Common::Time::System::LocalNow;

    using Blam::MapModel::Definitions::Type::Object::ObjectKind;
    using Blam::MapModel::Definitions::Type::Phmo::Phmo;
    using Blam::MapModel::Definitions::Type::Phmo::RigidBody;
    using Blam::MapModel::Definitions::Type::Phmo::Shape;
    using Blam::MapModel::Definitions::Type::Phmo::PhantomType;
    using Blam::MapModel::Definitions::State::DefinitionsStore;
    using Blam::MapModel::Definitions::Type::Object::ToObjectKind;
    using Blam::Layout::Snapshot::Type::RawObject;
    using Blam::Layout::Snapshot::Type::Snapshot;
    using DevTools::Workbench::Capture::Type::ComponentRange;
    using DevTools::Workbench::Capture::Type::Source;
    using DevTools::Workbench::Capture::Type::Target;
    using DevTools::Workbench::Capture::Type::ByteChange;
    using DevTools::Workbench::Capture::Type::HavokFilter;
    using DevTools::Workbench::Capture::Type::HavokBody;
    using DevTools::Workbench::Capture::Type::HavokPhantom;
    using DevTools::Workbench::Capture::Type::HavokComponent;

    using Engine::Base::Host::Type::k_GameModuleName;
    using DevTools::Workbench::Capture::Type::k_LabelSize;
    using Blam::MapModel::Definitions::Type::Phmo::k_NoCollisionGroup;

    namespace ObjeOffset = Blam::Layout::Object::Type::Obje::Offset;
    namespace AnimOffset = Blam::Layout::Object::Type::Anim::Offset;
    namespace UnitOffset = Blam::Layout::Object::Type::Unit::Offset;
    namespace BipdOffset = Blam::Layout::Object::Type::Bipd::Offset;
    namespace VehiOffset = Blam::Layout::Object::Type::Vehi::Offset;
    namespace ItemOffset = Blam::Layout::Object::Type::Item::Offset;
    namespace WeapOffset = Blam::Layout::Object::Type::Weap::Offset;
    namespace EqipOffset = Blam::Layout::Object::Type::Eqip::Offset;
    namespace ProjOffset = Blam::Layout::Object::Type::Proj::Offset;
    namespace ScenOffset = Blam::Layout::Object::Type::Scen::Offset;
    namespace BlocOffset = Blam::Layout::Object::Type::Bloc::Offset;
    namespace DeviOffset = Blam::Layout::Object::Type::Devi::Offset;
    namespace MachOffset = Blam::Layout::Object::Type::Mach::Offset;
    namespace CtrlOffset = Blam::Layout::Object::Type::Ctrl::Offset;

    using Blam::Layout::Object::Type::Constant::k_ThreadContextGameModeOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextTimeGlobalsOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextMapVariantOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextTelemetryDataOffset;
    using Blam::Layout::Object::Type::Constant::k_TelemetryDataPlayerTableOffset;
    using DevTools::Workbench::Capture::Type::k_HavokWorldRva;
    using DevTools::Workbench::Capture::Type::k_WorldFilterOffset;
    using DevTools::Workbench::Capture::Type::k_FilterLookupOffset;
    using DevTools::Workbench::Capture::Type::k_HavokComponentArrayRva;
    using Blam::Layout::Object::Type::Constant::k_DataArrayData;
    using DevTools::Workbench::Capture::Type::k_HavokComponentSize;
    using DevTools::Workbench::Capture::Type::k_ComponentFlagsOffset;
    using DevTools::Workbench::Capture::Type::k_ComponentOwnerOffset;
    using DevTools::Workbench::Capture::Type::k_ComponentSizeOffset;
    using DevTools::Workbench::Capture::Type::k_ComponentBodiesOffset;
    using DevTools::Workbench::Capture::Type::k_ComponentBodyCountOffset;
    using DevTools::Workbench::Capture::Type::k_BodyEntrySize;
    using DevTools::Workbench::Capture::Type::k_BodyEntryRigidBodyOffset;
    using DevTools::Workbench::Capture::Type::k_ComponentPhantomOffsets;
    using DevTools::Workbench::Capture::Type::k_RigidBodyFilterInfoOffset;
    using DevTools::Workbench::Capture::Type::k_RigidBodyMotionTypeOffset;
    using DevTools::Workbench::Capture::Type::k_CollisionTestFlagsRva;
    using DevTools::Workbench::Capture::Type::k_FilterFieldMask;
    using DevTools::Workbench::Capture::Type::k_SubsystemShift;
    using DevTools::Workbench::Capture::Type::k_IgnoredSubsystemShift;
    using DevTools::Workbench::Capture::Type::k_SystemGroupShift;
    using Common::Engine::Type::k_NoHandle;
    using DevTools::Gui::Style::Type::k_BytesPerRow;
    using Common::Engine::Type::k_HandleIndexMask;

    // Folder inside the storage folder that receives one file per capture session.
    constexpr std::string_view k_CaptureFolder{ "Captures" };

    // Rows the change table draws at most.
    constexpr int k_MaxChangeRows{ 2000 };

    // Engine globals copied with the engine option (A5), reached from the engine thread context.
    // note: Fake handles keep each block apart in the analysis script. Pointer offsets in DatumConstant.
    struct EngineBlock
    {
        const char* Name{ "" };

        // Offset of the block pointer in the thread context. The context itself when 0 and not indirect.
        std::uintptr_t PointerOffset{};
        bool IsIndirect{};

        // Second pointer to follow inside the first block, 0 for none.
        std::uintptr_t InnerOffset{};

        std::size_t MaximumSize{};
    };

    constexpr std::array<EngineBlock, 5> k_EngineBlocks
    {{
        { .Name = "context", .PointerOffset = 0, .IsIndirect = false, .InnerOffset = 0, .MaximumSize = 1024 },
        { .Name = "game", .PointerOffset = k_ThreadContextGameModeOffset, .IsIndirect = true, .InnerOffset = 0, .MaximumSize = 4096 },
        { .Name = "time", .PointerOffset = k_ThreadContextTimeGlobalsOffset, .IsIndirect = true, .InnerOffset = 0, .MaximumSize = 512 },
        { .Name = "engine", .PointerOffset = k_ThreadContextMapVariantOffset, .IsIndirect = true, .InnerOffset = 0, .MaximumSize = 131072 },
        { .Name = "players", .PointerOffset = k_ThreadContextTelemetryDataOffset, .IsIndirect = true,
            .InnerOffset = k_TelemetryDataPlayerTableOffset, .MaximumSize = 18688 },
    }};

    // Largest game state copied. The allocation is 64 MB but past ~10 MB it holds copies of the game state (A5).
    constexpr std::size_t k_MaxGameStateSize{ 12 * 1024 * 1024 };

    // First fake handle of the engine blocks.
    constexpr std::uint32_t k_EngineHandleBase{ 0xEEEE0000 };

    // Bytes read at once when copying an engine block.
    constexpr std::size_t k_PageSize{ 4096 };

    // Name of the range after the last component: the variable blocks of the datum.
    constexpr const char* k_BlocksName{ "blocks" };


    // Bodies read at most per component. A larger count means the component address is wrong.
    constexpr std::int32_t k_MaxHavokBodies{ 64 };

    constexpr ComponentRange k_Obje{ .Name = "obje", .Start = ObjeOffset::k_ComponentStart, .End = ObjeOffset::k_ComponentEnd };
    constexpr ComponentRange k_Anim{ .Name = "anim", .Start = AnimOffset::k_ComponentStart, .End = AnimOffset::k_ComponentEnd };
    constexpr ComponentRange k_Unit{ .Name = "unit", .Start = UnitOffset::k_ComponentStart, .End = UnitOffset::k_ComponentEnd };
    constexpr ComponentRange k_Bipd{ .Name = "bipd", .Start = BipdOffset::k_ComponentStart, .End = BipdOffset::k_ComponentEnd };
    constexpr ComponentRange k_Vehi{ .Name = "vehi", .Start = VehiOffset::k_ComponentStart, .End = VehiOffset::k_ComponentEnd };
    constexpr ComponentRange k_Item{ .Name = "item", .Start = ItemOffset::k_ComponentStart, .End = ItemOffset::k_ComponentEnd };
    constexpr ComponentRange k_Weap{ .Name = "weap", .Start = WeapOffset::k_ComponentStart, .End = WeapOffset::k_ComponentEnd };
    constexpr ComponentRange k_Eqip{ .Name = "eqip", .Start = EqipOffset::k_ComponentStart, .End = EqipOffset::k_ComponentEnd };
    constexpr ComponentRange k_Proj{ .Name = "proj", .Start = ProjOffset::k_ComponentStart, .End = ProjOffset::k_ComponentEnd };
    constexpr ComponentRange k_Scen{ .Name = "scen", .Start = ScenOffset::k_ComponentStart, .End = ScenOffset::k_ComponentEnd };
    constexpr ComponentRange k_Bloc{ .Name = "bloc", .Start = BlocOffset::k_ComponentStart, .End = BlocOffset::k_ComponentEnd };
    constexpr ComponentRange k_Devi{ .Name = "devi", .Start = DeviOffset::k_ComponentStart, .End = DeviOffset::k_ComponentEnd };
    constexpr ComponentRange k_Mach{ .Name = "mach", .Start = MachOffset::k_ComponentStart, .End = MachOffset::k_ComponentEnd };
    constexpr ComponentRange k_Ctrl{ .Name = "ctrl", .Start = CtrlOffset::k_ComponentStart, .End = CtrlOffset::k_ComponentEnd };

    // Components of each object class, in datum order (reverse notes of CreateObject).
    // note: Weapons, equipment and the classes below the unit have no animation component.
    constexpr std::array<ComponentRange, 4> k_BipedComponents{ k_Obje, k_Anim, k_Unit, k_Bipd };
    constexpr std::array<ComponentRange, 4> k_VehicleComponents{ k_Obje, k_Anim, k_Unit, k_Vehi };
    constexpr std::array<ComponentRange, 3> k_WeaponComponents{ k_Obje, k_Item, k_Weap };
    constexpr std::array<ComponentRange, 3> k_EquipmentComponents{ k_Obje, k_Item, k_Eqip };
    constexpr std::array<ComponentRange, 2> k_ProjectileComponents{ k_Obje, k_Proj };
    constexpr std::array<ComponentRange, 2> k_SceneryComponents{ k_Obje, k_Scen };
    constexpr std::array<ComponentRange, 2> k_CrateComponents{ k_Obje, k_Bloc };
    constexpr std::array<ComponentRange, 3> k_MachineComponents{ k_Obje, k_Devi, k_Mach };
    constexpr std::array<ComponentRange, 3> k_ControlComponents{ k_Obje, k_Devi, k_Ctrl };
    constexpr std::array<ComponentRange, 1> k_ObjectComponents{ k_Obje };

    // Entries of the component filter. The first one shows every component.
    constexpr std::array<const char*, 16> k_FilterNames
    {
        "all", "obje", "anim", "unit", "bipd", "vehi", "item", "weap", "eqip", "proj", "scen", "bloc", "devi", "mach", "ctrl", k_BlocksName,
    };

    // Returns the components of an object class. Classes without a layout only have the object component.
    auto ComponentsOf(ObjectKind kind) -> std::span<const ComponentRange>
    {
        switch (kind)
        {
        case ObjectKind::Biped: return k_BipedComponents;
        case ObjectKind::Vehicle: return k_VehicleComponents;
        case ObjectKind::Weapon: return k_WeaponComponents;
        case ObjectKind::Equipment: return k_EquipmentComponents;
        case ObjectKind::Projectile: return k_ProjectileComponents;
        case ObjectKind::Scenery: return k_SceneryComponents;
        case ObjectKind::Crate: return k_CrateComponents;
        case ObjectKind::Machine: return k_MachineComponents;
        case ObjectKind::Control: return k_ControlComponents;
        default: return k_ObjectComponents;
        }
    }

    // Cuts the datum by the components of its class, clipped to its size, plus one range with the bytes after the last component.
    auto CutDatum(const RawObject& datum) -> std::vector<ComponentRange>
    {
        std::vector<ComponentRange> cuts{};
        const std::size_t size{ datum.Bytes.size() };
        std::size_t end{ 0 };

        for (const ComponentRange& component : ComponentsOf(ToObjectKind(datum.KindId)))
        {
            if (component.Start >= size) break;

            const ComponentRange cut{ .Name = component.Name, .Start = component.Start, .End = (std::min)(component.End, size) };
            cuts.push_back(cut);
            end = cut.End;
        }

        if (end < size) cuts.push_back(ComponentRange{ .Name = k_BlocksName, .Start = end, .End = size });

        return cuts;
    }

    // Returns the name the session file uses for a source.
    auto SourceName(Source source) -> const char*
    {
        switch (source)
        {
        case Source::Self: return "self";
        case Source::Linked: return "linked";
        case Source::Player: return "player";
        case Source::Pinned: return "pinned";
        case Source::Projectile: return "projectile";
        default: return "unknown";
        }
    }

    // Formats bytes as space separated hex pairs.
    auto HexBytes(std::span<const std::uint8_t> bytes) -> std::string
    {
        std::string text{};
        text.reserve(bytes.size() * 3);

        for (const std::uint8_t byte : bytes)
        {
            if (!text.empty()) text += ' ';
            text += std::format("{:02X}", byte);
        }

        return text;
    }

    // Adds an object to the targets if it is alive and not listed yet.
    auto AddTarget(std::vector<Target>& targets, const Snapshot& snapshot, std::uint32_t handle, Source source) -> void
    {
        if (handle == k_NoHandle) return;
        if (!snapshot.RawObjects.contains(handle)) return;
        if (std::ranges::any_of(targets, [handle](const Target& target) { return target.Handle == handle; })) return;

        targets.push_back(Target{ .Handle = handle, .Source = source });
    }

    // Adds an object and, if it is a biped, its weapons, its armor ability, the drop shield bubble or hologram of the ability
    // and its parent (the vehicle it sits in).
    auto AddWithLinks(std::vector<Target>& targets, const Snapshot& snapshot, std::uint32_t handle, Source source) -> void
    {
        AddTarget(targets, snapshot, handle, source);

        const auto it{ snapshot.RawObjects.find(handle) };
        if (it == snapshot.RawObjects.end()) return;

        const RawObject& datum{ it->second };
        if (ToObjectKind(datum.KindId) != ObjectKind::Biped) return;

        AddTarget(targets, snapshot, datum.Read<std::uint32_t>(UnitOffset::k_PrimaryWeaponHandle), Source::Linked);
        AddTarget(targets, snapshot, datum.Read<std::uint32_t>(UnitOffset::k_SecondaryWeaponHandle), Source::Linked);
        AddTarget(targets, snapshot, datum.Read<std::uint32_t>(UnitOffset::k_ArmorAbilityHandle), Source::Linked);
        AddTarget(targets, snapshot, datum.Read<std::uint32_t>(ObjeOffset::k_ParentHandle), Source::Linked);

        const auto ability{ snapshot.RawObjects.find(datum.Read<std::uint32_t>(UnitOffset::k_ArmorAbilityHandle)) };
        if (ability == snapshot.RawObjects.end() || ToObjectKind(ability->second.KindId) != ObjectKind::Equipment) return;

        AddTarget(targets, snapshot, ability->second.Read<std::uint32_t>(EqipOffset::k_HologramObjectHandle), Source::Linked);
        AddTarget(targets, snapshot, ability->second.Read<std::uint32_t>(EqipOffset::k_DropShieldObjectHandle), Source::Linked);
    }

    // Writes the header of an object and the hex rows of each of its ranges.
    auto WriteObject(std::ofstream& file, const RawObject& datum, Source source, std::string_view fourCC,
        std::string_view tagName, std::span<const ComponentRange> cuts) -> void
    {
        file << std::format("-- object {:08X} source {} class {} size 0x{:X} address 0x{:X} tag {}\n",
            datum.Handle, SourceName(source), fourCC, datum.Bytes.size(), datum.Address, tagName);

        for (const ComponentRange& cut : cuts)
        {
            file << std::format(".. {} 0x{:X} 0x{:X}\n", cut.Name, cut.Start, cut.End);

            for (std::size_t offset{ cut.Start }; offset < cut.End; offset += k_BytesPerRow)
            {
                const std::size_t count{ (std::min)(k_BytesPerRow, cut.End - offset) };
                const std::span<const std::uint8_t> row{ datum.Bytes.data() + offset, count };

                file << std::format("{:04X}  {}\n", offset, HexBytes(row));
            }
        }
    }

    // return: Base address of the game module, or 0 if it is not loaded.
    auto GameModuleBase() -> std::uintptr_t
    {
        return reinterpret_cast<std::uintptr_t>(GetModuleHandleW(k_GameModuleName));
    }

    // Writes the layer table of the Havok collision filter, one row per layer, or a line saying it was not found.
    auto WriteHavokFilter(std::ofstream& file, const std::optional<HavokFilter>& filter) -> void
    {
        if (!filter)
        {
            file << "hk-filter none\n";
            return;
        }

        file << std::format("hk-filter world 0x{:X} filter 0x{:X}\n", filter->World, filter->Filter);

        for (std::size_t layer{ 0 }; layer < filter->Lookup.size(); ++layer)
        {
            file << std::format("hk-lookup {:02} {:08X}\n", layer, filter->Lookup[layer]);
        }
    }

    // Writes the Havok component of an object and one line per rigid body, with the fields of its filter info.
    // param handle: Value of obje 0xCC.
    auto WriteHavokComponent(std::ofstream& file, std::uint32_t handle, const std::optional<HavokComponent>& component) -> void
    {
        if (handle == k_NoHandle)
        {
            file << "hk-component none\n";
            return;
        }

        if (!component)
        {
            file << std::format("hk-component {:08X} unreadable\n", handle);
            return;
        }

        file << std::format("hk-component {:08X} address 0x{:X} salt {:04X} flags {:08X} owner {:08X} size {} bodies {}\n",
            component->Handle, component->Address, component->Salt, component->Flags, component->Owner,
            static_cast<int>(component->Size), component->BodyCount);

        for (std::size_t index{ 0 }; index < component->Bodies.size(); ++index)
        {
            const HavokBody& body{ component->Bodies[index] };
            const std::uint32_t info{ body.FilterInfo };

            file << std::format("hk-body {} address 0x{:X} info {:08X} layer {} subsystem {} ignores {} group {} motion {}\n",
                index, body.Address, info, info & k_FilterFieldMask, (info >> k_SubsystemShift) & k_FilterFieldMask,
                (info >> k_IgnoredSubsystemShift) & k_FilterFieldMask, info >> k_SystemGroupShift, static_cast<int>(body.MotionType));
        }

        for (const HavokPhantom& phantom : component->Phantoms)
        {
            const std::uint32_t info{ phantom.FilterInfo };

            file << std::format("hk-phantom 0x{:X} address 0x{:X} info {:08X} layer {} subsystem {} ignores {} group {}\n",
                phantom.Slot, phantom.Address, info, info & k_FilterFieldMask, (info >> k_SubsystemShift) & k_FilterFieldMask,
                (info >> k_IgnoredSubsystemShift) & k_FilterFieldMask, info >> k_SystemGroupShift);
        }
    }

    // Writes the flags of the collision test of projectiles, or a line saying they were not read.
    auto WriteCollisionTestFlags(std::ofstream& file, const std::optional<std::uint64_t>& flags) -> void
    {
        if (!flags)
        {
            file << "ct-flags none\n";
            return;
        }

        file << std::format("ct-flags {:016X}\n", *flags);
    }

    // return: Model tag name of the object definition, or empty if its class has no definition or the tag is missing.
    // note: The definition stores must be frozen.
    auto ModelTagNameOf(const DefinitionsStore& definitions, ObjectKind kind, const std::string& tagName) -> std::string
    {
        const auto modelOf = [&tagName](const auto& store) -> std::string
        {
            const auto* definition{ store.Get(tagName) };
            return definition ? definition->Base.ModelTagName : std::string{};
        };

        switch (kind)
        {
        case ObjectKind::Biped: return modelOf(definitions.m_Bipd);
        case ObjectKind::Vehicle: return modelOf(definitions.m_Vehi);
        case ObjectKind::Weapon: return modelOf(definitions.m_Weap);
        case ObjectKind::Equipment: return modelOf(definitions.m_Eqip);
        case ObjectKind::Projectile: return modelOf(definitions.m_Proj);
        case ObjectKind::Scenery: return modelOf(definitions.m_Scen);
        case ObjectKind::Machine: return modelOf(definitions.m_Mach);
        case ObjectKind::Control: return modelOf(definitions.m_Ctrl);
        case ObjectKind::Crate: return modelOf(definitions.m_Bloc);
        default: return {};
        }
    }

    // Writes the physics model of an object: its rigid bodies, their primitive shapes with the layer overrides of the shape and of
    // its material, and its phantom types. Static tag data, written with every capture for convenience.
    auto WritePhmo(std::ofstream& file, const Phmo* phmo) -> void
    {
        if (!phmo)
        {
            file << "hk-phmo none\n";
            return;
        }

        file << std::format("hk-phmo {} rigidbodies {} materials {} phantomtypes {}\n",
            phmo->TagName, phmo->RigidBodies.size(), phmo->Materials.size(), phmo->PhantomTypes.size());

        for (std::size_t bodyIndex{ 0 }; bodyIndex < phmo->RigidBodies.size(); ++bodyIndex)
        {
            const RigidBody& body{ phmo->RigidBodies[bodyIndex] };

            file << std::format("hk-rigid {} node {} region {} permutation {} motion {} size {} flags {:04X} mask {:08X} root {} {} shapes {}\n",
                bodyIndex, body.NodeIndex, body.RegionIndex, body.PermutationIndex, static_cast<int>(body.MotionType), body.Size,
                body.Flags, body.CollisionGroupMask, static_cast<int>(body.RootShapeType), body.RootShapeIndex, body.Shapes.size());

            for (std::size_t shapeIndex{ 0 }; shapeIndex < body.Shapes.size(); ++shapeIndex)
            {
                const Shape& shape{ body.Shapes[shapeIndex] };

                std::uint8_t materialGroup{ k_NoCollisionGroup };
                const auto material{ static_cast<std::size_t>(shape.MaterialIndex) };
                if (shape.MaterialIndex >= 0 && material < phmo->Materials.size()) materialGroup = phmo->Materials[material].CollisionGroup;

                file << std::format("hk-shape {} {} type {} index {} material {} group {} materialgroup {} phantomtype {} flags {:02X} phantom {}\n",
                    bodyIndex, shapeIndex, static_cast<int>(shape.Type), shape.Index, static_cast<int>(shape.MaterialIndex),
                    static_cast<int>(shape.CollisionGroup), static_cast<int>(materialGroup), static_cast<int>(shape.PhantomTypeIndex),
                    shape.MaterialFlags, shape.IsPhantom ? 1 : 0);
            }
        }

        for (std::size_t index{ 0 }; index < phmo->PhantomTypes.size(); ++index)
        {
            const PhantomType& type{ phmo->PhantomTypes[index] };

            file << std::format("hk-phantomtype {} flags {:08X} marker {:08X} alignment {:08X} direction {} {} center {} {} axis {} {} orbit {} {}\n",
                index, type.Flags, type.MarkerName, type.AlignmentMarkerName, type.DirectionAcceleration, type.DirectionMaximumVelocity,
                type.CenterAcceleration, type.CenterMaximumVelocity, type.AxisAcceleration, type.AxisMaximumVelocity,
                type.OrbitAcceleration, type.OrbitMaximumVelocity);
        }
    }

    // Appends the runs of bytes that differ from the previous capture, one run never crossing a range.
    // return: Changed bytes per range, as "unit 12, eqip 9", or empty if nothing changed.
    // note: previous has the same size as the datum.
    auto DiffObject(const RawObject& datum, const std::vector<std::uint8_t>& previous, std::string_view fourCC,
        std::span<const ComponentRange> cuts, std::vector<ByteChange>& changes) -> std::string
    {
        std::string summary{};

        for (const ComponentRange& cut : cuts)
        {
            std::size_t changed{ 0 };
            std::size_t offset{ cut.Start };

            while (offset < cut.End)
            {
                if (datum.Bytes[offset] == previous[offset])
                {
                    ++offset;
                    continue;
                }

                std::size_t end{ offset };
                while (end < cut.End && datum.Bytes[end] != previous[end]) ++end;

                ByteChange change{ .Handle = datum.Handle, .FourCC = std::string(fourCC), .Component = cut.Name, .Offset = offset };
                change.Before.assign(previous.data() + offset, previous.data() + end);
                change.After.assign(datum.Bytes.data() + offset, datum.Bytes.data() + end);
                changes.push_back(std::move(change));

                changed += end - offset;
                offset = end;
            }

            if (changed == 0) continue;
            if (!summary.empty()) summary += ", ";
            summary += std::format("{} {}", cut.Name, changed);
        }

        return summary;
    }
}

namespace DevTools::Workbench::Capture::System
{
    auto CaptureUIService::Update() -> void
    {
        if (!m_CaptureUIStore.ConsumeCaptureRequest()) return;

        m_CaptureUIStore.SetStatus(this->Capture());
    }

    auto CaptureUIService::Draw() -> void
    {
        this->DrawControls();
        this->DrawPinned();
        this->DrawChanges();
    }

    auto CaptureUIService::Capture() -> std::string
    {
        const std::shared_ptr<const Snapshot> snapshot{ m_CaptureStore.Acquire() };
        if (!snapshot) return "No datum snapshot yet. Load a map and try again.";

        const std::shared_ptr<const Tick> tick{ m_TickStore.Acquire() };
        if (!tick) return "No tick yet. Load a map and try again.";

        const std::vector<Target> targets{ this->CollectTargets(*tick, *snapshot) };
        if (targets.empty() && !m_CaptureUIStore.IsEngineIncluded()) return "Nothing to capture: no local biped and no pinned object alive.";

        std::optional<std::ofstream> file{ this->OpenSessionFile() };
        if (!file) return "Storage Folder is not set (Settings) or the capture file cannot be opened.";

        const std::uint32_t index{ m_CaptureUIStore.GetCaptureCount() + 1 };

        std::string label{ m_CaptureUIStore.GetLabel().data() };
        std::erase(label, '"');

        *file << std::format("== capture {} generation {} time {:%H:%M:%S} label \"{}\"\n", index, tick->Generation, LocalNow(), label);
        WriteHavokFilter(*file, this->ReadHavokFilter());
        WriteCollisionTestFlags(*file, this->ReadCollisionTestFlags());

        const std::size_t engineBlocks{ m_CaptureUIStore.IsEngineIncluded() ? this->WriteEngineBlocks(*file, m_CaptureStore.GetThreadContext()) : 0 };
        if (m_CaptureUIStore.IsEngineIncluded()) this->WriteGameState(*file, m_CaptureStore.GetThreadContext(), index);

        std::vector<ByteChange> changes{};
        std::string details{};

        for (const Target& target : targets)
        {
            const RawObject& datum{ snapshot->RawObjects.at(target.Handle) };

            std::string fourCC{ "????" };
            std::string tagName{ "?" };

            if (tick->ObjectTable)
            {
                const auto it{ tick->ObjectTable->find(target.Handle) };
                if (it != tick->ObjectTable->end())
                {
                    fourCC = it->second.FourCC;
                    tagName = it->second.TagName;
                }
            }

            const std::vector<ComponentRange> cuts{ CutDatum(datum) };
            WriteObject(*file, datum, target.Source, fourCC, tagName, cuts);

            const std::uint32_t havokHandle{ datum.Read<std::uint32_t>(ObjeOffset::k_HavokComponentHandle) };
            WriteHavokComponent(*file, havokHandle, this->ReadHavokComponent(havokHandle));
            WritePhmo(*file, this->FindPhmo(ToObjectKind(datum.KindId), tagName));

            const Hlmt* hlmt{ this->FindHlmt(ToObjectKind(datum.KindId), tagName) };
            const bool hasColl{ hlmt && !hlmt->CollisionModelTagName.empty() };
            *file << std::format("ct-coll {}\n", hasColl ? hlmt->CollisionModelTagName : std::string{ "none" });

            const std::vector<std::uint8_t>* previous{ m_CaptureUIStore.FindPrevious(target.Handle) };
            const bool isComparable{ previous != nullptr && previous->size() == datum.Bytes.size() };
            const std::string summary{ isComparable ? DiffObject(datum, *previous, fourCC, cuts, changes) : "new" };

            if (!summary.empty())
            {
                if (!details.empty()) details += ". ";
                details += std::format("{} {:08X} {}", fourCC, target.Handle, summary);
            }

            m_CaptureUIStore.SetPrevious(target.Handle, datum.Bytes);
        }

        m_CaptureUIStore.SetChanges(std::move(changes));
        m_CaptureUIStore.SetCaptureCount(index);

        const std::string labelText{ label.empty() ? "" : std::format(" ({})", label) };
        if (details.empty()) details = "Nothing changed";

        m_Log.Message("[CaptureUIService] INFO: Capture {}{}, {} objects, {} engine blocks. {}.", index, labelText, targets.size(),
            engineBlocks, details);

        return std::format("Capture {}{}: {} objects -> {}", index, labelText, targets.size(), m_CaptureUIStore.GetFilePath().string());
    }

    auto CaptureUIService::CollectTargets(const Tick& tick, const Snapshot& snapshot) const -> std::vector<Target>
    {
        std::vector<Target> targets{};

        if (tick.Self) AddWithLinks(targets, snapshot, tick.Self->BipedHandle, Source::Self);

        if (m_CaptureUIStore.IsPlayersIncluded() && tick.PlayerTable)
        {
            for (const auto& [handle, player] : *tick.PlayerTable)
            {
                AddWithLinks(targets, snapshot, player.CurrentBipedHandle, Source::Player);
            }
        }

        for (const std::uint32_t handle : m_CaptureUIStore.GetPinned())
        {
            AddWithLinks(targets, snapshot, handle, Source::Pinned);
        }

        if (m_CaptureUIStore.IsProjectilesIncluded())
        {
            for (const auto& [handle, datum] : snapshot.RawObjects)
            {
                if (ToObjectKind(datum.KindId) == ObjectKind::Projectile) AddTarget(targets, snapshot, handle, Source::Projectile);
            }
        }

        return targets;
    }

    auto CaptureUIService::ReadHavokFilter() -> std::optional<HavokFilter>
    {
        const std::uintptr_t module{ GameModuleBase() };
        if (module == 0) return std::nullopt;

        HavokFilter filter{};

        filter.World = m_MemoryReaderService.Read<std::uintptr_t>(module, k_HavokWorldRva);
        if (filter.World == 0) return std::nullopt;

        filter.Filter = m_MemoryReaderService.Read<std::uintptr_t>(filter.World, k_WorldFilterOffset);
        if (filter.Filter == 0) return std::nullopt;

        const std::size_t bytes{ sizeof(std::uint32_t) * filter.Lookup.size() };
        if (!m_MemoryReaderService.ReadRaw(filter.Filter + k_FilterLookupOffset, filter.Lookup.data(), bytes)) return std::nullopt;

        return filter;
    }

    auto CaptureUIService::ReadHavokComponent(std::uint32_t handle) -> std::optional<HavokComponent>
    {
        if (handle == k_NoHandle) return std::nullopt;

        const std::uintptr_t module{ GameModuleBase() };
        if (module == 0) return std::nullopt;

        const auto array{ m_MemoryReaderService.Read<std::uintptr_t>(module, k_HavokComponentArrayRva) };
        if (array == 0) return std::nullopt;

        const auto elements{ m_MemoryReaderService.Read<std::uintptr_t>(array, k_DataArrayData) };
        if (elements == 0) return std::nullopt;

        const std::uintptr_t address{ elements + (handle & k_HandleIndexMask) * k_HavokComponentSize };

        HavokComponent component
        {
            .Handle = handle,
            .Address = address,
            .Salt = m_MemoryReaderService.Read<std::uint16_t>(address, 0),
            .Flags = m_MemoryReaderService.Read<std::uint32_t>(address, k_ComponentFlagsOffset),
            .Owner = m_MemoryReaderService.Read<std::uint32_t>(address, k_ComponentOwnerOffset),
            .Size = m_MemoryReaderService.Read<std::int8_t>(address, k_ComponentSizeOffset),
            .BodyCount = m_MemoryReaderService.Read<std::int32_t>(address, k_ComponentBodyCountOffset),
        };

        for (const std::uintptr_t slot : k_ComponentPhantomOffsets)
        {
            const auto phantom{ m_MemoryReaderService.Read<std::uintptr_t>(address, slot) };
            if (phantom == 0) continue;

            component.Phantoms.push_back(HavokPhantom
            {
                .Slot = slot,
                .Address = phantom,
                .FilterInfo = m_MemoryReaderService.Read<std::uint32_t>(phantom, k_RigidBodyFilterInfoOffset),
            });
        }

        if (component.BodyCount <= 0 || component.BodyCount > k_MaxHavokBodies) return component;

        const auto entries{ m_MemoryReaderService.Read<std::uintptr_t>(address, k_ComponentBodiesOffset) };
        if (entries == 0) return component;

        for (std::int32_t index{ 0 }; index < component.BodyCount; ++index)
        {
            const std::uintptr_t entry{ entries + static_cast<std::uintptr_t>(index) * k_BodyEntrySize };
            HavokBody body{ .Address = m_MemoryReaderService.Read<std::uintptr_t>(entry, k_BodyEntryRigidBodyOffset) };

            if (body.Address != 0)
            {
                body.FilterInfo = m_MemoryReaderService.Read<std::uint32_t>(body.Address, k_RigidBodyFilterInfoOffset);
                body.MotionType = m_MemoryReaderService.Read<std::uint8_t>(body.Address, k_RigidBodyMotionTypeOffset);
            }

            component.Bodies.push_back(body);
        }

        return component;
    }

    auto CaptureUIService::WriteEngineBlocks(std::ofstream& file, std::uintptr_t threadContext) -> std::size_t
    {
        if (threadContext == 0) return 0;

        std::size_t written{ 0 };

        for (std::size_t index{ 0 }; index < k_EngineBlocks.size(); ++index)
        {
            const EngineBlock& block{ k_EngineBlocks[index] };

            std::uintptr_t address{ threadContext };
            if (block.IsIndirect) address = m_MemoryReaderService.Read<std::uintptr_t>(threadContext, block.PointerOffset);
            if (address != 0 && block.InnerOffset != 0) address = m_MemoryReaderService.Read<std::uintptr_t>(address, block.InnerOffset);
            if (address == 0) continue;

            const std::vector<std::uint8_t> bytes{ this->ReadPages(address, block.MaximumSize) };
            if (bytes.empty()) continue;

            file << std::format("-- object {:08X} source engine class {} size 0x{:X} address 0x{:X} tag engine\\{}\n",
                k_EngineHandleBase + static_cast<std::uint32_t>(index), block.Name, bytes.size(), address, block.Name);
            file << std::format(".. {} 0x0 0x{:X}\n", block.Name, bytes.size());

            for (std::size_t offset{ 0 }; offset < bytes.size(); offset += k_BytesPerRow)
            {
                const std::size_t count{ (std::min)(k_BytesPerRow, bytes.size() - offset) };
                file << std::format("{:04X}  {}\n", offset, HexBytes(std::span<const std::uint8_t>{ bytes.data() + offset, count }));
            }

            ++written;
        }

        return written;
    }

    auto CaptureUIService::WriteGameState(std::ofstream& file, std::uintptr_t threadContext, std::uint32_t index) -> std::size_t
    {
        if (threadContext == 0) return 0;

        const auto anchor{ m_MemoryReaderService.Read<std::uintptr_t>(threadContext, k_ThreadContextGameModeOffset) };
        if (anchor == 0) return 0;

        MEMORY_BASIC_INFORMATION info{};
        if (VirtualQuery(reinterpret_cast<const void*>(anchor), &info, sizeof(info)) == 0) return 0;

        const auto base{ reinterpret_cast<std::uintptr_t>(info.AllocationBase) };

        std::vector<MEMORY_BASIC_INFORMATION> regions{};
        std::uintptr_t cursor{ base };

        while (cursor - base < k_MaxGameStateSize)
        {
            MEMORY_BASIC_INFORMATION region{};
            if (VirtualQuery(reinterpret_cast<const void*>(cursor), &region, sizeof(region)) == 0) break;
            if (region.AllocationBase != info.AllocationBase) break;

            regions.push_back(region);
            cursor = reinterpret_cast<std::uintptr_t>(region.BaseAddress) + region.RegionSize;
        }

        const std::size_t size{ (std::min)(static_cast<std::size_t>(cursor - base), k_MaxGameStateSize) };
        if (size == 0) return 0;

        std::vector<std::uint8_t> bytes(size);

        for (const MEMORY_BASIC_INFORMATION& region : regions)
        {
            if (region.State != MEM_COMMIT || (region.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) continue;

            const std::size_t offset{ reinterpret_cast<std::uintptr_t>(region.BaseAddress) - base };
            const std::size_t count{ (std::min)(region.RegionSize, size - offset) };

            for (std::size_t page{ 0 }; page < count; page += k_PageSize)
            {
                const std::size_t pageCount{ (std::min)(k_PageSize, count - page) };
                m_MemoryReaderService.ReadRaw(base + offset + page, bytes.data() + offset + page, pageCount);
            }
        }

        const path sessionPath{ m_CaptureUIStore.GetFilePath() };
        const std::string fileName{ std::format("{}_gs{}.bin", sessionPath.stem().string(), index) };

        std::ofstream binary{ sessionPath.parent_path() / fileName, std::ios::binary | std::ios::trunc };
        if (!binary) return 0;

        binary.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        file << std::format("gs-dump {} base 0x{:X} size 0x{:X} context 0x{:X}\n", fileName, base, size, threadContext);

        return size;
    }

    auto CaptureUIService::ReadPages(std::uintptr_t address, std::size_t maximum) -> std::vector<std::uint8_t>
    {
        std::vector<std::uint8_t> bytes{};
        bytes.reserve(maximum);

        std::uintptr_t cursor{ address };

        while (bytes.size() < maximum)
        {
            const std::uintptr_t pageEnd{ (cursor & ~(k_PageSize - 1)) + k_PageSize };
            const std::size_t count{ (std::min)(static_cast<std::size_t>(pageEnd - cursor), maximum - bytes.size()) };

            std::vector<std::uint8_t> chunk(count);
            if (!m_MemoryReaderService.ReadRaw(cursor, chunk.data(), count)) break;

            bytes.insert(bytes.end(), chunk.begin(), chunk.end());
            cursor += count;
        }

        return bytes;
    }

    auto CaptureUIService::ReadCollisionTestFlags() -> std::optional<std::uint64_t>
    {
        const std::uintptr_t module{ GameModuleBase() };
        if (module == 0) return std::nullopt;

        std::uint64_t flags{};
        if (!m_MemoryReaderService.ReadRaw(module + k_CollisionTestFlagsRva, &flags, sizeof(flags))) return std::nullopt;

        return flags;
    }

    auto CaptureUIService::FindHlmt(ObjectKind kind, const std::string& tagName) const -> const Hlmt*
    {
        if (!m_DefinitionsStore.m_Hlmt.IsFrozen()) return nullptr;

        const std::string modelTagName{ ModelTagNameOf(m_DefinitionsStore, kind, tagName) };
        if (modelTagName.empty()) return nullptr;

        return m_DefinitionsStore.m_Hlmt.Get(modelTagName);
    }

    auto CaptureUIService::FindPhmo(ObjectKind kind, const std::string& tagName) const -> const Phmo*
    {
        if (!m_DefinitionsStore.m_Phmo.IsFrozen()) return nullptr;

        const Hlmt* hlmt{ this->FindHlmt(kind, tagName) };
        if (!hlmt || hlmt->PhysicsModelTagName.empty()) return nullptr;

        return m_DefinitionsStore.m_Phmo.Get(hlmt->PhysicsModelTagName);
    }

    auto CaptureUIService::OpenSessionFile() -> std::optional<std::ofstream>
    {
        const std::string directory{ m_SettingsStore.GetAppDataDirectory() };
        if (directory.empty()) return std::nullopt;

        const bool isNewSession{ m_CaptureUIStore.GetFilePath().empty() };

        if (isNewSession)
        {
            const path folder{ path(directory) / k_CaptureFolder };
            std::error_code error{};
            std::filesystem::create_directories(folder, error);

            m_CaptureUIStore.SetFilePath(folder / std::format("{:%Y%m%d_%H%M%S}.txt", LocalNow()));
        }

        std::ofstream file{ m_CaptureUIStore.GetFilePath(), std::ios::app };
        if (!file) return std::nullopt;

        if (isNewSession)
        {
            file << "# Artemis datum captures (D81). Offsets and ranges are absolute datum offsets.\n";
            file << "# hk-* lines: Havok filter layer table per capture, Havok component and physics model of each object (A10).\n";
            file << "# ct-* lines: flags of the projectile collision test per capture and collision model of each object (A13).\n";
        }

        return file;
    }

    auto CaptureUIService::DrawControls() -> void
    {
        ImGui::TextWrapped("F7 or Capture copies the datum of the local biped, its weapons, its armor ability (with its bubble or hologram), "
            "its vehicle and the pinned objects "
            "to a session file in the storage folder, with the Havok component of each one and the Havok layer table (A10). "
            "Pin objects from the Object Table cards.");

        std::array<char, k_LabelSize>& label{ m_CaptureUIStore.GetLabel() };
        ImGui::SetNextItemWidth(260.0f);
        ImGui::InputText("Label", label.data(), label.size());
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Name of the state the next captures show, such as \"camo active\".");

        bool isPlayersIncluded{ m_CaptureUIStore.IsPlayersIncluded() };
        if (ImGui::Checkbox("Every player", &isPlayersIncluded)) m_CaptureUIStore.SetPlayersIncluded(isPlayersIncluded);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Also captures the biped of every other player, with its weapons, armor ability and vehicle.");

        ImGui::SameLine();
        bool isProjectilesIncluded{ m_CaptureUIStore.IsProjectilesIncluded() };
        if (ImGui::Checkbox("Every projectile", &isProjectilesIncluded)) m_CaptureUIStore.SetProjectilesIncluded(isProjectilesIncluded);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Also captures every projectile alive: grenades, rockets, stuck needles (A3).");

        ImGui::SameLine();
        bool isEngineIncluded{ m_CaptureUIStore.IsEngineIncluded() };
        if (ImGui::Checkbox("Engine globals", &isEngineIncluded)) m_CaptureUIStore.SetEngineIncluded(isEngineIncluded);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Also captures the engine globals of the thread context (game, time, game engine, players) and "
            "copies the whole game state to a .bin file per capture next to the session file (A5). Works without a local biped.");

        if (ImGui::Button("Capture")) m_CaptureUIStore.RequestCapture();
        ImGui::SameLine();

        if (ImGui::Button("New File"))
        {
            m_CaptureUIStore.ResetSession();
            m_CaptureUIStore.SetStatus("The next capture starts a new file.");
        }

        ImGui::SameLine();
        ImGui::Text("Captures: %u", m_CaptureUIStore.GetCaptureCount());

        const std::string& status{ m_CaptureUIStore.GetStatus() };
        if (!status.empty()) ImGui::TextWrapped("%s", status.c_str());
    }

    auto CaptureUIService::DrawPinned() -> void
    {
        ImGui::SeparatorText("Pinned objects");

        const std::vector<std::uint32_t> pinned{ m_CaptureUIStore.GetPinned() };

        if (pinned.empty())
        {
            ImGui::TextDisabled("None.");
            return;
        }

        for (const std::uint32_t handle : pinned)
        {
            ImGui::PushID(static_cast<int>(handle));

            ImGui::Text("%08X", handle);
            ImGui::SameLine();
            if (ImGui::SmallButton("Unpin")) m_CaptureUIStore.SetPinned(handle, false);

            ImGui::PopID();
        }
    }

    auto CaptureUIService::DrawChanges() -> void
    {
        ImGui::SeparatorText("Changes since the previous capture");

        int& filter{ m_CaptureUIStore.GetComponentFilter() };
        ImGui::SetNextItemWidth(120.0f);
        ImGui::Combo("Component", &filter, k_FilterNames.data(), static_cast<int>(k_FilterNames.size()));

        const auto index{ static_cast<std::size_t>(filter) };
        const std::string_view selected{ index > 0 && index < k_FilterNames.size() ? k_FilterNames[index] : "" };

        constexpr ImGuiTableFlags k_TableFlags{ ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit };

        if (!ImGui::BeginTable("##changes", 5, k_TableFlags, ImVec2(0.0f, ImGui::GetContentRegionAvail().y))) return;

        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Object");
        ImGui::TableSetupColumn("Component");
        ImGui::TableSetupColumn("Offset");
        ImGui::TableSetupColumn("Before");
        ImGui::TableSetupColumn("After", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        int rows{ 0 };

        for (const ByteChange& change : m_CaptureUIStore.GetChanges())
        {
            if (!selected.empty() && selected != change.Component) continue;
            if (rows++ >= k_MaxChangeRows) break;

            const std::string object{ std::format("{} {:08X}", change.FourCC, change.Handle) };
            const std::string offset{ std::format("0x{:X}", change.Offset) };
            const std::string before{ HexBytes(change.Before) };
            const std::string after{ HexBytes(change.After) };

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(object.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(change.Component);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(offset.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(before.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(after.c_str());
        }

        ImGui::EndTable();
    }
}