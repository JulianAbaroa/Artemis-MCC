export module Blam.Layout.Snapshot.Type:Region;

import Common.Engine.Type;
import Blam.Layout.Object.Type;
import Blam.Layout.Player.Type;
import Blam.Layout.Interaction.Type;
import std;

namespace
{
    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_MaxLocalUsers;
    using Blam::Layout::Object::Type::Constant::k_MappingGlobalsSize;
    using Blam::Layout::Object::Type::Constant::k_TimeGlobalsSize;
    using Blam::Layout::Object::Type::Constant::k_GameEngineGlobalsSize;
    using Blam::Layout::Object::Type::Constant::k_EventGlobalsSize;
    using Blam::Layout::Object::Type::Constant::k_GameGlobalsSize;
    using Blam::Layout::Object::Type::Constant::k_MaxPlacements;
    using Blam::Layout::Object::Type::Constant::k_PlacementSize;
    using Blam::Layout::Player::Type::Size::k_Base;
    using Blam::Layout::Interaction::Type::Offset::k_OutputUserStride;
}

export namespace Blam::Layout::Snapshot::Type
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
        Placements,
        GameGlobals
    };

    // Number of regions, the size of Snapshot::Regions.
    inline constexpr std::size_t k_RegionCount{ 8 };

    // One block of game memory as it was when the snapshot was taken.
    struct Region
    {
        // Address the bytes were copied from. Only for tools and logs, nothing reads memory with it.
        std::uintptr_t OriginalAddress{};
        // Empty when the region was not captured.
        std::vector<std::uint8_t> Bytes{};
    };

    // return: Bytes that Capture copies for the region: the whole block the engine registers, or the whole array (D249).
    // note: A field read later is then already in the recordings made before it.
    constexpr auto RegionSize(RegionId id) -> std::size_t
    {
        switch (id)
        {
        case RegionId::PlayerArray: return k_MaxPlayers * k_Base;
        case RegionId::PlayerMapping: return k_MappingGlobalsSize;
        case RegionId::Interaction: return k_MaxLocalUsers * k_OutputUserStride;
        case RegionId::TimeGlobals: return k_TimeGlobalsSize;
        case RegionId::GameEngineGlobals: return k_GameEngineGlobalsSize;
        case RegionId::EventGlobals: return k_EventGlobalsSize;
        case RegionId::Placements: return static_cast<std::size_t>(k_MaxPlacements) * k_PlacementSize;
        case RegionId::GameGlobals: return k_GameGlobalsSize;
        }

        return 0;
    }
}