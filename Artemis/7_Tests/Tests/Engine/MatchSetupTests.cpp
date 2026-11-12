#include <gtest/gtest.h>
#include <process.h>

import Common.Log.System;
import Common.Log.Type;
import Services.Settings.State;
import Engine.Base.Lifecycle.State;
import Engine.Base.Lifecycle.Type;
import Engine.Base.MapVariant.State;
import Engine.Base.MapVariant.System;
import Engine.Base.MapVariant.Type;
import Engine.Base.MatchSetup.State;
import Engine.Base.MatchSetup.System;
import Engine.Base.MatchSetup.Type;
import Engine.Base.Preset.State;
import Engine.Base.Preset.System;
import Engine.Base.Preset.Type;
import Engine.Match.Type;
import Blam.Layout.Game.Type;
import Blam.Layout.Object.Type;
import Tests.Support.Log;
import std;

// MatchSetupStore and MatchSetupService (LS.2, D286): the preset, the map variant and the bots of a match in one request. The store is
// data and the service writes the start configuration whole or not at all. The part of the service that writes the live memory of a
// match (the round break) runs in the game thread and is checked in the game; here the request, the start and the store.

namespace
{
    using Common::Log::System::Log;
    using Common::Log::Type::LogLevel;
    using Common::Log::Type::LogMessage;
    using Engine::Base::Lifecycle::State::LifecycleStore;
    using Engine::Base::Lifecycle::Type::Status;
    using Engine::Base::MapVariant::State::MapVariantStore;
    using Engine::Base::MapVariant::System::MapVariantService;
    using Engine::Base::MapVariant::Type::ActiveMapVariant;
    using Engine::Base::MatchSetup::State::MatchSetupStore;
    using Engine::Base::MatchSetup::System::MatchSetupService;
    using Engine::Base::MatchSetup::System::RosterPort;
    using Engine::Base::MatchSetup::Type::ActiveMatchSetup;
    using Engine::Base::MatchSetup::Type::CheckRosterSize;
    using Engine::Base::MatchSetup::Type::MatchSetup;
    using Engine::Base::MatchSetup::Type::MatchSetupLoad;
    using Engine::Base::MatchSetup::Type::MatchSetupOrigin;
    using Engine::Base::MatchSetup::Type::MatchSetupOutcome;
    using Engine::Base::MatchSetup::Type::MatchSetupResult;
    using Engine::Base::MatchSetup::Type::Roster;
    using Engine::Base::MatchSetup::Type::RosterApplication;
    using Engine::Base::MatchSetup::Type::ToText;
    using Engine::Base::Preset::State::PresetStore;
    using Engine::Base::Preset::System::PresetService;
    using Engine::Base::Preset::Type::PresetPick;
    using Engine::Base::Preset::Type::k_ConfigurationVariantOffset;
    using Engine::Match::Type::k_EngineConfigurationSize;
    using Services::Settings::State::SettingsStore;
    using Tests::Support::Log::CollectingSink;
    using Blam::Layout::Game::Type::Offset::k_GameEngineRoundIndex;
    using Blam::Layout::Game::Type::Offset::k_GameEngineRoundState;
    using Blam::Layout::Game::Type::Offset::k_GameGlobalsGameVariant;
    using Blam::Layout::Game::Type::Offset::k_RoundStateBetween;
    using Blam::Layout::Game::Type::Offset::k_RoundStateEnded;
    using Blam::Layout::Game::Type::Offset::k_VariantEngineType;
    using Blam::Layout::Game::Type::Offset::k_VariantEngineTypeMegalo;
    using Blam::Layout::Game::Type::Offset::k_VariantMegaloTraitSetCount;
    using Blam::Layout::Game::Type::Offset::k_VariantRespawnTime;
    using Blam::Layout::Object::Type::Constant::k_ConfigurationMapVariantOffset;
    using Blam::Layout::Object::Type::Constant::k_GameGlobalsMapVariant;
    using Blam::Layout::Object::Type::Constant::k_GameGlobalsSize;
    using Blam::Layout::Object::Type::Constant::k_GameModeMultiplayer;
    using Blam::Layout::Object::Type::Constant::k_GameModeTypeOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantAdjust;
    using Blam::Layout::Object::Type::Constant::k_MapVariantAlignMask;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextGameEngineOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextGameModeOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextMapVariantOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantMapIdOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantQuotasOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantLabelCountOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantSize;
    using Blam::Layout::Object::Type::Constant::k_MapVariantVersion;
    using Blam::Layout::Object::Type::Constant::k_MapVariantVersionOffset;

    using std::filesystem::path;

    using Buffer = std::vector<std::byte>;

    auto MakeRoster(std::size_t red, std::size_t blue) -> Roster
    {
        Roster roster{};
        roster.Teams[0] = red;
        roster.Teams[1] = blue;
        return roster;
    }

    constexpr std::size_t k_VariantHeadSize{ 0x1000 };

    // The bots service as the setup service sees it: it answers what the test says and records what it was asked.
    class FakeRosterPort : public RosterPort
    {
    public:
        using Roster = Engine::Base::MatchSetup::Type::Roster;
        using RosterApplication = Engine::Base::MatchSetup::Type::RosterApplication;

        std::optional<std::string> m_CheckReason{};
        RosterApplication m_Application{};
        bool m_IsThrowing{ false };
        std::size_t m_Checks{ 0 };
        std::size_t m_Applies{ 0 };
        // Runs when the roster is applied: the test looks at the memory of the match at that moment.
        std::function<void()> m_OnApply{};

        auto CheckRoster(std::uintptr_t threadContext, const Roster& roster) -> std::optional<std::string> override
        {
            static_cast<void>(threadContext);
            static_cast<void>(roster);
            ++m_Checks;
            if (m_IsThrowing) throw std::runtime_error{ "check failed" };
            return m_CheckReason;
        }

        auto ApplyRoster(std::uintptr_t threadContext, const Roster& roster) -> RosterApplication override
        {
            static_cast<void>(threadContext);
            static_cast<void>(roster);
            ++m_Applies;
            if (m_OnApply) m_OnApply();
            return m_Application;
        }
    };

