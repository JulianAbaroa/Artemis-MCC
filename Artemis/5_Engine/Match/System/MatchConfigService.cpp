module Engine.Match.System;
import :MatchConfig;

import Engine.Match.Type;
import Blam.Layout.Object.Type;
import std;

namespace
{
    using std::filesystem::path;
    using Engine::Match::Type::MatchConfig;

    namespace LayoutMapVariant = Blam::Layout::Object::Type::MapVariant;

    using Blam::Layout::Object::Type::Constant::k_ConfigurationMapVariantOffset;
    using Blam::Layout::Object::Type::Constant::k_ForgeWorldMapId;
    using Blam::Layout::Object::Type::Constant::k_MapVariantVersion;
    using Blam::Layout::Object::Type::Constant::k_MapVariantSize;

    using Engine::Match::Type::k_EngineConfigurationSize;
    using Engine::Match::Type::k_ConfigurationBaseMapOffset;
    using Engine::Match::Type::k_TransplantBlocks;
    using Engine::Match::Type::k_MaxMatchConfigNameLength;

    // Folder inside the storage folder that holds the recorded configurations, and the extension of their files.
    constexpr std::string_view k_LibraryFolder{ "MatchConfigs" };
    constexpr std::string_view k_ConfigExtension{ ".bin" };

    // Names Windows opens as devices instead of files, whatever the extension.
    constexpr std::array<std::string_view, 22> k_DeviceNames{
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9",
    };

    // Folder of the library where Delete moves the files, outside the listing (the listing does not enter subfolders, D283).
    constexpr std::string_view k_DeletedFolder{ "_deleted" };

    // Copies of a name that Delete keeps before it stops numbering them.
    constexpr std::size_t k_MaxDeletedCopies{ 1000 };

    // return: Why the name cannot be the file name of a configuration, empty if it can: letters, digits, spaces, '_', '-' and '.'.
    // note: Windows drops a final space or dot from a file name, so the name cannot end with one. Before the first dot the name cannot
    // be a device name.
    auto FindNameProblem(std::string_view name) -> std::string
    {
        if (name.empty()) return "The name is empty.";
        if (name.size() > k_MaxMatchConfigNameLength) return std::format("The name is longer than {} characters.", k_MaxMatchConfigNameLength);
        if (name.back() == ' ' || name.back() == '.') return "The name cannot end with a space or a dot.";

        const bool isAllowed{ std::ranges::all_of(name, [](char c) {
            return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == ' ' || c == '_' || c == '-' || c == '.';
            }) };

        if (!isAllowed) return "Only letters, digits, spaces, '_', '-' and '.' are allowed.";

        const std::string_view stem{ name.substr(0, name.find('.')) };

        const bool isDevice{ std::ranges::any_of(k_DeviceNames, [stem](std::string_view device) {
            return std::ranges::equal(stem, device, [](char a, char b) {
                return std::toupper(static_cast<unsigned char>(a)) == std::toupper(static_cast<unsigned char>(b));
                });
            }) };

        return isDevice ? "The name is a reserved device name of Windows." : std::string{};
    }

    auto IsValidName(std::string_view name) -> bool
    {
        return FindNameProblem(name).empty();
    }

    // return: The path as UTF-8. Unlike path::string, it does not throw on characters outside the code page.
    auto ToText(const path& value) -> std::string
    {
        const std::u8string text{ value.u8string() };
        return std::string{ text.begin(), text.end() };
    }

    // return: The id of the base map of a configuration.
    auto ReadBaseMap(const std::byte* configuration) -> std::uint32_t
    {
        std::uint32_t id{ 0 };
        std::memcpy(&id, configuration + k_ConfigurationBaseMapOffset, sizeof(id));
        return id;
    }
}

namespace Engine::Match::System
{
    auto MatchConfigService::ListConfigs() const -> std::vector<std::string>
    {
        std::vector<std::string> names{};

        const path folder{ this->GetLibraryFolder() };
        if (folder.empty()) return names;

        // The increment with an error code: the one of a range-for throws.
        std::error_code error{};
        std::filesystem::directory_iterator iterator{ folder, error };

        for (; !error && iterator != std::filesystem::directory_iterator{}; iterator.increment(error))
        {
            const auto& entry{ *iterator };
            std::error_code fileError{};

            if (!entry.is_regular_file(fileError) || entry.path().extension() != k_ConfigExtension) continue;
            names.push_back(ToText(entry.path().stem()));
        }

        std::ranges::sort(names);
        return names;
    }

