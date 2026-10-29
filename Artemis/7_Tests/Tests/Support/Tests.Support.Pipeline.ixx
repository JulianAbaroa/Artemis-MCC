export module Tests.Support.Pipeline;

import Common.Engine.Type;
import Common.Log.Type;
import Common.Log.System;
import Common.Math.Type;
import Common.Team.Type;
import Blam.Layout.Object.Type;
import Blam.Layout.Player.Type;
import Blam.Layout.Snapshot.Type;
import Blam.Map.Layer;
import Blam.Map.Reader.Type;
import Blam.MapModel.Layer;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Layer;
import Blam.Runtime.Player.Type;
import Perception.Layer;
import Views.Layer;
import Bridge.Codec.Layer;
import Tests.Support.Blam;
import Tests.Support.Log;
import std;

// The pure chain of M5b (D198, D200) over a tag index and Snapshots made by hand, shared by the tests that run the pipeline.

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Team::Type::Team;
    using ::Blam::Layout::Snapshot::Type::RawObject;
    using ::Blam::Layout::Snapshot::Type::RegionId;
    using ::Blam::Layout::Snapshot::Type::RegionSize;
    using ::Blam::Layout::Snapshot::Type::Snapshot;
    using ::Blam::MapModel::Definitions::Type::Object::ObjectKind;
    using ::Blam::Runtime::Player::Type::Alive::ConnectionState;
    using Tests::Support::Blam::SyntheticTag;
    using Tests::Support::Blam::Fill;
    using Tests::Support::Blam::MakeHandle;
    using Tests::Support::Blam::Put;
    using Tests::Support::Blam::CaptureRegion;

    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_NoTick;
    using Common::Engine::Type::k_MaxLocalUsers;
    using Common::Engine::Type::k_MaxPlayers;
    using ::Blam::Map::Reader::Type::Magic::Tag::k_Bipd;

    namespace ObjeOffset = ::Blam::Layout::Object::Type::Obje::Offset;
    namespace UnitOffset = ::Blam::Layout::Object::Type::Unit::Offset;
    namespace PlayerOffset = ::Blam::Layout::Player::Type::Offset;
    namespace PlayerMapping = ::Blam::Layout::Player::Type::Mapping;

    using ::Blam::Layout::Player::Type::Size::k_Base;

    // Bytes of every hand made datum: more than the last field of a biped.
    constexpr std::size_t k_DatumBytes{ 0x1000 };

    const std::array<std::uint32_t, 1> k_Groups{ k_Bipd };

    constexpr std::string_view k_SpartanName{ "objects\\characters\\spartans\\spartans" };

    const std::array<SyntheticTag, 1> k_Tags{ {
        { .Group = 0, .Salt = 0xE000, .Name = k_SpartanName },
    } };

    const std::uint32_t k_SpartanTag{ MakeHandle(0xE000, 0) };
}

export namespace Tests::Support::Pipeline
{
    // Handles of the biped and the player that WithBipedAndPlayer builds.
    inline const std::uint32_t k_BipedHandle{ MakeHandle(0xEC01, 3) };
    inline const std::uint32_t k_PlayerHandle{ MakeHandle(0xE5A1, 2) };