    // The memory of a match that the setup service reads through the thread context: the game engine globals (round), the game globals
    // (mode, game variant and the map variant of the options) and the runtime map variant.
    class FakeGame
    {
    public:
        // param configuration: A start configuration: its variant and map variant are the ones of the match.
        explicit FakeGame(const Buffer& configuration) :
            m_ThreadContext(0x400), m_GameGlobals(k_GameGlobalsSize), m_GameEngine(0x100), m_Runtime(k_MapVariantSize + 0x400)
        {
            // The head of the game variant is what the presets read and write.
            std::memcpy(m_GameGlobals.data() + k_GameGlobalsGameVariant, configuration.data() + k_ConfigurationVariantOffset, k_VariantHeadSize);
            m_GameGlobals[k_GameModeTypeOffset] = static_cast<std::byte>(k_GameModeMultiplayer);
            std::memcpy(m_GameGlobals.data() + k_GameGlobalsMapVariant, configuration.data() + k_ConfigurationMapVariantOffset, k_MapVariantSize);

            const std::uintptr_t base{ reinterpret_cast<std::uintptr_t>(m_Runtime.data()) };
            std::memcpy(this->RuntimeData(), configuration.data() + k_ConfigurationMapVariantOffset, k_MapVariantSize);

            this->SetPointer(k_ThreadContextGameModeOffset, reinterpret_cast<std::uintptr_t>(m_GameGlobals.data()));
            this->SetPointer(k_ThreadContextGameEngineOffset, reinterpret_cast<std::uintptr_t>(m_GameEngine.data()));
            this->SetPointer(k_ThreadContextMapVariantOffset, base);
        }

        auto Context() -> std::uintptr_t { return reinterpret_cast<std::uintptr_t>(m_ThreadContext.data()); }

        auto SetRound(std::uint8_t state, std::int16_t index) -> void
        {
            m_GameEngine[k_GameEngineRoundState] = static_cast<std::byte>(state);
            std::memcpy(m_GameEngine.data() + k_GameEngineRoundIndex, &index, sizeof(index));
        }

        auto Variant() -> std::uint8_t* { return reinterpret_cast<std::uint8_t*>(m_GameGlobals.data() + k_GameGlobalsGameVariant); }
        auto Options() -> std::byte* { return m_GameGlobals.data() + k_GameGlobalsMapVariant; }

        // return: The runtime map variant, where the engine computes it from the pointer of the thread context.
        auto RuntimeData() -> std::byte*
        {
            const std::uintptr_t base{ reinterpret_cast<std::uintptr_t>(m_Runtime.data()) };
            return reinterpret_cast<std::byte*>((base + k_MapVariantAdjust) & ~k_MapVariantAlignMask);
        }

        // return: Everything a setup can write, to compare before and after.
        auto Snapshot() -> Buffer
        {
            Buffer all(m_GameGlobals);
            all.insert(all.end(), this->RuntimeData(), this->RuntimeData() + k_MapVariantSize);
            return all;
        }

    private:
        Buffer m_ThreadContext;
        Buffer m_GameGlobals;
        Buffer m_GameEngine;
        Buffer m_Runtime;

        auto SetPointer(std::uintptr_t offset, std::uintptr_t value) -> void
        {
            std::memcpy(m_ThreadContext.data() + offset, &value, sizeof(value));
        }
    };

    class MatchSetupTests : public testing::Test
    {
    protected:
        CollectingSink m_Sink{};
        Log m_Log{ m_Sink };
        SettingsStore m_SettingsStore{};
        LifecycleStore m_LifecycleStore{};
        PresetStore m_PresetStore{};
        PresetService m_PresetService{ m_Log, m_SettingsStore, m_PresetStore };
        MapVariantStore m_MapVariantStore{};
        MapVariantService m_MapVariantService{ m_Log, m_MapVariantStore };
        MatchSetupStore m_Store{};
        MatchSetupService m_Service{ m_Log, m_LifecycleStore, m_Store, m_PresetService, m_MapVariantService };
        path m_Folder{};

        auto SetUp() -> void override
        {
            const std::string test{ testing::UnitTest::GetInstance()->current_test_info()->name() };
            m_Folder = std::filesystem::temp_directory_path() / std::format("artemis_tests_setup_{}_{}", _getpid(), test);

            std::error_code error{};
            std::filesystem::remove_all(m_Folder, error);
            std::filesystem::create_directories(m_Folder / "Presets", error);

            // The shared fixture of Blam.Layout. This file is Artemis/7_Tests/Tests/Engine/MatchSetupTests.cpp.
            const path source{ std::source_location::current().file_name() };
            const path fixture{ source.parent_path().parent_path() / "Blam" / "Fixtures" / "presets" };
            for (const char* name : { "_catalog.txt", "fixture.txt" }) std::filesystem::copy_file(fixture / name, m_Folder / "Presets" / name, error);

            m_SettingsStore.SetAppDataDirectory(m_Folder.string());
        }

        auto TearDown() -> void override
        {
            std::error_code error{};
            std::filesystem::remove_all(m_Folder, error);
        }

        // return: A start configuration with a Megalo variant of 3 trait sets and a map variant of the given map id.
        static auto MakeConfiguration(std::uint32_t mapId) -> Buffer
        {
            Buffer configuration(k_EngineConfigurationSize);
            for (std::size_t i{}; i < configuration.size(); ++i) configuration[i] = static_cast<std::byte>(0x40 + i % 7);

            const std::uint32_t megalo{ k_VariantEngineTypeMegalo };
            const std::uint32_t sets{ 3 };
            std::memcpy(configuration.data() + k_ConfigurationVariantOffset + k_VariantEngineType, &megalo, sizeof(megalo));
            std::memcpy(configuration.data() + k_ConfigurationVariantOffset + k_VariantMegaloTraitSetCount, &sets, sizeof(sets));

            const std::uint16_t version{ k_MapVariantVersion };
            const std::uint16_t quotas{ 145 };
            std::memcpy(configuration.data() + k_ConfigurationMapVariantOffset + k_MapVariantVersionOffset, &version, sizeof(version));
            std::memcpy(configuration.data() + k_ConfigurationMapVariantOffset + k_MapVariantQuotasOffset, &quotas, sizeof(quotas));
            std::memcpy(configuration.data() + k_ConfigurationMapVariantOffset + k_MapVariantMapIdOffset, &mapId, sizeof(mapId));

            // No labels: the filler would be a label count out of range for the remap of a map variant written between rounds.
            const std::int32_t labels{ 0 };
            std::memcpy(configuration.data() + k_ConfigurationMapVariantOffset + k_MapVariantLabelCountOffset, &labels, sizeof(labels));
            return configuration;
        }

        // return: A map variant that goes over the one of MakeConfiguration(mapId) when the map ids are the same.
        static auto MakeMap(std::string_view name, std::uint32_t mapId, std::byte fill) -> std::shared_ptr<const ActiveMapVariant>
        {
            Buffer configuration{ MakeConfiguration(mapId) };
            Buffer block(configuration.begin() + k_ConfigurationMapVariantOffset, configuration.begin() + k_ConfigurationMapVariantOffset + k_MapVariantSize);
            std::fill(block.begin() + 0x10, block.begin() + 0x20, fill);
            return std::make_shared<const ActiveMapVariant>(ActiveMapVariant{ .Name = std::string{ name }, .Block = std::move(block) });
        }

        auto Pick(std::string_view name) const -> PresetPick
        {
            auto pick{ m_PresetService.Resolve(name) };
            EXPECT_TRUE(pick.has_value()) << name;
            return pick.value_or(PresetPick{});
        }

