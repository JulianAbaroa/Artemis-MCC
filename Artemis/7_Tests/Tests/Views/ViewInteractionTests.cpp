#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Blam.Runtime.Interaction.Type;
import Perception.Entities.Type;
import Perception.Players.Type;
import Views.View.Type;
import Tests.Support.Pipeline;
import std;

// The object the engine would have each agent use (F5.1 A, D371): the selection of the action applied to the agent, copied into its View
// with the function and subtype of the entity it points at.

namespace
{
    using Common::Semantic::Type::Function;
    using Common::Team::Type::Team;
    using Blam::Runtime::Interaction::Type::Alive::InteractionDetail;
    using Blam::Runtime::Interaction::Type::Alive::InteractionKind;
    using Blam::Runtime::Interaction::Type::Alive::ObjectSelections;
    using Perception::Entities::Type::Entities;
    using Perception::Entities::Type::Entity;
    using Perception::Players::Type::Body;
    using Perception::Players::Type::Player;
    using Perception::Players::Type::Players;
    using Tests::Support::Pipeline::Chain;
    using Views::View::Type::AgentSet;
    using Views::View::Type::View;
    using Views::View::Type::ViewTable;

    using Common::Engine::Type::k_NoHandle;

    // Player table indices 1, 2 and 5.
    constexpr std::uint32_t k_PlayerA{ 0xE5A10001 };
    constexpr std::uint32_t k_PlayerB{ 0xE5A20002 };
    constexpr std::uint32_t k_PlayerC{ 0xE5A30005 };
    constexpr std::uint32_t k_Weapon{ 0xAA010021 };
    constexpr std::uint32_t k_Vehicle{ 0xBB010031 };
    constexpr std::uint32_t k_Flag{ 0xF1A60001 };

    auto MakePlayer(std::uint32_t handle, bool isAlive) -> Player
    {
        Player player{};
        player.Handle = handle;
        player.Team = Team::Red;
        if (isAlive) player.Body = Body{};
        return player;
    }

    auto MakeEntity(std::uint32_t handle, Function function, std::uint8_t subtype) -> Entity
    {
        Entity entity{};
        entity.Handle = handle;
        entity.Function = function;
        entity.Subtype = subtype;
        return entity;
    }

    class ViewInteractionTests : public ::testing::Test, protected Chain
    {
    protected:
        auto PublishPlayers(std::vector<Player> rows) -> void
        {
            std::ranges::sort(rows, {}, &Player::Handle);
            Players players{};
            players.Rows = std::move(rows);
            for (std::size_t row{ 0 }; row < players.Rows.size(); ++row) players.Index[players.Rows[row].Handle] = row;
            m_Environment.m_PlayersStore.Publish(std::move(players));
        }

        auto PublishEntities(std::vector<Entity> rows) -> void
        {
            std::ranges::sort(rows, {}, &Entity::Handle);
            Entities entities{};
            entities.Rows = std::move(rows);
            m_Environment.m_EntitiesStore.Publish(std::move(entities));
        }

        // Selection of the player table index, as the runtime decodes it from the applied action.
        auto Select(ObjectSelections& selections, std::size_t index, InteractionKind kind, InteractionDetail detail, std::uint32_t handle) -> void
        {
            selections[index] = { .Kind = kind, .Detail = detail, .TargetHandle = handle };
        }

        auto Compute(const AgentSet& agents) -> ViewTable
        {
            m_Egocentric.m_ViewsService.Update(agents);
            const auto table{ m_Egocentric.m_ViewsStore.Acquire() };
            return table ? *table : ViewTable{};
        }

        // return: A set of selections with nothing selected, as the runtime publishes it.
        static auto NoSelections() -> ObjectSelections
        {
            return ObjectSelections{};
        }
    };

    auto IsNone(const View& view) -> bool
    {
        return view.Interaction.Kind == InteractionKind::None && view.Interaction.Detail == InteractionDetail::None &&
            view.Interaction.TargetHandle == k_NoHandle && view.Interaction.TargetFunction == Function::Unknown && view.Interaction.TargetSubtype == 0;
    }
}

TEST_F(ViewInteractionTests, CopiesTheSelectionOfEachAgent)
{
    this->PublishPlayers({ MakePlayer(k_PlayerA, true), MakePlayer(k_PlayerB, true), MakePlayer(k_PlayerC, true) });
    this->PublishEntities({ MakeEntity(k_Weapon, Function::Weapon, 7), MakeEntity(k_Vehicle, Function::Vehicle, 3) });

    ObjectSelections selections{ NoSelections() };
    this->Select(selections, 1, InteractionKind::GrabWeapon, InteractionDetail::GrabWeapon, k_Weapon);
    this->Select(selections, 2, InteractionKind::EnterVehicle, InteractionDetail::FirstSeat, k_Vehicle);
    m_Tables.m_InteractionStore.PublishSelections(selections);

    const ViewTable views{ this->Compute(AgentSet::All()) };

    // Rows follow the handle order: A (index 1), B (index 2), C (index 5, nothing selected).
    EXPECT_EQ(views[0].Interaction.Kind, InteractionKind::GrabWeapon);
    EXPECT_EQ(views[0].Interaction.Detail, InteractionDetail::GrabWeapon);
    EXPECT_EQ(views[0].Interaction.TargetHandle, k_Weapon);
    EXPECT_EQ(views[1].Interaction.Kind, InteractionKind::EnterVehicle);
    EXPECT_EQ(views[1].Interaction.Detail, InteractionDetail::FirstSeat);
    EXPECT_EQ(views[1].Interaction.TargetHandle, k_Vehicle);
    EXPECT_TRUE(IsNone(views[2]));
}

