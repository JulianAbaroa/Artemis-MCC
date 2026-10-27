module Platform.Match.System;
import :MatchConfig;

import Platform.Match.Type;
import std;

namespace
{
    using std::filesystem::path;
    using Platform::Match::Type::MatchConfig;

    using Platform::Match::Type::k_EngineConfigurationSize;
    using Platform::Match::Type::k_ConfigurationBaseMapOffset;
    using Platform::Match::Type::k_TransplantBlocks;
    using Platform::Match::Type::k_MaxMatchConfigNameLength;

    // Folder inside the storage folder that holds the recorded configurations, and the extension of their files.
    constexpr std::string_view k_LibraryFolder{ "MatchConfigs" };
    constexpr std::string_view k_ConfigExtension{ ".bin" };

    // Names Windows opens as devices instead of files, whatever the extension.
    constexpr std::array<std::string_view, 22> k_DeviceNames{
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9",
    };

    // return: Whether the name can be the file name of a configuration: letters, digits, spaces, '_', '-' and '.'.
    // note: Windows drops a final space or dot from a file name, so the name cannot end with one. Before the first dot the name cannot
    // be a device name.
    auto IsValidName(std::string_view name) -> bool
    {
        if (name.empty() || name.size() > k_MaxMatchConfigNameLength) return false;
        if (name.back() == ' ' || name.back() == '.') return false;

        const bool isAllowed{ std::ranges::all_of(name, [](char c) {
            return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == ' ' || c == '_' || c == '-' || c == '.';
            }) };

        if (!isAllowed) return false;

        const std::string_view stem{ name.substr(0, name.find('.')) };

        return std::ranges::none_of(k_DeviceNames, [stem](std::string_view device) {
            return std::ranges::equal(stem, device, [](char a, char b) {
                return std::toupper(static_cast<unsigned char>(a)) == std::toupper(static_cast<unsigned char>(b));
                });
            });
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

namespace Platform::Match::System
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
        if (!IsValidName(name))
        {
            m_Log.Message("[MatchConfigService] WARNING: \"{}\" is not a valid configuration name.", name);
            return false;
        }

        const path folder{ this->GetLibraryFolder() };

        if (folder.empty())
        {
            m_Log.Message("[MatchConfigService] WARNING: Storage Folder is not set, no configuration can be read.");
            return false;
        }

        const path file{ folder / std::format("{}{}", name, k_ConfigExtension) };
        std::ifstream stream{ file, std::ios::binary | std::ios::ate };

        if (!stream)
        {
            m_Log.Message("[MatchConfigService] ERROR: Failed to open {}.", ToText(file));
            return false;
        }

        const std::streamoff size{ stream.tellg() };

        if (size != static_cast<std::streamoff>(k_EngineConfigurationSize))
        {
            m_Log.Message("[MatchConfigService] ERROR: {} has {} bytes, a configuration has {}.", ToText(file),
                static_cast<long long>(size), k_EngineConfigurationSize);
            return false;
        }

        auto config{ std::make_shared<MatchConfig>() };
        config->Name = std::string{ name };
        config->Bytes.resize(k_EngineConfigurationSize);

        stream.seekg(0);

        if (!stream.read(reinterpret_cast<char*>(config->Bytes.data()), static_cast<std::streamsize>(k_EngineConfigurationSize)))
        {
            m_Log.Message("[MatchConfigService] ERROR: Failed to read {}.", ToText(file));
            return false;
        }

        const std::uint32_t baseMap{ ReadBaseMap(config->Bytes.data()) };
        m_MatchConfigStore.SetPending(std::move(config));

        m_Log.Message("[MatchConfigService] INFO: Configuration \"{}\" (base map 0x{:X}) is pending for the next start.",
            name, baseMap);
        return true;
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