        // What the start hook does: the per-start state, then the setup if there is something to apply.
        auto Start(Buffer& configuration) -> void
        {
            m_Service.BeginStart(configuration);
            if (m_Service.HasLoadWork()) m_Service.ApplyToStart(configuration);
        }

        auto Logged(LogLevel level, std::string_view text) const -> bool
        {
            return std::ranges::any_of(m_Sink.m_Messages, [&](const LogMessage& message) {
                return message.Level == level && message.Message.find(text) != std::string::npos;
                });
        }
    };
}

TEST(MatchSetupStoreTest, StartsNeutral)
{
    MatchSetupStore store{};

    EXPECT_FALSE(store.HasPending());
    EXPECT_EQ(store.GetResult(), MatchSetupResult{});
    EXPECT_EQ(store.GetSequence(), 0u);
    EXPECT_FALSE(store.GetActiveBots().has_value());
    EXPECT_FALSE(store.IsMatchStarted());
}

TEST(MatchSetupStoreTest, ARequestIsTakenOnceAndItsResultStaysPendingUntilFinished)
{
    MatchSetupStore store{};
    const std::uint64_t sequence{ store.Request(MatchSetup{ .Bots = MakeRoster(1, 2) }, MatchSetupOrigin::Bench) };

    EXPECT_TRUE(store.HasPending());
    EXPECT_EQ(store.GetResult(), (MatchSetupResult{ .Outcome = MatchSetupOutcome::Pending, .Sequence = sequence }));

    const auto taken{ store.TakePending() };

    ASSERT_TRUE(taken.has_value());
    EXPECT_EQ(taken->Sequence, sequence);
    EXPECT_EQ(taken->Value.Bots, MakeRoster(1, 2));
    EXPECT_FALSE(store.HasPending());
    EXPECT_FALSE(store.TakePending().has_value());
    EXPECT_EQ(store.GetResult().Outcome, MatchSetupOutcome::Pending);
    EXPECT_FALSE(store.GetActiveBots().has_value());

    store.Finish(MatchSetupResult{ .Outcome = MatchSetupOutcome::Applied, .Sequence = sequence, .Round = 2 }, taken->Value.Bots);

    EXPECT_EQ(store.GetResult(), (MatchSetupResult{ .Outcome = MatchSetupOutcome::Applied, .Sequence = sequence, .Round = 2 }));
    EXPECT_EQ(store.GetActiveBots(), MakeRoster(1, 2));
}

TEST(MatchSetupStoreTest, TakePendingIfOnlyTakesWhatTheMomentCanServe)
{
    MatchSetupStore store{};
    store.Request(MatchSetup{ .Bots = MakeRoster(1, 1) }, MatchSetupOrigin::Bench);

    EXPECT_FALSE(store.TakePendingIf([](const MatchSetup&) { return false; }).has_value());
    EXPECT_TRUE(store.HasPending());

    EXPECT_TRUE(store.TakePendingIf([](const MatchSetup& setup) { return setup.Bots.has_value(); }).has_value());
    EXPECT_FALSE(store.HasPending());
}

TEST(MatchSetupStoreTest, ANewRequestReplacesThePendingOne)
{
    MatchSetupStore store{};
    const std::uint64_t first{ store.Request(MatchSetup{ .Bots = MakeRoster(1, 0) }, MatchSetupOrigin::Bench) };
    const std::uint64_t second{ store.Request(MatchSetup{ .Bots = MakeRoster(0, 4) }, MatchSetupOrigin::Bench) };

    EXPECT_GT(second, first);
    EXPECT_EQ(store.GetResult().Sequence, second);

    const auto taken{ store.TakePending() };
    ASSERT_TRUE(taken.has_value());
    EXPECT_EQ(taken->Value.Bots, MakeRoster(0, 4));
    EXPECT_FALSE(store.TakePending().has_value());
}

TEST(MatchSetupStoreTest, TheResultOfAnOldRequestNeverReplacesTheOneOfANewerRequest)
{
    MatchSetupStore store{};
    store.Request(MatchSetup{ .Bots = MakeRoster(1, 0) }, MatchSetupOrigin::Bench);
    const auto older{ store.TakePending() };
    ASSERT_TRUE(older.has_value());

    const std::uint64_t newer{ store.Request(MatchSetup{ .Bots = MakeRoster(2, 0) }, MatchSetupOrigin::Bench) };
    store.Finish(MatchSetupResult{ .Outcome = MatchSetupOutcome::Applied, .Sequence = older->Sequence, .Round = 1 }, older->Value.Bots);

    // The engine holds the older roster now, but the newer request is still pending and shows as such.
    EXPECT_EQ(store.GetResult(), (MatchSetupResult{ .Outcome = MatchSetupOutcome::Pending, .Sequence = newer }));
    EXPECT_EQ(store.GetActiveBots(), MakeRoster(1, 0));
    EXPECT_TRUE(store.HasPending());

    const auto taken{ store.TakePending() };
    ASSERT_TRUE(taken.has_value());
    store.Finish(MatchSetupResult{ .Outcome = MatchSetupOutcome::Applied, .Sequence = newer, .Round = 2 }, taken->Value.Bots);

    EXPECT_EQ(store.GetResult().Round, 2);
    EXPECT_EQ(store.GetActiveBots(), MakeRoster(2, 0));
}

TEST(MatchSetupStoreTest, ARejectionDoesNotClearTheValidRequestThatWaitedAndItsOwnResultIsDropped)
{
    MatchSetupStore store{};
    store.Request(MatchSetup{ .Bots = MakeRoster(7, 8) }, MatchSetupOrigin::Bench);
    store.Finish(MatchSetupResult{ .Outcome = MatchSetupOutcome::AtLoad, .Sequence = 1 }, MakeRoster(7, 8));
    const std::uint64_t waiting{ store.Request(MatchSetup{ .Bots = MakeRoster(1, 1) }, MatchSetupOrigin::Bench) };

    const std::uint64_t sequence{ store.Reject("does not fit") };

    EXPECT_GT(sequence, waiting);
    EXPECT_TRUE(store.HasPending());
    EXPECT_EQ(store.GetResult(), (MatchSetupResult{ .Outcome = MatchSetupOutcome::Rejected, .Sequence = sequence, .Reason = "does not fit" }));
    EXPECT_EQ(store.GetActiveBots(), MakeRoster(7, 8));

    // The request that waited is still served, and what it did does not hide the newer refusal.
    const auto taken{ store.TakePending() };
    ASSERT_TRUE(taken.has_value());
    EXPECT_EQ(taken->Sequence, waiting);
    store.Finish(MatchSetupResult{ .Outcome = MatchSetupOutcome::Applied, .Sequence = waiting, .Round = 1 }, taken->Value.Bots);

    EXPECT_EQ(store.GetResult().Outcome, MatchSetupOutcome::Rejected);
    EXPECT_EQ(store.GetActiveBots(), MakeRoster(1, 1));
}

