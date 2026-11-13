#include <gtest/gtest.h>
#include <process.h>

import Common.Log.System;
import Common.Log.Type;
import Services.Settings.State;
import Engine.Match.Type;
import Engine.Match.State;
import Engine.Match.System;
import Blam.Layout.Object.Type;
import Tests.Support.Log;
import std;

// MatchConfigService and MatchConfigStore (M6c.2, D221): the recorded match configurations of the lobby (D123). The service works on a
// temporary storage folder, and the buffer of MCC is built by the test. The tests fix what the code does today.

namespace
{
    using Common::Log::System::Log;
    using Common::Log::Type::LogLevel;
    using Common::Log::Type::LogMessage;
    using Engine::Match::State::MatchConfigStore;
    using Engine::Match::System::MatchConfigService;
    using Services::Settings::State::SettingsStore;
    using Tests::Support::Log::CollectingSink;

    using Blam::Layout::Object::Type::Constant::k_ConfigurationMapVariantOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantSize;

    using Engine::Match::Type::k_ConfigurationBaseMapOffset;
    using Engine::Match::Type::k_EngineConfigurationSize;
    using Engine::Match::Type::k_TransplantBlocks;

    using std::filesystem::path;

    class MatchConfigTests : public testing::Test
    {
    protected:
        CollectingSink m_Sink{};
        Log m_Log{ m_Sink };
        SettingsStore m_SettingsStore{};
        MatchConfigStore m_Store{};
        MatchConfigService m_Service{ m_Log, m_SettingsStore, m_Store };
        path m_Folder{};

        auto SetUp() -> void override
        {
            const std::string test{ testing::UnitTest::GetInstance()->current_test_info()->name() };
            m_Folder = std::filesystem::temp_directory_path() / std::format("artemis_tests_{}_{}", _getpid(), test);

            std::error_code error{};
            std::filesystem::remove_all(m_Folder, error);
            std::filesystem::create_directories(m_Folder, error);

            m_SettingsStore.SetAppDataDirectory(m_Folder.string());
        }

        auto TearDown() -> void override
        {
            std::error_code error{};
            std::filesystem::remove_all(m_Folder, error);
        }

        auto Library() const -> path
        {
            return m_Folder / "MatchConfigs";
        }

        // return: A buffer of MCC with every byte set to fill.
        static auto MakeBuffer(std::uint8_t fill) -> std::vector<std::byte>
        {
            return std::vector<std::byte>(k_EngineConfigurationSize, static_cast<std::byte>(fill));
        }

        static auto SetBaseMap(std::vector<std::byte>& buffer, std::uint32_t id) -> void
        {
            std::memcpy(buffer.data() + k_ConfigurationBaseMapOffset, &id, sizeof(id));
        }

        // Writes a file of the library with the given bytes.
        auto WriteConfig(const std::string& fileName, const std::vector<std::byte>& bytes) const -> void
        {
            std::filesystem::create_directories(this->Library());
            std::ofstream stream{ this->Library() / fileName, std::ios::binary | std::ios::trunc };
            stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        }

        static auto ReadFile(const path& file) -> std::vector<std::byte>
        {
            std::ifstream stream{ file, std::ios::binary };
            std::vector<char> text{ std::istreambuf_iterator<char>{ stream }, std::istreambuf_iterator<char>{} };
            return std::vector<std::byte>{ reinterpret_cast<const std::byte*>(text.data()),
                reinterpret_cast<const std::byte*>(text.data()) + text.size() };
        }

        // return: Whether some message of the level has the text in it.
        auto Logged(LogLevel level, std::string_view text) const -> bool
        {
            return std::ranges::any_of(m_Sink.m_Messages, [&](const LogMessage& message) {
                return message.Level == level && message.Message.find(text) != std::string::npos;
                });
        }

        auto FileCount() const -> std::size_t
        {
            std::error_code error{};
            if (!std::filesystem::exists(this->Library(), error)) return 0;

            return static_cast<std::size_t>(std::distance(std::filesystem::directory_iterator{ this->Library() },
                std::filesystem::directory_iterator{}));
        }
    };
}