    // return: A biped of the spartan tag whose four damage slots are empty, at (10, 20, 30) looking along +Y with +Z up.
    auto MakeBiped() -> RawObject
    {
        RawObject biped{};
        biped.Handle = k_BipedHandle;
        biped.Address = 0x7FF100000000 + (static_cast<std::uintptr_t>(k_BipedHandle & 0xFFFF) * k_DatumBytes);
        biped.KindId = static_cast<std::uint8_t>(ObjectKind::Biped);
        biped.Bytes.assign(k_DatumBytes, 0);

        Put(biped.Bytes, ObjeOffset::k_TagHandle, k_SpartanTag);
        Put(biped.Bytes, ObjeOffset::k_ParentHandle, k_NoHandle);
        Put(biped.Bytes, ObjeOffset::k_VariantPlacementIndex, std::uint16_t{ 0xFFFF });
        Put(biped.Bytes, ObjeOffset::k_Origin, Vec3{ 10.0f, 20.0f, 30.0f });
        Put(biped.Bytes, ObjeOffset::k_Forward, Vec3{ 0.0f, 1.0f, 0.0f });
        Put(biped.Bytes, ObjeOffset::k_Up, Vec3{ 0.0f, 0.0f, 1.0f });

        const std::size_t slotStride{ UnitOffset::k_Damage1Tick - UnitOffset::k_Damage0Tick };
        for (std::size_t slot{ 0 }; slot < 4; ++slot)
        {
            Put(biped.Bytes, UnitOffset::k_Damage0Tick + (slot * slotStride), k_NoTick);
        }

        return biped;
    }

    // return: A snapshot of a running game with the object table found, the biped and a connected player that owns it.
    auto WithBipedAndPlayer() -> Snapshot
    {
        Snapshot snapshot{};
        snapshot.HasThreadContext = true;
        snapshot.HasObjectTable = true;
        snapshot.RawObjects.emplace(k_BipedHandle, MakeBiped());

        std::vector<std::uint8_t>& players{ CaptureRegion(snapshot, RegionId::PlayerArray, RegionSize(RegionId::PlayerArray)) };
        const std::size_t entry{ 2 * k_Base };
        Put(players, entry + PlayerOffset::k_Handle, std::uint16_t{ 0xE5A1 });
        Put(players, entry + PlayerOffset::k_ConnectionState, ConnectionState::Connected);
        Put(players, entry + PlayerOffset::k_Team, Team::Blue);
        Put(players, entry + PlayerOffset::k_AliveBipedHandle, k_BipedHandle);
        Put(players, entry + PlayerOffset::k_CurrentBipedHandle, k_BipedHandle);
        Put(players, entry + PlayerOffset::k_DeadBipedHandle, k_NoHandle);

        // The player mapping with no player bound to any input user nor output view.
        std::vector<std::uint8_t>& mapping{ CaptureRegion(snapshot, RegionId::PlayerMapping, RegionSize(RegionId::PlayerMapping)) };
        for (std::size_t index{ 0 }; index < k_MaxPlayers; ++index)
        {
            Put(mapping, PlayerMapping::k_PlayerInputUser + (index * PlayerMapping::k_EntryStride), std::int32_t{ -1 });
        }
        for (std::size_t view{ 0 }; view < k_MaxLocalUsers; ++view)
        {
            Put(mapping, PlayerMapping::k_OutputUserPlayer + (view * PlayerMapping::k_EntryStride), k_NoHandle);
        }

        return snapshot;
    }

    // Writes the tag index of the spartan and marks the map as loaded.
    auto FillSyntheticTags(::Blam::Map::Layer& map) -> void
    {
        Fill(map.m_TagIndexStore, k_Groups, k_Tags);
        map.m_FileStore.SetLoaded(true);
    }

    // The seven layers of the pure chain over one Log (D197).
    class Chain
    {
    private:
        using Log = Common::Log::System::Log;
        using CollectingSink = Tests::Support::Log::CollectingSink;

    public:
        Chain() = default;
        ~Chain() = default;

        Chain(const Chain&) = delete;
        auto operator=(const Chain&) -> Chain& = delete;

        CollectingSink m_Sink{};
        Log m_Log{ m_Sink };
        ::Blam::Map::Layer m_Map{ m_Log };
        ::Blam::MapModel::Layer m_Resolved{ m_Log, m_Map };
        ::Blam::Runtime::Layer m_Tables{ m_Log, m_Resolved };
        Perception::Layer m_Environment{ m_Log, m_Resolved, m_Tables };
        Views::Layer m_Egocentric{ m_Log, m_Resolved, m_Tables, m_Environment };
        Bridge::Codec::Layer m_Export{ m_Resolved, m_Tables, m_Environment, m_Egocentric };
    };
}