TEST(MatchSetupStoreTest, ARequestReplacesTheOneWaitingEvenOfTheSameOrigin)
{
    MatchSetupStore store{};
    PresetPick pick{};
    pick.Name = "slayer";
    store.Request(MatchSetup{ .Preset = pick }, MatchSetupOrigin::Bridge);
    store.Request(MatchSetup{ .Bots = MakeRoster(1, 1) }, MatchSetupOrigin::Bridge);

    const auto taken{ store.TakePending() };
    ASSERT_TRUE(taken.has_value());
    EXPECT_FALSE(taken->Value.Preset.has_value());
}

TEST(MatchSetupStoreTest, DroppingOnlyDropsTheRequestOfThatOrigin)
{
    MatchSetupStore store{};
    store.Request(MatchSetup{ .Bots = MakeRoster(1, 1) }, MatchSetupOrigin::Bench);

    store.DropPending(MatchSetupOrigin::Bridge);

    EXPECT_TRUE(store.HasPending());
    EXPECT_EQ(store.GetResult().Outcome, MatchSetupOutcome::Pending);
}

TEST(MatchSetupStoreTest, DroppingThePendingRequestSendsItsResultBackToNeutral)
{
    MatchSetupStore store{};
    store.Request(MatchSetup{ .Bots = MakeRoster(1, 1) }, MatchSetupOrigin::Bench);

    store.DropPending(MatchSetupOrigin::Bench);

    EXPECT_FALSE(store.HasPending());
    EXPECT_EQ(store.GetResult(), MatchSetupResult{});
}

TEST(MatchSetupStoreTest, TheLoadIsTakenOnce)
{
    MatchSetupStore store{};
    store.SetLoad(MatchSetupLoad{ .Sequence = 3, .Bots = MakeRoster(2, 2) });

    const auto load{ store.TakeLoad() };

    ASSERT_TRUE(load.has_value());
    EXPECT_EQ(load->Sequence, 3u);
    EXPECT_EQ(load->Bots, MakeRoster(2, 2));
    EXPECT_FALSE(store.TakeLoad().has_value());
}

TEST(MatchSetupStoreTest, CleanupKeepsTheActiveBotsAndAPendingRequestAndNeutralizesTheResult)
{
    MatchSetupStore store{};
    store.Request(MatchSetup{ .Bots = MakeRoster(7, 8) }, MatchSetupOrigin::Bench);
    store.Finish(MatchSetupResult{ .Outcome = MatchSetupOutcome::Applied, .Sequence = 1, .Round = 3 }, MakeRoster(7, 8));
    const std::uint64_t waiting{ store.Request(MatchSetup{ .Bots = MakeRoster(1, 2) }, MatchSetupOrigin::Bench) };
    store.SetLoad(MatchSetupLoad{ .Sequence = 1 });
    store.SetMatchStarted();

    store.Cleanup();

    EXPECT_EQ(store.GetActiveBots(), MakeRoster(7, 8));
    EXPECT_TRUE(store.HasPending());
    EXPECT_EQ(store.GetResult(), (MatchSetupResult{ .Outcome = MatchSetupOutcome::Pending, .Sequence = waiting }));
    EXPECT_FALSE(store.IsMatchStarted());
    EXPECT_FALSE(store.TakeLoad().has_value());
}

TEST(MatchSetupStoreTest, CleanupWithoutAPendingRequestResetsTheResultButKeepsTheActiveBots)
{
    MatchSetupStore store{};
    store.Request(MatchSetup{ .Bots = MakeRoster(7, 8) }, MatchSetupOrigin::Bench);
    const auto taken{ store.TakePending() };
    store.Finish(MatchSetupResult{ .Outcome = MatchSetupOutcome::Applied, .Sequence = taken->Sequence, .Round = 3 }, taken->Value.Bots);

    store.Cleanup();

    EXPECT_EQ(store.GetActiveBots(), MakeRoster(7, 8));
    EXPECT_EQ(store.GetResult(), MatchSetupResult{});
}

TEST(MatchSetupTypeTest, TheSetupLineShowsThePresetTheMapTheRosterAndWhatHappened)
{
    const ActiveMatchSetup active{ .Preset = "slayer", .Map = "reach bus", .Bots = MakeRoster(7, 8) };

    EXPECT_EQ(ToText(active, MatchSetupResult{ .Outcome = MatchSetupOutcome::AtLoad }), "Setup: preset slayer, map reach bus, roster 0:7 1:8 (at load)");
    EXPECT_EQ(ToText(active, MatchSetupResult{ .Outcome = MatchSetupOutcome::Applied, .Round = 2 }),
        "Setup: preset slayer, map reach bus, roster 0:7 1:8 (between rounds 2)");
    EXPECT_EQ(ToText(active, MatchSetupResult{ .Outcome = MatchSetupOutcome::Pending }), "Setup: preset slayer, map reach bus, roster 0:7 1:8 (pending)");
    EXPECT_EQ(ToText(active, MatchSetupResult{ .Outcome = MatchSetupOutcome::Rejected, .Reason = "no" }),
        "Setup: preset slayer, map reach bus, roster 0:7 1:8 (rejected: no)");
    EXPECT_EQ(ToText(ActiveMatchSetup{}, MatchSetupResult{}), "Setup: preset none, map none, roster none (applies at the next load)");
}

TEST(MatchSetupTypeTest, ARosterThatDoesNotFitThePlayerTableIsRefusedBeforeAnyMatch)
{
    EXPECT_FALSE(CheckRosterSize(MakeRoster(8, 7)).has_value());
    EXPECT_TRUE(CheckRosterSize(MakeRoster(8, 8)).has_value());

    Roster withAuto{ MakeRoster(8, 7) };
    withAuto.Auto = 1;
    EXPECT_TRUE(CheckRosterSize(withAuto).has_value());
}

TEST_F(MatchSetupTests, ARosterThatDoesNotFitIsRejectedAtOnceAndNothingWaits)
{
    const std::uint64_t sequence{ m_Service.Request(MatchSetup{ .Preset = this->Pick("fixture"), .Bots = MakeRoster(8, 8) }, MatchSetupOrigin::Bench) };

    EXPECT_FALSE(m_Store.HasPending());
    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::Rejected);
    EXPECT_EQ(m_Service.GetResult().Sequence, sequence);
    EXPECT_NE(m_Service.GetResult().Reason.find("roster"), std::string::npos);

    // The preset of the same request was not applied either.
    Buffer start{ MakeConfiguration(3006) };
    const Buffer original{ start };
    this->Start(start);

    EXPECT_EQ(start, original);
    EXPECT_TRUE(m_Service.GetActive().Preset.empty());
}