TEST_F(MatchConfigTests, RecordWritesTheArmedName)
{
    auto buffer{ MakeBuffer(0x55) };
    SetBaseMap(buffer, 0x1234);

    EXPECT_TRUE(m_Service.ArmRecord("ctf 1v1"));
    m_Service.Record(buffer.data());

    const path file{ this->Library() / "ctf 1v1.bin" };
    ASSERT_TRUE(std::filesystem::exists(file));
    EXPECT_EQ(ReadFile(file), buffer);

    std::error_code error{};
    EXPECT_TRUE(std::filesystem::equivalent(path{ m_Store.GetLastRecorded() }, file, error));
    EXPECT_TRUE(Logged(LogLevel::Info, "base map 0x1234"));

    // The armed name is taken once: a second Record writes nothing.
    auto other{ MakeBuffer(0x77) };
    m_Service.Record(other.data());

    EXPECT_EQ(ReadFile(file), buffer);
    EXPECT_EQ(this->FileCount(), 1u);
}

TEST_F(MatchConfigTests, NoStorageFolderWritesNothing)
{
    m_SettingsStore.SetAppDataDirectory("");
    const auto buffer{ MakeBuffer(0x55) };

    EXPECT_TRUE(m_Service.ArmRecord("ctf"));
    m_Service.Record(buffer.data());

    EXPECT_FALSE(std::filesystem::exists(this->Library()));
    EXPECT_TRUE(Logged(LogLevel::Warning, "Storage Folder is not set"));
    EXPECT_FALSE(m_Service.LoadPending("ctf"));
    EXPECT_TRUE(m_Service.ListConfigs().empty());
}

TEST_F(MatchConfigTests, ListConfigsIsSortedAndOnlyBin)
{
    const auto bytes{ MakeBuffer(0x11) };
    this->WriteConfig("b.bin", bytes);
    this->WriteConfig("a.bin", bytes);
    this->WriteConfig("c.txt", bytes);
    std::filesystem::create_directories(this->Library() / "d.bin");

    EXPECT_EQ(m_Service.ListConfigs(), (std::vector<std::string>{ "a", "b" }));
}

TEST_F(MatchConfigTests, LoadPendingChecksTheSize)
{
    this->WriteConfig("short.bin", std::vector<std::byte>(10, std::byte{ 0x01 }));

    EXPECT_FALSE(m_Service.LoadPending("short"));
    EXPECT_TRUE(Logged(LogLevel::Error, "has 10 bytes"));
    EXPECT_EQ(m_Store.GetPending(), nullptr);

    auto full{ MakeBuffer(0x42) };
    SetBaseMap(full, 0x99);
    this->WriteConfig("full.bin", full);

    EXPECT_TRUE(m_Service.LoadPending("full"));

    const auto pending{ m_Store.GetPending() };
    ASSERT_NE(pending, nullptr);
    EXPECT_EQ(pending->Name, "full");
    EXPECT_EQ(pending->Bytes, full);
}

TEST_F(MatchConfigTests, TransplantCopiesOnlyTheBlocks)
{
    // Without a pending configuration nothing changes.
    const auto mcc{ MakeBuffer(0x55) };
    std::vector<std::byte> start{ std::byte{ 0x01 } };

    EXPECT_FALSE(m_Service.Transplant(mcc.data(), start));
    EXPECT_EQ(start, std::vector<std::byte>{ std::byte{ 0x01 } });

    this->WriteConfig("all.bin", MakeBuffer(0xAA));
    ASSERT_TRUE(m_Service.LoadPending("all"));

    EXPECT_TRUE(m_Service.Transplant(mcc.data(), start));
    ASSERT_EQ(start.size(), k_EngineConfigurationSize);

    std::vector<bool> isInBlock(k_EngineConfigurationSize, false);
    for (const auto& block : k_TransplantBlocks)
    {
        for (std::size_t i{ block.Offset }; i < block.Offset + block.Size; i++) isInBlock[i] = true;
    }

    std::size_t wrong{ 0 };
    for (std::size_t i{ 0 }; i < start.size(); i++)
    {
        const std::byte expected{ isInBlock[i] ? std::byte{ 0xAA } : std::byte{ 0x55 } };
        if (start[i] != expected) wrong++;
    }

    EXPECT_EQ(wrong, 0u);
    EXPECT_EQ(m_Store.GetLastApplied(), "all");

    // The pending configuration is taken once.
    EXPECT_FALSE(m_Service.Transplant(mcc.data(), start));
}