    auto MatchConfigService::LoadPending(std::string_view name) -> bool
    {
        auto config{ this->ReadConfig(name) };
        if (!config) return false;

        const std::uint32_t baseMap{ ReadBaseMap(config->Bytes.data()) };
        m_MatchConfigStore.SetPending(std::move(config));

        m_Log.Message("[MatchConfigService] INFO: Configuration \"{}\" (base map 0x{:X}) is pending for the next start.",
            name, baseMap);
        return true;
    }

    auto MatchConfigService::ReadConfig(std::string_view name) const -> std::shared_ptr<MatchConfig>
    {
        if (!IsValidName(name))
        {
            m_Log.Message("[MatchConfigService] WARNING: \"{}\" is not a valid configuration name.", name);
            return nullptr;
        }

        const path folder{ this->GetLibraryFolder() };

        if (folder.empty())
        {
            m_Log.Message("[MatchConfigService] WARNING: Storage Folder is not set, no configuration can be read.");
            return nullptr;
        }

        const path file{ folder / std::format("{}{}", name, k_ConfigExtension) };
        std::ifstream stream{ file, std::ios::binary | std::ios::ate };

        if (!stream)
        {
            m_Log.Message("[MatchConfigService] ERROR: Failed to open {}.", ToText(file));
            return nullptr;
        }

        const std::streamoff size{ stream.tellg() };

        if (size != static_cast<std::streamoff>(k_EngineConfigurationSize))
        {
            m_Log.Message("[MatchConfigService] ERROR: {} has {} bytes, a configuration has {}.", ToText(file),
                static_cast<long long>(size), k_EngineConfigurationSize);
            return nullptr;
        }

        auto config{ std::make_shared<MatchConfig>() };
        config->Name = std::string{ name };
        config->Bytes.resize(k_EngineConfigurationSize);

        stream.seekg(0);

        if (!stream.read(reinterpret_cast<char*>(config->Bytes.data()), static_cast<std::streamsize>(k_EngineConfigurationSize)))
        {
            m_Log.Message("[MatchConfigService] ERROR: Failed to read {}.", ToText(file));
            return nullptr;
        }

        return config;
    }

    auto MatchConfigService::ReadMapVariant(std::string_view name) const -> std::vector<std::byte>
    {
        const auto config{ this->ReadConfig(name) };
        if (!config) return {};

        const auto first{ config->Bytes.begin() + k_ConfigurationMapVariantOffset };
        return std::vector<std::byte>(first, first + k_MapVariantSize);
    }

    auto MatchConfigService::ReadConfigBytes(std::string_view name) const -> std::vector<std::byte>
    {
        const auto config{ this->ReadConfig(name) };
        return config ? config->Bytes : std::vector<std::byte>{};
    }