TEST_F(MatchSetupTests, ARequestInTheLobbyGivesAtLoadAndWritesPresetAndMapIntoTheCopy)
{
    EXPECT_FALSE(m_Service.HasLoadWork());

    const std::uint64_t sequence{ m_Service.Request(MatchSetup{ .Preset = this->Pick("fixture"), .Map = MakeMap("reach bus", 3006, std::byte{ 0x5C }) }, MatchSetupOrigin::Bench) };

    EXPECT_TRUE(m_Service.HasLoadWork());
    EXPECT_EQ(m_Service.GetResult(), (MatchSetupResult{ .Outcome = MatchSetupOutcome::Pending, .Sequence = sequence }));

    Buffer start{ MakeConfiguration(3006) };
    const Buffer original{ start };
    this->Start(start);

    EXPECT_NE(start, original);
    EXPECT_EQ(static_cast<int>(start[k_ConfigurationVariantOffset + k_VariantRespawnTime]), 3);
    EXPECT_EQ(static_cast<int>(start[k_ConfigurationMapVariantOffset + 0x10]), 0x5C);
    EXPECT_FALSE(m_Store.HasPending());
    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::AtLoad);
    EXPECT_EQ(m_Service.GetResult().Sequence, sequence);
    EXPECT_EQ(m_Service.GetActive().Preset, "fixture");
    EXPECT_EQ(m_Service.GetActive().Map, "reach bus");
}

TEST_F(MatchSetupTests, TheBotsOfARequestWaitForTheFirstTickAndTheResultStaysPending)
{
    const std::uint64_t sequence{ m_Service.Request(MatchSetup{ .Bots = MakeRoster(7, 8) }, MatchSetupOrigin::Bench) };

    Buffer start{ MakeConfiguration(3006) };
    const Buffer original{ start };
    this->Start(start);

    EXPECT_EQ(start, original);
    EXPECT_EQ(m_Service.GetResult(), (MatchSetupResult{ .Outcome = MatchSetupOutcome::Pending, .Sequence = sequence }));

    const auto load{ m_Store.TakeLoad() };
    ASSERT_TRUE(load.has_value());
    EXPECT_EQ(load->Sequence, sequence);
    EXPECT_EQ(load->Bots, MakeRoster(7, 8));
    // Active only once the first tick applied them.
    EXPECT_FALSE(m_Service.GetActive().Bots.has_value());
}

TEST_F(MatchSetupTests, AMapThatDoesNotFitRejectsTheWholeSetupAndRestoresTheCopyAndTheBase)
{
    m_Service.Request(MatchSetup{ .Preset = this->Pick("fixture"), .Map = MakeMap("other map", 3010, std::byte{ 0x5C }), .Bots = MakeRoster(1, 1) }, MatchSetupOrigin::Bench);

    Buffer start{ MakeConfiguration(3006) };
    const Buffer original{ start };
    this->Start(start);

    EXPECT_EQ(start, original);
    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::Rejected);
    EXPECT_NE(m_Service.GetResult().Reason.find("map id 3010 instead of 3006"), std::string::npos);
    // The preset written before the map is out again, with its base.
    EXPECT_EQ(m_PresetStore.GetBase(), nullptr);
    EXPECT_TRUE(m_Service.GetActive().Preset.empty());
    EXPECT_TRUE(m_Service.GetActive().Map.empty());
    // The bots do not wait for the first tick.
    const auto load{ m_Store.TakeLoad() };
    ASSERT_TRUE(load.has_value());
    EXPECT_FALSE(load->Bots.has_value());
    EXPECT_TRUE(Logged(LogLevel::Warning, "was not applied at load"));
}

TEST_F(MatchSetupTests, AnAbsentFieldDoesNotTouchTheActiveState)
{
    m_Service.Request(MatchSetup{ .Preset = this->Pick("fixture") }, MatchSetupOrigin::Bench);
    Buffer first{ MakeConfiguration(3006) };
    this->Start(first);
    ASSERT_EQ(m_Service.GetActive().Preset, "fixture");

    // Only the map: the preset stays and is written again.
    m_Service.Request(MatchSetup{ .Map = MakeMap("reach bus", 3006, std::byte{ 0x5C }) }, MatchSetupOrigin::Bench);
    Buffer second{ MakeConfiguration(3006) };
    this->Start(second);

    EXPECT_EQ(m_Service.GetActive().Preset, "fixture");
    EXPECT_EQ(m_Service.GetActive().Map, "reach bus");
    EXPECT_EQ(static_cast<int>(second[k_ConfigurationVariantOffset + k_VariantRespawnTime]), 3);
    EXPECT_EQ(static_cast<int>(second[k_ConfigurationMapVariantOffset + 0x10]), 0x5C);
    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::AtLoad);
}

TEST_F(MatchSetupTests, AnEmptyPresetAndAnEmptyMapGiveTheBaseValuesAtTheNextLoad)
{
    m_Service.Request(MatchSetup{ .Preset = this->Pick("fixture"), .Map = MakeMap("reach bus", 3006, std::byte{ 0x5C }) }, MatchSetupOrigin::Bench);
    Buffer first{ MakeConfiguration(3006) };
    this->Start(first);

    m_Service.Request(MatchSetup{ .Preset = PresetPick{}, .Map = std::shared_ptr<const ActiveMapVariant>{} }, MatchSetupOrigin::Bench);
    Buffer second{ MakeConfiguration(3006) };
    const Buffer original{ second };
    this->Start(second);

    EXPECT_EQ(second, original);
    EXPECT_TRUE(m_Service.GetActive().Preset.empty());
    EXPECT_TRUE(m_Service.GetActive().Map.empty());
    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::AtLoad);
    EXPECT_FALSE(m_Service.HasLoadWork());
}

TEST_F(MatchSetupTests, TheActiveSetupIsWrittenAgainAtEveryLoadWithoutARequest)
{
    m_Service.Request(MatchSetup{ .Preset = this->Pick("fixture") }, MatchSetupOrigin::Bench);
    Buffer first{ MakeConfiguration(3006) };
    this->Start(first);

    m_Store.Cleanup();
    EXPECT_EQ(m_Service.GetResult(), MatchSetupResult{});
    EXPECT_EQ(m_Service.GetActive().Preset, "fixture");

    Buffer second{ MakeConfiguration(3006) };
    this->Start(second);

    EXPECT_EQ(second, first);
    // A start with no request has no result to give.
    EXPECT_EQ(m_Service.GetResult(), MatchSetupResult{});
}

