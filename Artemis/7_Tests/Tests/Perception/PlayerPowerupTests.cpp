#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Semantic.Type;
import Blam.Layout.Game.Type;
import Blam.Runtime.Game.Type;
import Blam.Runtime.Player.Type;
import Perception.Entities.Type;
import Perception.Players.Type;
import Tests.Support.Pipeline;
import std;

// A18 (D263): the custom powerup of each player in the Players table, from the pickups of the player table and the game time and
// powerup durations of the game store. The real values are checked by ReplayTests.PowerupsFilm*.

namespace
{
    using Common::Semantic::Type::Subtype::Powerup;
    using Blam::Runtime::Game::Type::Alive::AliveGame;
    using Blam::Runtime::Player::Type::Alive::AlivePlayer;
    using Blam::Runtime::Player::Type::Alive::ConnectionState;
    using Perception::Players::Type::Player;
    using Tests::Support::Pipeline::Chain;

    using Blam::Layout::Game::Type::Offset::k_PowerupCount;
    using Common::Engine::Type::k_NoHandle;

    constexpr std::uint32_t k_PlayerHandle{ 0xE5A10000 };

    class PlayerPowerupTests : public ::testing::Test
    {
    protected:
        // Publishes a player without biped with the pickups, runs the players service and returns its row.
        // param game: Whether the game store has a game, with the tick and the durations given.
        auto Step(const std::array<std::int32_t, k_PowerupCount>& pickups, bool hasGame, std::uint32_t tick = 0,
            const std::array<std::uint32_t, k_PowerupCount>& durations = {}) -> Player
        {
            AlivePlayer player{};
            player.Handle = k_PlayerHandle;
            player.ConnectionState = ConnectionState::Connected;
            player.AliveBipedHandle = k_NoHandle;
            player.CurrentBipedHandle = k_NoHandle;
            player.PowerupPickupTicks = pickups;
            m_Chain.m_Tables.m_PlayerStore.RemovePlayer(k_PlayerHandle);
            m_Chain.m_Tables.m_PlayerStore.AddPlayer(k_PlayerHandle, player);
            m_Chain.m_Tables.m_PlayerStore.Publish();
            m_Chain.m_Tables.m_ObjectStore.Publish();
            m_Chain.m_Environment.m_EntitiesStore.Publish(Perception::Entities::Type::Entities{});

            if (hasGame)
            {
                AliveGame game{};
                game.EngineTick = tick;
                game.PowerupDurationTicks = durations;
                m_Chain.m_Tables.m_GameStore.Publish(std::move(game));
            }

            m_Chain.m_Environment.m_PlayersService.Update();
            return m_Chain.m_Environment.m_PlayersStore.Acquire()->Rows.at(0);
        }

        Chain m_Chain{};
    };
}

TEST_F(PlayerPowerupTests, ThePickupsAndTheGameClockGiveThePowerupAlsoWithoutBody)
{
    const std::array<std::uint32_t, k_PowerupCount> durations{ 600, 900, 1200 };

    const Player red{ this->Step({ 1321, 0, 0 }, true, 1322, durations) };
    EXPECT_FALSE(red.IsAlive());
    EXPECT_EQ(red.Powerup, Powerup::Red);
    EXPECT_EQ(red.PowerupTicks, 600u);

    const Player yellow{ this->Step({ 1321, 4015, 4253 }, true, 4254, durations) };
    EXPECT_EQ(yellow.Powerup, Powerup::Yellow);
    EXPECT_EQ(yellow.PowerupTicks, 1200u);

    const Player expired{ this->Step({ 1321, 0, 0 }, true, 1922, durations) };
    EXPECT_EQ(expired.Powerup, Powerup::None);
    EXPECT_EQ(expired.PowerupTicks, 0u);
}

TEST_F(PlayerPowerupTests, WithoutPickupsThereIsNoPowerup)
{
    const Player none{ this->Step({ 0, 0, 0 }, true, 500, { 600, 900, 1200 }) };
    EXPECT_EQ(none.Powerup, Powerup::None);
    EXPECT_EQ(none.PowerupTicks, 0u);
}

TEST_F(PlayerPowerupTests, WithoutGameThereIsNoPowerup)
{
    const Player noGame{ this->Step({ 1321, 0, 0 }, false) };
    EXPECT_EQ(noGame.Powerup, Powerup::None);
    EXPECT_EQ(noGame.PowerupTicks, 0u);
}