module;

#include <Windows.h>
#include <ShlObj.h>
#include <KnownFolders.h>

module Engine.Match.System;
import :MapVariantCatalog;

import Common.Text.System;
import Blam.Layout.MapVariantFile.Type;
import std;

namespace
{
    namespace FileParse = Blam::Layout::MapVariantFile::Type::Parse;

    using Common::Text::System::WideToUtf8;
    using FileParse::k_CatalogBytes;

    // Folders under the account folder that hold the map variants of Reach.
    constexpr std::string_view k_GameFolder{ "HaloReach" };
    constexpr std::string_view k_MapFolder{ "Map" };
    constexpr std::string_view k_Extension{ ".mvar" };

    // return: The LocalFiles folder of MCC, empty if Windows does not give LocalAppDataLow.
    auto FindLocalFiles() -> std::filesystem::path
    {
        PWSTR folder{ nullptr };
        std::filesystem::path result{};

        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppDataLow, 0, nullptr, &folder)) && folder != nullptr)
        {
            result = std::filesystem::path{ folder } / "MCC" / "LocalFiles";
        }

        CoTaskMemFree(folder);
        return result;
    }

    // return: The first k_CatalogBytes of a file, fewer if it is shorter, empty if it cannot be opened.
    auto ReadStart(const std::filesystem::path& file) -> std::vector<std::byte>
    {
        std::ifstream stream{ file, std::ios::binary };
        std::vector<std::byte> bytes(k_CatalogBytes);

        if (!stream) return {};

        stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        bytes.resize(static_cast<std::size_t>(stream.gcount()));
        return bytes;
    }
}

namespace Engine::Match::System
{
    auto MapVariantCatalogService::List() const -> std::vector<MapVariantEntry>
    {
        const path localFiles{ FindLocalFiles() };

        return localFiles.empty() ? std::vector<MapVariantEntry>{} : this->ListIn(localFiles);
    }

    auto MapVariantCatalogService::ListIn(const path& localFiles) const -> std::vector<MapVariantEntry>
    {
        std::vector<MapVariantEntry> entries{};
        std::error_code error{};

        for (const auto& account : std::filesystem::directory_iterator(localFiles, error))
        {
            const path folder{ account.path() / k_GameFolder / k_MapFolder };

            for (const auto& file : std::filesystem::directory_iterator(folder, error))
            {
                if (!file.is_regular_file(error) || file.path().extension() != k_Extension) continue;

                const std::vector<std::byte> start{ ReadStart(file.path()) };
                const FileParse::MapVariantFileResult parsed{ FileParse::Read(start) };

                if (!parsed.IsOk())
                {
                    m_Log.Message("[MapVariantCatalogService] WARNING: Skipped {} ({}).", file.path().filename().string(), parsed.Error);
                    continue;
                }

                MapVariantEntry entry{};
                entry.Path = file.path();
                entry.Name = parsed.File.Name.empty() ? file.path().filename().string() : WideToUtf8(parsed.File.Name);
                entry.Description = WideToUtf8(parsed.File.Description);
                entry.Account = account.path().filename().string();
                entry.Modified = std::chrono::clock_cast<std::chrono::system_clock>(file.last_write_time(error));
                entries.push_back(std::move(entry));
            }
        }

        std::ranges::sort(entries, [](const MapVariantEntry& first, const MapVariantEntry& second)
        {
            return std::tie(first.Name, first.Path) < std::tie(second.Name, second.Path);
        });

        return entries;
    }
}