TEST_F(MatchSetupTests, AStartWithNoRequestNeverPublishesOverTheRejectionOfAPreviousRequest)
{
    m_Service.Request(MatchSetup{ .Preset = this->Pick("fixture") }, MatchSetupOrigin::Bench);
    Buffer first{ MakeConfiguration(3006) };
    this->Start(first);
    const std::uint64_t rejected{ m_Service.Reject("map \"x\": the map variant of the configuration cannot be read") };

    Buffer second{ MakeConfiguration(3006) };
    this->Start(second);

    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::Rejected);
    EXPECT_EQ(m_Service.GetResult().Sequence, rejected);
}

TEST_F(MatchSetupTests, ARefusedSetupAtLoadStillGivesTheStartThePreviousActiveSetup)
{
    m_Service.Request(MatchSetup{ .Preset = this->Pick("fixture") }, MatchSetupOrigin::Bench);
    Buffer first{ MakeConfiguration(3006) };
    this->Start(first);
    m_Store.Cleanup();

    const std::uint64_t refused{ m_Service.Request(MatchSetup{ .Map = MakeMap("other map", 3010, std::byte{ 0x5C }) }, MatchSetupOrigin::Bench) };
    Buffer second{ MakeConfiguration(3006) };
    this->Start(second);

    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::Rejected);
    EXPECT_EQ(m_Service.GetResult().Sequence, refused);
    // Not the plain configuration: the preset of the engine is in this start too, and the map is not.
    EXPECT_EQ(second, first);
    EXPECT_EQ(m_Service.GetActive().Preset, "fixture");
    EXPECT_TRUE(m_Service.GetActive().Map.empty());
    const auto load{ m_Store.TakeLoad() };
    ASSERT_TRUE(load.has_value());
    EXPECT_TRUE(load->IsWritten);
}

TEST_F(MatchSetupTests, ARequestWhileTheMatchRunsIsComparedWithTheMapOfThatMatch)
{
    Buffer start{ MakeConfiguration(3006) };
    m_Service.BeginStart(start);
    m_LifecycleStore.SetStatus(Status::Running);

    const std::uint64_t other{ m_Service.Request(MatchSetup{ .Map = MakeMap("other map", 3010, std::byte{ 0x5C }) }, MatchSetupOrigin::Bench) };

    EXPECT_FALSE(m_Store.HasPending());
    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::Rejected);
    EXPECT_EQ(m_Service.GetResult().Sequence, other);
    EXPECT_NE(m_Service.GetResult().Reason.find("map id 3010 instead of 3006"), std::string::npos);

    const std::uint64_t fits{ m_Service.Request(MatchSetup{ .Map = MakeMap("reach bus", 3006, std::byte{ 0x5C }) }, MatchSetupOrigin::Bench) };

    EXPECT_TRUE(m_Store.HasPending());
    EXPECT_EQ(m_Service.GetResult(), (MatchSetupResult{ .Outcome = MatchSetupOutcome::Pending, .Sequence = fits }));
}

TEST_F(MatchSetupTests, ARejectedRequestDoesNotClearTheValidOneThatWaited)
{
    m_Service.Request(MatchSetup{ .Bots = MakeRoster(1, 1) }, MatchSetupOrigin::Bench);

    m_Service.Reject("preset \"x\": the compiled file is missing");

    EXPECT_TRUE(m_Store.HasPending());
    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::Rejected);
    EXPECT_TRUE(m_Service.HasLoadWork());
}

TEST_F(MatchSetupTests, ARequestThatFailsItsChecksDoesNotClearTheOneThatWaited)
{
    m_Service.Request(MatchSetup{ .Bots = MakeRoster(1, 1) }, MatchSetupOrigin::Bench);

    m_Service.Request(MatchSetup{ .Bots = MakeRoster(8, 8) }, MatchSetupOrigin::Bench);

    EXPECT_TRUE(m_Store.HasPending());
    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::Rejected);
}

TEST_F(MatchSetupTests, OneRequestOfTheBridgeCarriesThePresetAndTheMapAndBothAreApplied)
{
    const std::uint64_t sequence{ m_Service.Request(MatchSetup{ .Preset = this->Pick("fixture"), .Map = MakeMap("reach bus", 3006, std::byte{ 0x5C }) }, MatchSetupOrigin::Bridge) };

    Buffer start{ MakeConfiguration(3006) };
    this->Start(start);

    EXPECT_EQ(m_Service.GetResult(), (MatchSetupResult{ .Outcome = MatchSetupOutcome::AtLoad, .Sequence = sequence, .Round = 0 }));
    EXPECT_EQ(m_Service.GetActive().Preset, "fixture");
    EXPECT_EQ(m_Service.GetActive().Map, "reach bus");
}

TEST_F(MatchSetupTests, CancelDropsOnlyTheRequestOfTheBridge)
{
    m_Service.Request(MatchSetup{ .Bots = MakeRoster(1, 1) }, MatchSetupOrigin::Bench);
    m_Service.Cancel(MatchSetupOrigin::Bridge);
    EXPECT_TRUE(m_Store.HasPending());

    m_Service.Request(MatchSetup{ .Bots = MakeRoster(1, 1) }, MatchSetupOrigin::Bridge);
    m_Service.Cancel(MatchSetupOrigin::Bridge);

    EXPECT_FALSE(m_Store.HasPending());
    EXPECT_EQ(m_Service.GetResult(), MatchSetupResult{});
}

TEST_F(MatchSetupTests, CleanupOfTheMatchKeepsThePresetAndTheMapForTheNextLoad)
{
    m_Service.Request(MatchSetup{ .Preset = this->Pick("fixture"), .Map = MakeMap("reach bus", 3006, std::byte{ 0x5C }) }, MatchSetupOrigin::Bench);
    Buffer start{ MakeConfiguration(3006) };
    this->Start(start);

    m_Store.Cleanup();

    EXPECT_EQ(m_Service.GetActive().Preset, "fixture");
    EXPECT_EQ(m_Service.GetActive().Map, "reach bus");
    EXPECT_EQ(m_Service.GetResult(), MatchSetupResult{});
    EXPECT_TRUE(m_Service.HasLoadWork());
}


// --- The round break and the first tick, with the memory of a match made by the test -------------------------------------------------

namespace
{
    constexpr std::uint8_t k_RoundStateInPlay{ 1 };

    // A preset that changes only what a round break can change: respawn time and round limit.
    constexpr std::string_view k_BetweenPreset{ "preset between\nsource 0\nwrite 2c7 spawn 03 ff\nwrite 2bf round_end 03 ff\n" };

    class MatchSetupBreakTests : public MatchSetupTests
    {
    protected:
        FakeGame m_Game{ MatchSetupTests::MakeConfiguration(3006) };
        FakeRosterPort m_Port{};

        auto SetUp() -> void override
        {
            MatchSetupTests::SetUp();

            std::ofstream stream{ m_Folder / "Presets" / "between.txt", std::ios::binary | std::ios::trunc };
            stream << k_BetweenPreset;
            stream.close();

            m_Service.SetRosterPort(&m_Port);
            m_Game.SetRound(k_RoundStateBetween, 1);
        }