    auto MatchConfigService::CreateFromMvar(std::string_view templateName, std::span<const std::byte> mapVariant, std::string_view name)
        -> std::string
    {
        const auto refuse{ [&](std::string reason)
        {
            m_Log.Message("[MatchConfigService] WARNING: Configuration \"{}\" not created ({}).", name, reason);
            return reason;
        } };

        if (const std::string problem{ FindNameProblem(name) }; !problem.empty()) return refuse(problem);

        const path folder{ this->GetLibraryFolder() };
        if (folder.empty()) return refuse("Storage Folder is not set.");

        const auto header{ mapVariant.size() == k_MapVariantSize ? LayoutMapVariant::Read(mapVariant) : std::nullopt };
        if (!header) return refuse("The map variant is not a valid block.");
        if (header->Version != k_MapVariantVersion) return refuse(std::format("The map variant has version 0x{:X}.", header->Version));
        if (header->MapId != k_ForgeWorldMapId)
        {
            return refuse(std::format("The map variant is for map {}: only Forge World ({}) is supported.", header->MapId, k_ForgeWorldMapId));
        }

        const auto config{ this->ReadConfig(templateName) };
        if (!config) return refuse(std::format("The template \"{}\" cannot be read.", templateName));

        const std::span<const std::byte> templateBlock{ config->Bytes.data() + k_ConfigurationMapVariantOffset, k_MapVariantSize };
        const auto templateHeader{ LayoutMapVariant::Read(templateBlock) };

        if (!templateHeader || templateHeader->MapId != k_ForgeWorldMapId)
        {
            return refuse(std::format("The template \"{}\" is not a Forge World configuration.", templateName));
        }

        const path file{ folder / std::format("{}{}", name, k_ConfigExtension) };
        std::error_code error{};
        if (std::filesystem::exists(file, error)) return refuse("A configuration with that name already exists.");

        std::vector<std::byte> bytes{ config->Bytes };
        std::ranges::copy(mapVariant, bytes.begin() + k_ConfigurationMapVariantOffset);

        std::filesystem::create_directories(folder, error);
        std::ofstream stream{ file, std::ios::binary | std::ios::trunc };

        if (!stream || !stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
        {
            stream.close();
            std::filesystem::remove(file, error);
            m_Log.Message("[MatchConfigService] ERROR: Failed to write {}.", ToText(file));
            return "The file could not be written. See the log.";
        }

        m_Log.Message("[MatchConfigService] INFO: Configuration \"{}\" created from template \"{}\" with a map variant of {} placements.",
            name, templateName, header->PlacementsInUse);
        return {};
    }

    auto MatchConfigService::SuggestName(std::string_view text) -> std::string
    {
        std::string name{};

        for (const char c : text)
        {
            const std::string candidate{ std::format("a{}a", c) };
            const bool isAllowed{ c != '\0' && FindNameProblem(candidate).empty() };

            if (!isAllowed || (c == ' ' && (name.empty() || name.back() == ' '))) continue;
            if (name.size() >= k_MaxMatchConfigNameLength) break;

            name.push_back(c);
        }

        while (!name.empty() && (name.back() == ' ' || name.back() == '.')) name.pop_back();
        return name;
    }

    auto MatchConfigService::GetNameProblem(std::string_view name) -> std::string
    {
        return FindNameProblem(name);
    }

    auto MatchConfigService::RenameConfig(std::string_view from, std::string_view to) -> std::string
    {
        std::string problem{ FindNameProblem(from) };
        if (problem.empty()) problem = FindNameProblem(to);
        if (!problem.empty()) return problem;

        const path folder{ this->GetLibraryFolder() };
        if (folder.empty()) return "Storage Folder is not set.";

        // The pending configuration keeps the name it was loaded with, so it is the user who clears it first.
        const auto pending{ m_MatchConfigStore.GetPending() };
        if (pending && pending->Name == from) return "It is the pending configuration for the next start. Clear it first.";

        const path source{ folder / std::format("{}{}", from, k_ConfigExtension) };
        const path target{ folder / std::format("{}{}", to, k_ConfigExtension) };

        std::error_code error{};
        if (!std::filesystem::is_regular_file(source, error)) return "The configuration does not exist.";

        // Windows names are case-insensitive: a case-only rename is the same file, anything else that exists is not overwritten.
        const bool isSameName{ std::ranges::equal(from, to, [](char a, char b) {
            return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
            }) };
        if (!isSameName && std::filesystem::exists(target, error)) return "A configuration with that name already exists.";

        std::filesystem::rename(source, target, error);

        if (error)
        {
            m_Log.Message("[MatchConfigService] ERROR: Failed to rename {} to {} ({}).", ToText(source), ToText(target), error.message());
            return "The file could not be renamed. See the log.";
        }

        m_Log.Message("[MatchConfigService] INFO: Configuration \"{}\" renamed to \"{}\".", from, to);
        return {};
    }

    auto MatchConfigService::DeleteConfig(std::string_view name) -> std::string
    {
        const std::string problem{ FindNameProblem(name) };
        if (!problem.empty()) return problem;

        const path folder{ this->GetLibraryFolder() };
        if (folder.empty()) return "Storage Folder is not set.";

        const auto pending{ m_MatchConfigStore.GetPending() };
        if (pending && pending->Name == name) return "It is the pending configuration for the next start. Clear it first.";

        const path source{ folder / std::format("{}{}", name, k_ConfigExtension) };

        std::error_code error{};
        if (!std::filesystem::is_regular_file(source, error)) return "The configuration does not exist.";

        const path deletedFolder{ folder / k_DeletedFolder };
        std::filesystem::create_directories(deletedFolder, error);

        // A file already deleted under the same name stays: the new one takes the first free number.
        path target{ deletedFolder / std::format("{}{}", name, k_ConfigExtension) };

        for (std::size_t number{ 2 }; std::filesystem::exists(target, error) && number < k_MaxDeletedCopies; ++number)
        {
            target = deletedFolder / std::format("{} ({}){}", name, number, k_ConfigExtension);
        }

        std::filesystem::rename(source, target, error);

        if (error)
        {
            m_Log.Message("[MatchConfigService] ERROR: Failed to move {} to {} ({}).", ToText(source), ToText(target), error.message());
            return "The file could not be moved. See the log.";
        }

        m_Log.Message("[MatchConfigService] INFO: Configuration \"{}\" moved to {}.", name, ToText(target));
        return {};
    }

    auto MatchConfigService::ClearPending() -> void
    {
        if (!m_MatchConfigStore.TakePending()) return;

        m_Log.Message("[MatchConfigService] INFO: No configuration is pending, the next start is the match of MCC.");
    }

    auto MatchConfigService::ArmRecord(std::string_view name) -> bool
    {
        if (!IsValidName(name))
        {
            m_Log.Message("[MatchConfigService] WARNING: \"{}\" is not a valid configuration name.", name);
            return false;
        }

        m_MatchConfigStore.SetRecordName(std::string{ name });

        m_Log.Message("[MatchConfigService] INFO: The next start is recorded as \"{}\".", name);
        return true;
    }

    auto MatchConfigService::RequestLaunch() -> void
    {
        m_MatchConfigStore.SetLaunchRequested(true);

        m_Log.Message("[MatchConfigService] INFO: Launch of the lobby requested.");
    }

    auto MatchConfigService::CancelLaunch() -> void
    {
        if (!m_MatchConfigStore.TakeLaunchRequest()) return;

        m_Log.Message("[MatchConfigService] INFO: Launch of the lobby withdrawn.");
    }

    auto MatchConfigService::CancelRecord() -> void
    {
        if (m_MatchConfigStore.TakeRecordName().empty()) return;

        m_Log.Message("[MatchConfigService] INFO: The next start is not recorded.");
    }

    auto MatchConfigService::Record(const std::byte* configuration) -> void
    {
        const std::string name{ m_MatchConfigStore.TakeRecordName() };
        if (name.empty()) return;

        const path folder{ this->GetLibraryFolder() };

        if (folder.empty())
        {
            m_Log.Message("[MatchConfigService] WARNING: Storage Folder is not set, the start is not recorded.");
            return;
        }

        std::error_code error{};
        std::filesystem::create_directories(folder, error);

        const path file{ folder / std::format("{}{}", name, k_ConfigExtension) };
        std::ofstream stream{ file, std::ios::binary | std::ios::trunc };

        if (!stream)
        {
            m_Log.Message("[MatchConfigService] ERROR: Failed to open {}.", ToText(file));
            return;
        }

        if (!stream.write(reinterpret_cast<const char*>(configuration), static_cast<std::streamsize>(k_EngineConfigurationSize)))
        {
            m_Log.Message("[MatchConfigService] ERROR: Failed to write {}.", ToText(file));
            return;
        }

        m_MatchConfigStore.SetLastRecorded(ToText(file));

        m_Log.Message("[MatchConfigService] INFO: Start recorded to {} (base map 0x{:X}).", ToText(file),
            ReadBaseMap(configuration));
    }

    auto MatchConfigService::Transplant(const std::byte* configuration, std::vector<std::byte>& start) -> bool
    {
        const std::shared_ptr<const MatchConfig> pending{ m_MatchConfigStore.TakePending() };
        if (!pending) return false;

        if (pending->Bytes.size() != k_EngineConfigurationSize)
        {
            m_Log.Message("[MatchConfigService] ERROR: Configuration \"{}\" has {} bytes, the start is not changed.",
                pending->Name, pending->Bytes.size());
            return false;
        }

        start.assign(configuration, configuration + k_EngineConfigurationSize);

        for (const auto& block : k_TransplantBlocks)
        {
            std::memcpy(start.data() + block.Offset, pending->Bytes.data() + block.Offset, block.Size);
        }

        m_MatchConfigStore.SetLastApplied(pending->Name);

        m_Log.Message("[MatchConfigService] INFO: Configuration \"{}\" transplanted into the start, base map 0x{:X} of MCC"
            " replaced by 0x{:X}.", pending->Name, ReadBaseMap(configuration), ReadBaseMap(start.data()));
        return true;
    }

    auto MatchConfigService::GetLibraryFolder() const -> path
    {
        const std::string directory{ m_SettingsStore.GetAppDataDirectory() };
        if (directory.empty()) return path{};

        return path{ directory } / k_LibraryFolder;
    }
}