TEST_F(MatchConfigTests, InvalidNamesAreRejected)
{
    const std::vector<std::string> invalid{ "", std::string(65, 'a'), "a/b", "a:b", "name.", "name ", "CON", "con.txt", "LPT1" };

    for (const std::string& name : invalid)
    {
        const std::size_t before{ m_Sink.CountOf(LogLevel::Warning) };

        EXPECT_FALSE(m_Service.ArmRecord(name)) << name;
        EXPECT_FALSE(m_Service.LoadPending(name)) << name;
        EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), before + 2) << name;
    }

    EXPECT_TRUE(Logged(LogLevel::Warning, "is not a valid configuration name"));

    for (const std::string& name : std::vector<std::string>{ "Ctf_1v1-v2.final", "CONSOLE", std::string(64, 'a') })
    {
        EXPECT_TRUE(m_Service.ArmRecord(name)) << name;
    }
}

TEST_F(MatchConfigTests, ClearAndCancel)
{
    this->WriteConfig("one.bin", MakeBuffer(0x01));
    ASSERT_TRUE(m_Service.LoadPending("one"));
    ASSERT_NE(m_Store.GetPending(), nullptr);

    m_Service.ClearPending();
    EXPECT_EQ(m_Store.GetPending(), nullptr);

    m_Service.RequestLaunch();
    EXPECT_TRUE(m_Store.IsLaunchRequested());
    m_Service.CancelLaunch();
    EXPECT_FALSE(m_Store.IsLaunchRequested());

    EXPECT_TRUE(m_Service.ArmRecord("armed"));
    EXPECT_EQ(m_Store.GetRecordName(), "armed");
    m_Service.CancelRecord();
    EXPECT_TRUE(m_Store.GetRecordName().empty());
}

TEST_F(MatchConfigTests, ReadMapVariantGivesTheBlockOfTheConfig)
{
    auto buffer{ MakeBuffer(0x11) };
    std::memset(buffer.data() + k_ConfigurationMapVariantOffset, 0x5C, k_MapVariantSize);
    this->WriteConfig("reach bus.bin", buffer);

    const std::vector<std::byte> block{ m_Service.ReadMapVariant("reach bus") };

    ASSERT_EQ(block.size(), k_MapVariantSize);
    EXPECT_TRUE(std::ranges::all_of(block, [](std::byte value) { return value == std::byte{ 0x5C }; }));

    // Reading a map variant leaves the pending configuration as it was.
    EXPECT_EQ(m_Store.GetPending(), nullptr);
}

TEST_F(MatchConfigTests, ReadMapVariantReturnsTheBytesAtTheOffsetOnly)
{
    auto buffer{ MakeBuffer(0x00) };
    buffer[k_ConfigurationMapVariantOffset - 1] = std::byte{ 0xEE };
    buffer[k_ConfigurationMapVariantOffset] = std::byte{ 0x01 };
    buffer[k_ConfigurationMapVariantOffset + k_MapVariantSize - 1] = std::byte{ 0x02 };
    buffer[k_ConfigurationMapVariantOffset + k_MapVariantSize] = std::byte{ 0xEE };
    this->WriteConfig("edges.bin", buffer);

    const std::vector<std::byte> block{ m_Service.ReadMapVariant("edges") };

    ASSERT_EQ(block.size(), k_MapVariantSize);
    EXPECT_EQ(block.front(), std::byte{ 0x01 });
    EXPECT_EQ(block.back(), std::byte{ 0x02 });
}

