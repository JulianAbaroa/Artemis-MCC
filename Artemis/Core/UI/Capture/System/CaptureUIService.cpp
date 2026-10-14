module;

#include "External/imgui/imgui.h"

module UI.Capture.System;

import Resolved.Definitions.Type;
import std;

namespace
{
    using std::filesystem::path;
    using std::chrono::system_clock;
    using std::chrono::seconds;
    using std::chrono::zoned_time;

    using Resolved::Definitions::Type::Object::ObjectKind;
    using Datum::Object::Type::Raw::RawObject;
    using Datum::Object::Type::Raw::Snapshot;
    using UI::Capture::Type::ComponentRange;
    using UI::Capture::Type::Source;
    using UI::Capture::Type::Target;
    using UI::Capture::Type::ByteChange;

    using UI::Capture::Type::k_LabelSize;

    namespace ObjeOffset = Datum::Object::Type::Obje::Offset;
    namespace AnimOffset = Datum::Object::Type::Anim::Offset;
    namespace UnitOffset = Datum::Object::Type::Unit::Offset;
    namespace BipdOffset = Datum::Object::Type::Bipd::Offset;
    namespace VehiOffset = Datum::Object::Type::Vehi::Offset;
    namespace ItemOffset = Datum::Object::Type::Item::Offset;
    namespace WeapOffset = Datum::Object::Type::Weap::Offset;
    namespace EqipOffset = Datum::Object::Type::Eqip::Offset;
    namespace ProjOffset = Datum::Object::Type::Proj::Offset;
    namespace ScenOffset = Datum::Object::Type::Scen::Offset;
    namespace BlocOffset = Datum::Object::Type::Bloc::Offset;
    namespace DeviOffset = Datum::Object::Type::Devi::Offset;
    namespace MachOffset = Datum::Object::Type::Mach::Offset;
    namespace CtrlOffset = Datum::Object::Type::Ctrl::Offset;

    // Folder inside the storage folder that receives one file per capture session.
    constexpr std::string_view k_CaptureFolder{ "Captures" };

    // Handle of an empty slot.
    constexpr std::uint32_t k_NoHandle{ 0xFFFFFFFF };

    // Bytes per hex row of the session file.
    constexpr std::size_t k_BytesPerRow{ 16 };

    // Rows the change table draws at most.
    constexpr int k_MaxChangeRows{ 2000 };

    // Name of the range after the last component: the variable blocks of the datum.
    constexpr const char* k_BlocksName{ "blocks" };

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

    // Components of each object class, in datum order (Tareas/Transversal/CreateObject.txt).
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

        for (const ComponentRange& component : ComponentsOf(datum.Kind))
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
        default: return "unknown";
        }
    }

    // return: The current local time, to the second.
    auto LocalNow() -> zoned_time<seconds>
    {
        return zoned_time{ std::chrono::current_zone(), std::chrono::floor<seconds>(system_clock::now()) };
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

    // Adds an object and, if it is a biped, its weapons, its armor ability and its parent (the vehicle it sits in).
    auto AddWithLinks(std::vector<Target>& targets, const Snapshot& snapshot, std::uint32_t handle, Source source) -> void
    {
        AddTarget(targets, snapshot, handle, source);

        const auto it{ snapshot.RawObjects.find(handle) };
        if (it == snapshot.RawObjects.end()) return;

        const RawObject& datum{ it->second };
        if (datum.Kind != ObjectKind::Biped) return;

        AddTarget(targets, snapshot, datum.Read<std::uint32_t>(UnitOffset::k_PrimaryWeaponHandle), Source::Linked);
        AddTarget(targets, snapshot, datum.Read<std::uint32_t>(UnitOffset::k_SecondaryWeaponHandle), Source::Linked);
        AddTarget(targets, snapshot, datum.Read<std::uint32_t>(UnitOffset::k_ArmorAbilityHandle), Source::Linked);
        AddTarget(targets, snapshot, datum.Read<std::uint32_t>(ObjeOffset::k_ParentHandle), Source::Linked);
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

namespace UI::Capture::System
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
        const std::shared_ptr<const Snapshot> snapshot{ m_DatumStore.Acquire() };
        if (!snapshot) return "No datum snapshot yet. Load a map and try again.";

        const std::shared_ptr<const Tick> tick{ m_TickStore.Acquire() };
        if (!tick) return "No tick yet. Load a map and try again.";

        const std::vector<Target> targets{ this->CollectTargets(*tick, *snapshot) };
        if (targets.empty()) return "Nothing to capture: no local biped and no pinned object alive.";

        std::optional<std::ofstream> file{ this->OpenSessionFile() };
        if (!file) return "Storage Folder is not set (Settings) or the capture file cannot be opened.";

        const std::uint32_t index{ m_CaptureUIStore.GetCaptureCount() + 1 };

        std::string label{ m_CaptureUIStore.GetLabel().data() };
        std::erase(label, '"');

        *file << std::format("== capture {} generation {} time {:%H:%M:%S} label \"{}\"\n", index, tick->Generation, LocalNow(), label);

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

        m_LogsService.Message("[CaptureUIService] INFO: Capture {}{}, {} objects. {}.", index, labelText, targets.size(), details);

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

        return targets;
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

        if (isNewSession) file << "# Artemis datum captures (D81). Offsets and ranges are absolute datum offsets.\n";

        return file;
    }

    auto CaptureUIService::DrawControls() -> void
    {
        ImGui::TextWrapped("F7 or Capture copies the datum of the local biped, its weapons, its armor ability, its vehicle and the pinned objects "
            "to a session file in the storage folder. Pin objects from the Object Table cards.");

        std::array<char, k_LabelSize>& label{ m_CaptureUIStore.GetLabel() };
        ImGui::SetNextItemWidth(260.0f);
        ImGui::InputText("Label", label.data(), label.size());
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Name of the state the next captures show, such as \"camo active\".");

        bool isPlayersIncluded{ m_CaptureUIStore.IsPlayersIncluded() };
        if (ImGui::Checkbox("Every player", &isPlayersIncluded)) m_CaptureUIStore.SetPlayersIncluded(isPlayersIncluded);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Also captures the biped of every other player, with its weapons, armor ability and vehicle.");

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