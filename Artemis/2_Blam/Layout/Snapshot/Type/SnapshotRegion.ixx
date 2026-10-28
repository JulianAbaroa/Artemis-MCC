export module Layout.Snapshot.Type:Region;

import Common.Engine.Type;
import Datum.Object.Type;
import Layout.Player.Type;
import Layout.Interaction.Type;
import Layout.Game.Type;
import std;

namespace
{
    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_MaxLocalUsers;
    using Datum::Object::Type::Constant::k_TimeGlobalsTickFractionOffset;
    using Datum::Object::Type::Constant::k_EventGlobalsSequenceOffset;
    using Datum::Object::Type::Constant::k_MaxPlacements;
    using Datum::Object::Type::Constant::k_PlacementSize;
    using Layout::Player::Type::Size::k_Base;
    using Layout::Player::Type::Mapping::k_OutputUserPlayer;
    using Layout::Player::Type::Mapping::k_EntryStride;
    using Layout::Interaction::Type::Offset::k_OutputUserStride;
    using Layout::Game::Type::Offset::k_GameEnginePlayerScores;
    using Layout::Game::Type::Offset::k_PlayerScoreStride;
}

export namespace Layout::Snapshot::Type
{
    // Block of game memory that is copied whole into the snapshot, one per structure the tick reads field by field.
    // note: The order is the index in Snapshot::Regions.
    enum class RegionId : std::uint8_t
    {
        PlayerArray,
        PlayerMapping,
        Interaction,
        TimeGlobals,
        GameEngineGlobals,
        EventGlobals,
        Placements
    };

    // Number of regions, the size of Snapshot::Regions.
    inline constexpr std::size_t k_RegionCount{ 7 };

    // One block of game memory as it was when the snapshot was taken.
    struct Region
    {
        // Address the bytes were copied from. Only for tools and logs, nothing reads memory with it.
        std::uintptr_t OriginalAddress{};
        // Empty when the region was not captured.
        std::vector<std::uint8_t> Bytes{};
    };

    // return: Bytes that Capture copies for the region, the last field the code reads or the whole known structure.
    constexpr auto RegionSize(RegionId id) -> std::size_t
    {
        switch (id)
        {
        case RegionId::PlayerArray: return k_MaxPlayers * k_Base;
        case RegionId::PlayerMapping: return k_OutputUserPlayer + k_MaxLocalUsers * k_EntryStride;
        case RegionId::Interaction: return k_MaxLocalUsers * k_OutputUserStride;
        case RegionId::TimeGlobals: return k_TimeGlobalsTickFractionOffset + sizeof(float);
        case RegionId::GameEngineGlobals: return k_GameEnginePlayerScores + k_MaxPlayers * k_PlayerScoreStride;
        case RegionId::EventGlobals: return k_EventGlobalsSequenceOffset + sizeof(std::uint32_t);
        case RegionId::Placements: return static_cast<std::size_t>(k_MaxPlacements) * k_PlacementSize;
        }

        return 0;
    }
}