TEST_F(MatchConfigTests, ReadMapVariantFailsWithoutReadingAWrongFile)
{
    this->WriteConfig("short.bin", std::vector<std::byte>(k_EngineConfigurationSize - 1, std::byte{ 0 }));

    EXPECT_TRUE(m_Service.ReadMapVariant("short").empty());
    EXPECT_TRUE(Logged(LogLevel::Error, "a configuration has"));
    EXPECT_TRUE(m_Service.ReadMapVariant("missing").empty());
    EXPECT_TRUE(m_Service.ReadMapVariant("bad/name").empty());
    EXPECT_TRUE(Logged(LogLevel::Warning, "is not a valid configuration name"));
}

TEST_F(MatchConfigTests, GetNameProblemSaysWhyAndIsEmptyForAValidName)
{
    EXPECT_TRUE(MatchConfigService::GetNameProblem("ctf_1").empty());
    EXPECT_FALSE(MatchConfigService::GetNameProblem("").empty());
    EXPECT_FALSE(MatchConfigService::GetNameProblem("a/b").empty());
    EXPECT_FALSE(MatchConfigService::GetNameProblem("NUL").empty());
    EXPECT_FALSE(MatchConfigService::GetNameProblem("name.").empty());
    EXPECT_FALSE(MatchConfigService::GetNameProblem(std::string(100, 'a')).empty());
}

TEST_F(MatchConfigTests, RenameMovesTheFileAndTheListingFollows)
{
    this->WriteConfig("old.bin", MakeBuffer(0x11));

    EXPECT_TRUE(m_Service.RenameConfig("old", "new").empty());

    EXPECT_EQ(m_Service.ListConfigs(), std::vector<std::string>{ "new" });
    EXPECT_EQ(ReadFile(this->Library() / "new.bin"), MakeBuffer(0x11));
    EXPECT_FALSE(std::filesystem::exists(this->Library() / "old.bin"));
}

TEST_F(MatchConfigTests, RenameRejectsAnInvalidOrExistingNameAndAMissingSource)
{
    this->WriteConfig("a.bin", MakeBuffer(0x11));
    this->WriteConfig("b.bin", MakeBuffer(0x22));

    EXPECT_FALSE(m_Service.RenameConfig("a", "bad/name").empty());
    EXPECT_FALSE(m_Service.RenameConfig("a", "CON").empty());
    EXPECT_FALSE(m_Service.RenameConfig("a", "b").empty());
    EXPECT_FALSE(m_Service.RenameConfig("missing", "c").empty());

    EXPECT_EQ(ReadFile(this->Library() / "a.bin"), MakeBuffer(0x11));
    EXPECT_EQ(ReadFile(this->Library() / "b.bin"), MakeBuffer(0x22));
    EXPECT_EQ(m_Service.ListConfigs(), (std::vector<std::string>{ "a", "b" }));
}

TEST_F(MatchConfigTests, RenameAndDeleteRefuseThePendingConfiguration)
{
    this->WriteConfig("p.bin", MakeBuffer(0x33));
    ASSERT_TRUE(m_Service.LoadPending("p"));

    EXPECT_FALSE(m_Service.RenameConfig("p", "q").empty());
    EXPECT_FALSE(m_Service.DeleteConfig("p").empty());
    EXPECT_TRUE(std::filesystem::exists(this->Library() / "p.bin"));
    ASSERT_NE(m_Store.GetPending(), nullptr);
    EXPECT_EQ(m_Store.GetPending()->Name, "p");

    m_Service.ClearPending();
    EXPECT_TRUE(m_Service.DeleteConfig("p").empty());
}