        auto TearDown() -> void override
        {
            m_Service.SetRosterPort(nullptr);
            MatchSetupTests::TearDown();
        }

        // The first tick of a match that started with no setup.
        auto FirstTick() -> void
        {
            Buffer start{ MatchSetupTests::MakeConfiguration(3006) };
            this->Start(start);
            m_Service.UpdateLoad(m_Game.Context());
        }

        auto RespawnTime() -> int { return static_cast<int>(m_Game.Variant()[k_VariantRespawnTime]); }
    };
}

TEST_F(MatchSetupBreakTests, TheRequestIsAppliedWholeInTheOrderPresetMapRoster)
{
    this->FirstTick();
    const Buffer before{ m_Game.Snapshot() };
    int respawnAtRoster{ -1 };
    int mapAtRoster{ -1 };
    m_Port.m_OnApply = [&] {
        respawnAtRoster = this->RespawnTime();
        mapAtRoster = static_cast<int>(m_Game.Options()[0x10]);
    };

    const std::uint64_t sequence{ m_Service.Request(MatchSetup{ .Preset = this->Pick("between"), .Map = MakeMap("reach bus", 3006, std::byte{ 0x5C }),
        .Bots = MakeRoster(1, 2) }, MatchSetupOrigin::Bench) };

    m_Service.UpdateBreak(m_Game.Context());

    EXPECT_NE(m_Game.Snapshot(), before);
    EXPECT_EQ(this->RespawnTime(), 3);
    EXPECT_EQ(static_cast<int>(m_Game.Options()[0x10]), 0x5C);
    EXPECT_EQ(static_cast<int>(m_Game.RuntimeData()[0x10]), 0x5C);
    // The roster is applied last: the preset and the map variant were in already.
    EXPECT_EQ(respawnAtRoster, 3);
    EXPECT_EQ(mapAtRoster, 0x5C);
    EXPECT_EQ(m_Port.m_Applies, 1u);
    EXPECT_EQ(m_Service.GetResult(), (MatchSetupResult{ .Outcome = MatchSetupOutcome::Applied, .Sequence = sequence, .Round = 1 }));
    EXPECT_EQ(m_Service.GetActive().Preset, "between");
    EXPECT_EQ(m_Service.GetActive().Map, "reach bus");
    EXPECT_EQ(m_Service.GetActive().Bots, MakeRoster(1, 2));
}

TEST_F(MatchSetupBreakTests, ARosterThatDoesNotFitTheMatchRejectsTheWholeSetupBeforeAnythingIsWritten)
{
    this->FirstTick();
    const Buffer before{ m_Game.Snapshot() };
    m_Port.m_CheckReason = "6 bots and 12 other players do not fit in 16 player slots";

    m_Service.Request(MatchSetup{ .Preset = this->Pick("between"), .Map = MakeMap("reach bus", 3006, std::byte{ 0x5C }), .Bots = MakeRoster(3, 3) },
        MatchSetupOrigin::Bench);
    m_Service.UpdateBreak(m_Game.Context());

    EXPECT_EQ(m_Game.Snapshot(), before);
    EXPECT_EQ(m_Port.m_Applies, 0u);
    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::Rejected);
    EXPECT_NE(m_Service.GetResult().Reason.find("do not fit"), std::string::npos);
    EXPECT_TRUE(m_Service.GetActive().Preset.empty());
    EXPECT_TRUE(m_Service.GetActive().Map.empty());
    EXPECT_FALSE(m_Service.GetActive().Bots.has_value());
    EXPECT_FALSE(m_Store.HasPending());
}

TEST_F(MatchSetupBreakTests, AMapThatDoesNotFitTheMatchLeavesThePresetUnwrittenToo)
{
    this->FirstTick();
    const Buffer before{ m_Game.Snapshot() };

    m_Service.Request(MatchSetup{ .Preset = this->Pick("between"), .Map = MakeMap("other map", 3010, std::byte{ 0x5C }), .Bots = MakeRoster(1, 1) },
        MatchSetupOrigin::Bench);
    m_Service.UpdateBreak(m_Game.Context());

    EXPECT_EQ(m_Game.Snapshot(), before);
    EXPECT_EQ(m_Port.m_Applies, 0u);
    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::Rejected);
    EXPECT_NE(m_Service.GetResult().Reason.find("map id 3010 instead of 3006"), std::string::npos);
    EXPECT_EQ(m_PresetStore.GetBase(), nullptr);
}

TEST_F(MatchSetupBreakTests, ARosterThatFailsAfterThePresetWentInIsAppliedWithAPartialReasonNeverRejected)
{
    this->FirstTick();
    m_Port.m_Application = RosterApplication{ .Done = 1, .Total = 3 };

    const std::uint64_t sequence{ m_Service.Request(MatchSetup{ .Preset = this->Pick("between"), .Bots = MakeRoster(2, 2) }, MatchSetupOrigin::Bench) };
    m_Service.UpdateBreak(m_Game.Context());

    EXPECT_EQ(this->RespawnTime(), 3);
    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::Applied);
    EXPECT_EQ(m_Service.GetResult().Sequence, sequence);
    EXPECT_EQ(m_Service.GetResult().Reason, "partial: roster 1 of 3 operations done, see the log");
    EXPECT_EQ(m_Service.GetActive().Preset, "between");
    // The bots the engine holds are not the roster: it is not the active one.
    EXPECT_FALSE(m_Service.GetActive().Bots.has_value());
}

TEST_F(MatchSetupBreakTests, ARosterThatThrowsAfterThePresetWentInIsAlsoAPartialResult)
{
    this->FirstTick();
    m_Port.m_OnApply = [] { throw std::runtime_error{ "engine call failed" }; };

    m_Service.Request(MatchSetup{ .Preset = this->Pick("between"), .Bots = MakeRoster(2, 2) }, MatchSetupOrigin::Bench);

    // The hook never lets an exception out of the service.
    EXPECT_NO_THROW(m_Service.UpdateBreak(m_Game.Context()));
    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::Applied);
    EXPECT_NE(m_Service.GetResult().Reason.find("partial"), std::string::npos);
}

TEST_F(MatchSetupBreakTests, ARosterOnlyThatDoesNothingIsRejected)
{
    this->FirstTick();
    m_Port.m_Application = RosterApplication{ .Done = 0, .Total = 2 };

    m_Service.Request(MatchSetup{ .Bots = MakeRoster(2, 2) }, MatchSetupOrigin::Bench);
    m_Service.UpdateBreak(m_Game.Context());

    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::Rejected);
    EXPECT_FALSE(m_Service.GetActive().Bots.has_value());
}