TEST_F(ViewInteractionTests, TargetFunctionComesFromEntities)
{
    this->PublishPlayers({ MakePlayer(k_PlayerA, true), MakePlayer(k_PlayerB, true) });
    this->PublishEntities({ MakeEntity(k_Weapon, Function::Weapon, 7) });

    ObjectSelections selections{ NoSelections() };
    this->Select(selections, 1, InteractionKind::GrabWeapon, InteractionDetail::GrabWeapon, k_Weapon);
    // The target of the other agent is not in Entities: the handle is kept, the function is Unknown and the subtype 0.
    this->Select(selections, 2, InteractionKind::GrabWeapon, InteractionDetail::ChangeWeapon, 0xAA990077);
    m_Tables.m_InteractionStore.PublishSelections(selections);

    const ViewTable views{ this->Compute(AgentSet::All()) };

    EXPECT_EQ(views[0].Interaction.TargetFunction, Function::Weapon);
    EXPECT_EQ(views[0].Interaction.TargetSubtype, 7);
    EXPECT_EQ(views[1].Interaction.TargetHandle, 0xAA990077u);
    EXPECT_EQ(views[1].Interaction.TargetFunction, Function::Unknown);
    EXPECT_EQ(views[1].Interaction.TargetSubtype, 0);
}

TEST_F(ViewInteractionTests, DeadAgentHasNone)
{
    this->PublishPlayers({ MakePlayer(k_PlayerA, false), MakePlayer(k_PlayerB, true) });
    this->PublishEntities({ MakeEntity(k_Weapon, Function::Weapon, 7) });

    ObjectSelections selections{ NoSelections() };
    this->Select(selections, 1, InteractionKind::GrabWeapon, InteractionDetail::GrabWeapon, k_Weapon);
    this->Select(selections, 2, InteractionKind::GrabWeapon, InteractionDetail::GrabWeapon, k_Weapon);
    m_Tables.m_InteractionStore.PublishSelections(selections);

    const ViewTable views{ this->Compute(AgentSet::All()) };

    EXPECT_TRUE(views[0].Head.IsComputed);
    EXPECT_FALSE(views[0].Head.HasBody);
    EXPECT_TRUE(IsNone(views[0]));
    EXPECT_EQ(views[1].Interaction.Kind, InteractionKind::GrabWeapon);
}

TEST_F(ViewInteractionTests, NoSelectionsPublishedGivesNoneToTheComputedViews)
{
    // A tick without published actions publishes all None, and before the first publish there is no store content at all.
    this->PublishPlayers({ MakePlayer(k_PlayerA, true) });
    EXPECT_TRUE(IsNone(this->Compute(AgentSet::All())[0]));

    m_Tables.m_InteractionStore.PublishSelections(NoSelections());
    EXPECT_TRUE(IsNone(this->Compute(AgentSet::All())[0]));
}

TEST_F(ViewInteractionTests, OutsideTheSetIsZero)
{
    this->PublishPlayers({ MakePlayer(k_PlayerA, true), MakePlayer(k_PlayerB, true), MakePlayer(k_PlayerC, true) });
    this->PublishEntities({ MakeEntity(k_Weapon, Function::Weapon, 7) });

    ObjectSelections selections{ NoSelections() };
    for (std::size_t index : { std::size_t{ 1 }, std::size_t{ 2 }, std::size_t{ 5 } })
    {
        this->Select(selections, index, InteractionKind::GrabWeapon, InteractionDetail::GrabWeapon, k_Weapon);
    }
    m_Tables.m_InteractionStore.PublishSelections(selections);

    AgentSet agents{};
    agents.Add(k_PlayerB);
    const ViewTable views{ this->Compute(agents) };

    for (const std::size_t row : { std::size_t{ 0 }, std::size_t{ 2 } })
    {
        EXPECT_EQ(views[row].Interaction.Kind, InteractionKind::None) << row;
        EXPECT_EQ(static_cast<std::uint8_t>(views[row].Interaction.Detail), 0) << row;
        EXPECT_EQ(views[row].Interaction.TargetHandle, 0u) << row;
        EXPECT_EQ(views[row].Interaction.TargetFunction, Function::Unknown) << row;
        EXPECT_EQ(views[row].Interaction.TargetSubtype, 0) << row;
    }
    EXPECT_EQ(views[1].Interaction.Kind, InteractionKind::GrabWeapon);
}

TEST_F(ViewInteractionTests, FlagIsGrabObjective)
{
    this->PublishPlayers({ MakePlayer(k_PlayerA, true) });
    this->PublishEntities({ MakeEntity(k_Flag, Function::ObjectiveItem, 1) });

    ObjectSelections selections{ NoSelections() };
    this->Select(selections, 1, InteractionKind::GrabObjective, InteractionDetail::None, k_Flag);
    m_Tables.m_InteractionStore.PublishSelections(selections);

    const View view{ this->Compute(AgentSet::All())[0] };

    EXPECT_EQ(view.Interaction.Kind, InteractionKind::GrabObjective);
    EXPECT_EQ(view.Interaction.TargetHandle, k_Flag);
    EXPECT_EQ(view.Interaction.TargetFunction, Function::ObjectiveItem);
}