TEST_F(MatchConfigTests, DeleteMovesTheFileToDeletedAndItIsNoLongerListed)
{
    this->WriteConfig("gone.bin", MakeBuffer(0x44));
    this->WriteConfig("stay.bin", MakeBuffer(0x55));

    EXPECT_TRUE(m_Service.DeleteConfig("gone").empty());

    EXPECT_EQ(m_Service.ListConfigs(), std::vector<std::string>{ "stay" });
    EXPECT_FALSE(m_Service.LoadPending("gone"));
    EXPECT_EQ(ReadFile(this->Library() / "_deleted" / "gone.bin"), MakeBuffer(0x44));
    EXPECT_FALSE(m_Service.DeleteConfig("gone").empty());
}

TEST_F(MatchConfigTests, DeleteKeepsAnEarlierDeletedFileWithTheSameName)
{
    this->WriteConfig("x.bin", MakeBuffer(0x01));
    ASSERT_TRUE(m_Service.DeleteConfig("x").empty());
    this->WriteConfig("x.bin", MakeBuffer(0x02));
    ASSERT_TRUE(m_Service.DeleteConfig("x").empty());

    EXPECT_EQ(ReadFile(this->Library() / "_deleted" / "x.bin"), MakeBuffer(0x01));
    EXPECT_EQ(ReadFile(this->Library() / "_deleted" / "x (2).bin"), MakeBuffer(0x02));
    EXPECT_TRUE(m_Service.ListConfigs().empty());
}

namespace
{
    using Blam::Layout::Object::Type::Constant::k_ForgeWorldMapId;
    using Blam::Layout::Object::Type::Constant::k_MapVariantMapIdOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantVersion;
    using Blam::Layout::Object::Type::Constant::k_MapVariantVersionOffset;

    // return: A map variant block with the version and the map id, every other byte set to fill.
    auto MakeMapVariant(std::uint16_t version, std::uint32_t mapId, std::uint8_t fill) -> std::vector<std::byte>
    {
        std::vector<std::byte> block(k_MapVariantSize, static_cast<std::byte>(fill));
        std::memcpy(block.data() + k_MapVariantVersionOffset, &version, sizeof(version));
        std::memcpy(block.data() + k_MapVariantMapIdOffset, &mapId, sizeof(mapId));
        return block;
    }
}

// CreateFromMvar (Q106, D290): a configuration made from a template and the map variant of a .mvar.
TEST_F(MatchConfigTests, CreateFromMvarReplacesOnlyTheMapVariantOfTheTemplate)
{
    std::vector<std::byte> templateBytes{ MakeBuffer(0x11) };
    const std::vector<std::byte> original{ MakeMapVariant(k_MapVariantVersion, k_ForgeWorldMapId, 0x22) };
    std::ranges::copy(original, templateBytes.begin() + k_ConfigurationMapVariantOffset);
    this->WriteConfig("Template.bin", templateBytes);

    const std::vector<std::byte> incoming{ MakeMapVariant(k_MapVariantVersion, k_ForgeWorldMapId, 0x33) };

    EXPECT_EQ(m_Service.CreateFromMvar("Template", incoming, "Map New"), "");

    const std::vector<std::byte> created{ ReadFile(this->Library() / "Map New.bin") };
    ASSERT_EQ(created.size(), k_EngineConfigurationSize);

    std::vector<std::byte> expected{ templateBytes };
    std::ranges::copy(incoming, expected.begin() + k_ConfigurationMapVariantOffset);

    EXPECT_EQ(created, expected);
    EXPECT_EQ(ReadFile(this->Library() / "Template.bin"), templateBytes);
    EXPECT_EQ(m_Service.ReadConfigBytes("Map New"), expected);
    EXPECT_EQ(m_Service.ListConfigs(), (std::vector<std::string>{ "Map New", "Template" }));
}