TEST_F(MatchSetupBreakTests, AnExceptionAfterTheRequestWasTakenEndsItAsRejectedForItsSequence)
{
    this->FirstTick();
    m_Port.m_IsThrowing = true;

    const std::uint64_t sequence{ m_Service.Request(MatchSetup{ .Preset = this->Pick("between"), .Bots = MakeRoster(1, 1) }, MatchSetupOrigin::Bench) };
    const Buffer before{ m_Game.Snapshot() };

    EXPECT_NO_THROW(m_Service.UpdateBreak(m_Game.Context()));

    EXPECT_FALSE(m_Store.HasPending());
    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::Rejected);
    EXPECT_EQ(m_Service.GetResult().Sequence, sequence);
    EXPECT_EQ(m_Game.Snapshot(), before);
}

TEST_F(MatchSetupBreakTests, ARequestWaitsWhileTheRoundIsInPlayAndIsServedOnceAtTheBreak)
{
    this->FirstTick();
    m_Service.Request(MatchSetup{ .Preset = this->Pick("between") }, MatchSetupOrigin::Bench);

    m_Game.SetRound(k_RoundStateInPlay, 1);
    m_Service.UpdateBreak(m_Game.Context());
    EXPECT_TRUE(m_Store.HasPending());
    EXPECT_NE(this->RespawnTime(), 3);

    m_Game.SetRound(k_RoundStateEnded, 1);
    m_Service.UpdateBreak(m_Game.Context());
    EXPECT_FALSE(m_Store.HasPending());
    EXPECT_EQ(this->RespawnTime(), 3);
    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::Applied);
}

TEST_F(MatchSetupBreakTests, ARequestWithARosterWaitsWhileThereIsNoBotsService)
{
    this->FirstTick();
    m_Service.SetRosterPort(nullptr);
    m_Service.Request(MatchSetup{ .Bots = MakeRoster(1, 1) }, MatchSetupOrigin::Bench);

    m_Service.UpdateBreak(m_Game.Context());

    EXPECT_TRUE(m_Store.HasPending());
}

TEST_F(MatchSetupBreakTests, NothingIsServedBeforeTheFirstTickOfTheMatch)
{
    m_Service.Request(MatchSetup{ .Preset = this->Pick("between") }, MatchSetupOrigin::Bench);

    m_Service.UpdateBreak(m_Game.Context());

    EXPECT_TRUE(m_Store.HasPending());
    EXPECT_NE(this->RespawnTime(), 3);
}

TEST_F(MatchSetupBreakTests, ARequestMadeAfterTheStartAndBeforeTheFirstTickIsServedAsPartOfTheLoadWhateverTheRound)
{
    Buffer start{ MatchSetupTests::MakeConfiguration(3006) };
    this->Start(start);
    const std::uint64_t sequence{ m_Service.Request(MatchSetup{ .Preset = this->Pick("between"), .Bots = MakeRoster(1, 1) }, MatchSetupOrigin::Bench) };

    m_Game.SetRound(k_RoundStateInPlay, 0);
    m_Service.UpdateLoad(m_Game.Context());
    EXPECT_TRUE(m_Store.HasPending());

    m_Service.UpdateBreak(m_Game.Context());

    EXPECT_FALSE(m_Store.HasPending());
    EXPECT_EQ(this->RespawnTime(), 3);
    EXPECT_EQ(m_Port.m_Applies, 1u);
    EXPECT_EQ(m_Service.GetResult(), (MatchSetupResult{ .Outcome = MatchSetupOutcome::AtLoad, .Sequence = sequence, .Round = 0 }));
}

TEST_F(MatchSetupBreakTests, TheBotsOfTheStartAreAppliedAtTheFirstTickAndTheResultIsAtLoad)
{
    const std::uint64_t sequence{ m_Service.Request(MatchSetup{ .Bots = MakeRoster(7, 8) }, MatchSetupOrigin::Bench) };
    Buffer start{ MatchSetupTests::MakeConfiguration(3006) };
    this->Start(start);

    m_Game.SetRound(k_RoundStateInPlay, 0);
    m_Service.UpdateLoad(m_Game.Context());

    EXPECT_EQ(m_Port.m_Applies, 1u);
    EXPECT_EQ(m_Service.GetResult(), (MatchSetupResult{ .Outcome = MatchSetupOutcome::AtLoad, .Sequence = sequence, .Round = 0 }));
    EXPECT_EQ(m_Service.GetActive().Bots, MakeRoster(7, 8));

    // The first tick goes by once.
    m_Service.UpdateLoad(m_Game.Context());
    EXPECT_EQ(m_Port.m_Applies, 1u);
}

TEST_F(MatchSetupBreakTests, ARosterThatFailsAtTheFirstTickAfterThePresetWentIntoTheStartIsAtLoadWithAPartialReason)
{
    m_Port.m_CheckReason = "no free slots";
    const std::uint64_t sequence{ m_Service.Request(MatchSetup{ .Preset = this->Pick("fixture"), .Bots = MakeRoster(7, 8) }, MatchSetupOrigin::Bench) };
    Buffer start{ MatchSetupTests::MakeConfiguration(3006) };
    this->Start(start);
    ASSERT_EQ(m_Service.GetActive().Preset, "fixture");

    m_Service.UpdateLoad(m_Game.Context());

    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::AtLoad);
    EXPECT_EQ(m_Service.GetResult().Sequence, sequence);
    EXPECT_EQ(m_Service.GetResult().Reason, "partial: roster: no free slots");
    EXPECT_FALSE(m_Service.GetActive().Bots.has_value());
}

TEST_F(MatchSetupBreakTests, ARosterThatFailsAtTheFirstTickWithNothingWrittenIntoTheStartIsRejected)
{
    m_Port.m_CheckReason = "no free slots";
    m_Service.Request(MatchSetup{ .Bots = MakeRoster(7, 8) }, MatchSetupOrigin::Bench);
    Buffer start{ MatchSetupTests::MakeConfiguration(3006) };
    this->Start(start);

    m_Service.UpdateLoad(m_Game.Context());

    EXPECT_EQ(m_Service.GetResult().Outcome, MatchSetupOutcome::Rejected);
}

TEST_F(MatchSetupBreakTests, TheActiveBotsAtAStartWithNoRequestAreAppliedWithoutAResult)
{
    m_Service.Request(MatchSetup{ .Bots = MakeRoster(7, 8) }, MatchSetupOrigin::Bench);
    Buffer first{ MatchSetupTests::MakeConfiguration(3006) };
    this->Start(first);
    m_Service.UpdateLoad(m_Game.Context());
    m_Store.Cleanup();
    ASSERT_EQ(m_Port.m_Applies, 1u);

    Buffer second{ MatchSetupTests::MakeConfiguration(3006) };
    this->Start(second);
    m_Service.UpdateLoad(m_Game.Context());

    EXPECT_EQ(m_Port.m_Applies, 2u);
    EXPECT_EQ(m_Service.GetResult(), MatchSetupResult{});
}