TEST_F(MatchConfigTests, CreateFromMvarRefusesAnotherMapIdWithTheReason)
{
    std::vector<std::byte> templateBytes{ MakeBuffer(0x11) };
    std::ranges::copy(MakeMapVariant(k_MapVariantVersion, k_ForgeWorldMapId, 0x22), templateBytes.begin() + k_ConfigurationMapVariantOffset);
    this->WriteConfig("Template.bin", templateBytes);

    const std::string reason{ m_Service.CreateFromMvar("Template", MakeMapVariant(k_MapVariantVersion, 3010, 0x33), "Map Other") };

    EXPECT_NE(reason.find("map 3010"), std::string::npos);
    EXPECT_NE(reason.find("Forge World"), std::string::npos);
    EXPECT_EQ(this->FileCount(), 1u);
}

TEST_F(MatchConfigTests, CreateFromMvarRefusesATemplateOfAnotherMap)
{
    std::vector<std::byte> templateBytes{ MakeBuffer(0x11) };
    std::ranges::copy(MakeMapVariant(k_MapVariantVersion, 3010, 0x22), templateBytes.begin() + k_ConfigurationMapVariantOffset);
    this->WriteConfig("Template.bin", templateBytes);

    const std::string reason{ m_Service.CreateFromMvar("Template", MakeMapVariant(k_MapVariantVersion, k_ForgeWorldMapId, 0x33), "Map New") };

    EXPECT_NE(reason.find("not a Forge World configuration"), std::string::npos);
    EXPECT_EQ(this->FileCount(), 1u);
}

TEST_F(MatchConfigTests, CreateFromMvarNeverOverwritesAndChecksTheName)
{
    std::vector<std::byte> templateBytes{ MakeBuffer(0x11) };
    std::ranges::copy(MakeMapVariant(k_MapVariantVersion, k_ForgeWorldMapId, 0x22), templateBytes.begin() + k_ConfigurationMapVariantOffset);
    this->WriteConfig("Template.bin", templateBytes);
    this->WriteConfig("Taken.bin", MakeBuffer(0x44));

    const std::vector<std::byte> incoming{ MakeMapVariant(k_MapVariantVersion, k_ForgeWorldMapId, 0x33) };

    EXPECT_EQ(m_Service.CreateFromMvar("Template", incoming, "Taken"), "A configuration with that name already exists.");
    EXPECT_EQ(ReadFile(this->Library() / "Taken.bin"), MakeBuffer(0x44));
    EXPECT_FALSE(m_Service.CreateFromMvar("Template", incoming, "bad/name").empty());
    EXPECT_FALSE(m_Service.CreateFromMvar("Template", incoming, "").empty());
    EXPECT_FALSE(m_Service.CreateFromMvar("Missing", incoming, "Map New").empty());
    EXPECT_FALSE(m_Service.CreateFromMvar("Template", std::vector<std::byte>(10), "Map New").empty());
    EXPECT_FALSE(m_Service.CreateFromMvar("Template", MakeMapVariant(0x1, k_ForgeWorldMapId, 0x33), "Map New").empty());
    EXPECT_EQ(this->FileCount(), 2u);
}

TEST(MatchConfigNameTests, SuggestNameDropsWhatTheRuleDoesNotAllow)
{
    EXPECT_EQ(MatchConfigService::SuggestName("Map CTF (1v1) First"), "Map CTF 1v1 First");
    EXPECT_EQ(MatchConfigService::SuggestName("Map CTF: 2v2 (final iteration)"), "Map CTF 2v2 final iteration");
    EXPECT_EQ(MatchConfigService::SuggestName("Map   spaced  out  "), "Map spaced out");
    EXPECT_EQ(MatchConfigService::SuggestName("Map ends with dot."), "Map ends with dot");
    EXPECT_EQ(MatchConfigService::SuggestName("(((" ), "");
    EXPECT_TRUE(MatchConfigService::GetNameProblem(MatchConfigService::SuggestName("Map CTF (1v1) First")).empty());
    EXPECT_EQ(MatchConfigService::SuggestName(std::string(200, 'a')).size(), Engine::Match::Type::k_MaxMatchConfigNameLength);
}