#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Log.Type;
import Common.Log.System;
import Common.Math.Type;
import Common.Team.Type;
import Blam.Layout.Object.Type;
import Blam.Layout.Player.Type;
import Blam.Layout.Interaction.Type;
import Blam.Layout.Game.Type;
import Blam.Layout.Snapshot.Type;
import Blam.Map.Layer;
import Blam.Map.Reader.Type;
import Blam.MapModel.Layer;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Layer;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Player.Type;
import Blam.Runtime.Interaction.Type;
import Blam.Runtime.Game.Type;
import Tests.Support.Blam;
import Tests.Support.Log;
import std;

// Blam.Runtime (M5a, D189, D190, D197): the tables of the live objects, players, interactions and game, built from Snapshots made by
// hand and a tag index written by hand. No .map and no MCC. The expected values are what the code did on 2026-10-25 (M4b).

namespace
{
    using Common::Log::Type::LogLevel;
    using Common::Log::Type::LogMessage;
    using Common::Log::System::Log;
    using Common::Math::Type::Vec3;
    using Common::Team::Type::Team;
    using Blam::Layout::Snapshot::Type::RawObject;
    using Blam::Layout::Snapshot::Type::RegionId;
    using Blam::Layout::Snapshot::Type::RegionSize;
    using Blam::Layout::Snapshot::Type::Snapshot;
    using Blam::MapModel::Definitions::Type::Object::ObjectKind;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::Biped::Biped;
    using Blam::Runtime::Object::Type::Weapon::Weapon;
    using Blam::Runtime::Object::Type::Vehicle::Vehicle;
    using Blam::Runtime::Object::Type::Crate::Crate;
    using Blam::Runtime::Object::Type::Scenery::Scenery;
    using Blam::Runtime::Object::Type::BoneMatrix::BoneMatrix;
    using Blam::Runtime::Object::Type::BoneMatrix::BoneMatrixTable;
    using Blam::Runtime::Object::Type::DamageSection::DamageSectionTable;
    using Blam::Runtime::Player::Type::Alive::AlivePlayer;
    using Blam::Runtime::Player::Type::Alive::ConnectionState;
    using Blam::Runtime::Interaction::Type::Alive::InteractionKind;
    using Tests::Support::Blam::SyntheticTag;
    using Tests::Support::Blam::Fill;
    using Tests::Support::Blam::MakeHandle;
    using Tests::Support::Blam::Put;
    using Tests::Support::Blam::PutText;
    using Tests::Support::Blam::CaptureRegion;
    using Tests::Support::Log::CollectingSink;

    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_NoTick;
    using Common::Engine::Type::k_MaxLocalUsers;
    using Common::Engine::Type::k_MaxPlayers;
    using Blam::Map::Reader::Type::Magic::Tag::k_Bipd;
    using Blam::Map::Reader::Type::Magic::Tag::k_Weap;

    namespace ObjeOffset = Blam::Layout::Object::Type::Obje::Offset;
    namespace UnitOffset = Blam::Layout::Object::Type::Unit::Offset;
    namespace WeapOffset = Blam::Layout::Object::Type::Weap::Offset;
    namespace PlayerOffset = Blam::Layout::Player::Type::Offset;
    namespace PlayerMapping = Blam::Layout::Player::Type::Mapping;
    namespace InteractionOffset = Blam::Layout::Interaction::Type::Offset;
    namespace GameOffset = Blam::Layout::Game::Type::Offset;

    using Blam::Layout::Player::Type::Size::k_Base;
    using Blam::Layout::Object::Type::Constant::k_TimeGlobalsEngineTickOffset;
    using Blam::Layout::Object::Type::Constant::k_EventGlobalsSequenceOffset;

    // Bytes of every hand made datum: more than the last field of a biped.
    constexpr std::size_t k_DatumBytes{ 0x1000 };

    const std::array<std::uint32_t, 2> k_Groups{ k_Bipd, k_Weap };

    constexpr std::string_view k_SpartanName{ "objects\\characters\\spartans\\spartans" };
    constexpr std::string_view k_RifleName{ "objects\\weapons\\rifle\\assault_rifle\\assault_rifle" };

    const std::array<SyntheticTag, 2> k_Tags{ {
        { .Group = 0, .Salt = 0xE000, .Name = k_SpartanName },
        { .Group = 1, .Salt = 0xE001, .Name = k_RifleName },
    } };

    const std::uint32_t k_SpartanTag{ MakeHandle(0xE000, 0) };
    const std::uint32_t k_RifleTag{ MakeHandle(0xE001, 1) };

    const std::uint32_t k_BipedHandle{ MakeHandle(0xEC01, 3) };
    const std::uint32_t k_WeaponHandle{ MakeHandle(0xEC02, 4) };

    template <typename T>
    concept HasAddress = requires(const T& value) { value.Address; };
    template <typename T>
    concept HasBaseAddress = requires(const T& value) { value.BaseAddress; };
    template <typename T>
    concept HasBase = requires(const T& value) { value.Base; };

    // return: A live object of the kind with the tag, zero filled, without parent nor placement.
    auto MakeObject(std::uint32_t handle, ObjectKind kind, std::uint32_t tagHandle) -> RawObject
    {
        RawObject object{};
        object.Handle = handle;
        object.Address = 0x7FF100000000 + (static_cast<std::uintptr_t>(handle & 0xFFFF) * k_DatumBytes);
        object.KindId = static_cast<std::uint8_t>(kind);
        object.Bytes.assign(k_DatumBytes, 0);

        Put(object.Bytes, ObjeOffset::k_TagHandle, tagHandle);
        Put(object.Bytes, ObjeOffset::k_ParentHandle, k_NoHandle);
        Put(object.Bytes, ObjeOffset::k_VariantPlacementIndex, std::uint16_t{ 0xFFFF });

        return object;
    }

    // return: A biped whose four damage slots are empty, at (10, 20, 30) looking along +Y with +Z up.
    auto MakeBiped() -> RawObject
    {
        RawObject biped{ MakeObject(k_BipedHandle, ObjectKind::Biped, k_SpartanTag) };

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

    // return: A snapshot with the object table found and these objects.
    auto WithObjects(std::initializer_list<RawObject> objects) -> Snapshot
    {
        Snapshot snapshot{};
        snapshot.HasThreadContext = true;
        snapshot.HasObjectTable = true;
        for (const RawObject& object : objects)
        {
            snapshot.RawObjects.emplace(object.Handle, object);
        }
        return snapshot;
    }

    // Writes a player in the slot of the player array: salt, connection state and the rest zero.
    auto PutPlayer(std::vector<std::uint8_t>& players, std::size_t index, std::uint16_t salt, ConnectionState state) -> std::size_t
    {
        const std::size_t entry{ index * k_Base };
        Put(players, entry + PlayerOffset::k_Handle, salt);
        Put(players, entry + PlayerOffset::k_ConnectionState, state);
        return entry;
    }

    // Captures the player mapping with no player bound to any input user nor output view.
    auto CaptureEmptyMapping(Snapshot& snapshot) -> std::vector<std::uint8_t>&
    {
        std::vector<std::uint8_t>& mapping{ CaptureRegion(snapshot, RegionId::PlayerMapping, RegionSize(RegionId::PlayerMapping)) };
        for (std::size_t index{ 0 }; index < k_MaxPlayers; ++index)
        {
            Put(mapping, PlayerMapping::k_PlayerInputUser + (index * PlayerMapping::k_EntryStride), std::int32_t{ -1 });
        }
        for (std::size_t view{ 0 }; view < k_MaxLocalUsers; ++view)
        {
            Put(mapping, PlayerMapping::k_OutputUserPlayer + (view * PlayerMapping::k_EntryStride), k_NoHandle);
        }
        return mapping;
    }

    // An event of the ring as the engine writes it.
    struct RingEvent
    {
        std::uint32_t Sequence{};
        std::uint32_t Key{};
        std::uint32_t GameTick{};
        std::uint32_t CausePlayer{};
        std::uint32_t EffectObject{};
        std::uint8_t Modifier{};
    };

    auto PutEvent(std::vector<std::uint8_t>& events, const RingEvent& event, std::uint32_t storedSequence) -> void
    {
        const std::size_t entry{ GameOffset::k_EventRing + ((event.Sequence % GameOffset::k_EventRingSlots) * GameOffset::k_EventEntrySize) };
        Put(events, entry + GameOffset::k_EntryKey, event.Key);
        Put(events, entry + GameOffset::k_EntrySequence, storedSequence);
        Put(events, entry + GameOffset::k_EntryGameTick, event.GameTick);
        Put(events, entry + GameOffset::k_EntryData + GameOffset::k_DataCausePlayer, event.CausePlayer);
        Put(events, entry + GameOffset::k_EntryData + GameOffset::k_DataEffectObject, event.EffectObject);
        Put(events, entry + GameOffset::k_EntryData + GameOffset::k_DataModifier, event.Modifier);
    }

    // A snapshot of a running game with the event ring holding the sequences first..last (last is the newest).
    auto WithEvents(std::uint32_t first, std::uint32_t last) -> Snapshot
    {
        Snapshot snapshot{};
        snapshot.HasThreadContext = true;
        CaptureRegion(snapshot, RegionId::TimeGlobals, RegionSize(RegionId::TimeGlobals));
        CaptureRegion(snapshot, RegionId::GameEngineGlobals, RegionSize(RegionId::GameEngineGlobals));

        std::vector<std::uint8_t>& events{ CaptureRegion(snapshot, RegionId::EventGlobals, RegionSize(RegionId::EventGlobals)) };
        Put(events, k_EventGlobalsSequenceOffset, last);
        for (std::uint32_t sequence{ first }; sequence <= last && sequence != 0; ++sequence)
        {
            PutEvent(events, RingEvent{ .Sequence = sequence, .Key = 0x100 + sequence, .GameTick = 1000 + sequence }, sequence);
        }

        return snapshot;
    }

    // Blam::Runtime::Layer over a real Blam::Map::Layer and Blam::MapModel::Layer whose tag index is written by hand (D197).
    class RuntimeTests : public ::testing::Test
    {
    protected:
        auto SetUp() -> void override
        {
            Fill(m_Map.m_TagIndexStore, k_Groups, k_Tags);
            m_Map.m_FileStore.SetLoaded(true);
        }

        auto CountOf(LogLevel level, std::string_view tag, std::string_view message) const -> std::size_t
        {
            const LogMessage expected{ .Level = level, .Tag = std::string{ tag }, .Message = std::string{ message } };
            return static_cast<std::size_t>(std::ranges::count(m_Sink.m_Messages, expected));
        }

        auto UpdateObjects(const Snapshot& snapshot) -> std::shared_ptr<const Blam::Runtime::Object::Type::Alive::ObjectTable>
        {
            m_Tables.m_ObjectService.UpdateObjectTable(snapshot);
            return m_Tables.m_ObjectStore.Acquire();
        }

        auto UpdatePlayers(const Snapshot& snapshot) -> std::shared_ptr<const Blam::Runtime::Player::Type::Alive::PlayerTable>
        {
            m_Tables.m_PlayerService.UpdatePlayerTable(snapshot);
            return m_Tables.m_PlayerStore.Acquire();
        }

        auto UpdateGame(const Snapshot& snapshot) -> std::shared_ptr<const Blam::Runtime::Game::Type::Alive::AliveGame>
        {
            m_Tables.m_GameService.Update(snapshot);
            return m_Tables.m_GameStore.Acquire();
        }

        CollectingSink m_Sink{};
        Log m_Log{ m_Sink };
        Blam::Map::Layer m_Map{ m_Log };
        Blam::MapModel::Layer m_Resolved{ m_Log, m_Map };
        Blam::Runtime::Layer m_Tables{ m_Log, m_Resolved };
    };
}

// --- D190: no address of the process in the tables ---

TEST(RuntimeTypesTest, TablesHoldNoProcessAddress)
{
    EXPECT_FALSE(HasAddress<AliveObject>);
    EXPECT_FALSE(HasAddress<AlivePlayer>);
    EXPECT_FALSE(HasBaseAddress<BoneMatrixTable>);
    EXPECT_FALSE(HasBaseAddress<DamageSectionTable>);
    EXPECT_FALSE(HasBase<Vehicle>);
    EXPECT_FALSE(HasBase<Crate>);
    EXPECT_FALSE(HasBase<Scenery>);
}

// --- Objects ---

TEST_F(RuntimeTests, ObjectsNeedTheObjectTable)
{
    Snapshot snapshot{ WithObjects({ MakeBiped() }) };
    snapshot.HasObjectTable = false;

    EXPECT_EQ(this->UpdateObjects(snapshot), nullptr);
}

TEST_F(RuntimeTests, ObjectWithAResolvedTagIsAdded)
{
    const auto table{ this->UpdateObjects(WithObjects({ MakeBiped(), MakeObject(k_WeaponHandle, ObjectKind::Weapon, k_RifleTag) })) };
    ASSERT_NE(table, nullptr);
    ASSERT_EQ(table->size(), 2u);

    const AliveObject& biped{ table->at(k_BipedHandle) };
    EXPECT_EQ(biped.Handle, k_BipedHandle);
    EXPECT_EQ(biped.DatumIndex, k_SpartanTag);
    EXPECT_EQ(biped.FourCC, "bipd");
    EXPECT_EQ(biped.TagName, k_SpartanName);
    EXPECT_EQ(biped.Profile.ObjectKind, ObjectKind::Biped);
    EXPECT_FALSE(biped.Profile.HasColl);
    EXPECT_NE(std::get_if<Biped>(&biped.Specific), nullptr);

    const AliveObject& weapon{ table->at(k_WeaponHandle) };
    EXPECT_EQ(weapon.FourCC, "weap");
    EXPECT_EQ(weapon.TagName, k_RifleName);
    EXPECT_EQ(weapon.Profile.ObjectKind, ObjectKind::Weapon);
    EXPECT_NE(std::get_if<Weapon>(&weapon.Specific), nullptr);

    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 0u);
}

TEST_F(RuntimeTests, ObjectWithAnUnknownTagIsLeftOut)
{
    // Salt of the rifle with the index of the spartan: the resolver rejects it with a warning.
    const auto table{ this->UpdateObjects(WithObjects({ MakeObject(k_WeaponHandle, ObjectKind::Weapon, MakeHandle(0xE001, 0)) })) };
    ASSERT_NE(table, nullptr);
    EXPECT_TRUE(table->empty());
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 1u);
}

TEST_F(RuntimeTests, BipedFieldsComeFromTheDatum)
{
    RawObject raw{ MakeBiped() };
    Put(raw.Bytes, ObjeOffset::k_LinearVelocity, Vec3{ 1.0f, 2.0f, 3.0f });
    Put(raw.Bytes, UnitOffset::k_TicksAlive, std::uint32_t{ 1234 });
    Put(raw.Bytes, UnitOffset::k_CurrentAim, Vec3{ 0.6f, 0.8f, 0.0f });
    Put(raw.Bytes, UnitOffset::k_PrimaryWeaponHandle, k_WeaponHandle);
    Put(raw.Bytes, UnitOffset::k_SecondaryWeaponHandle, MakeHandle(0xEC05, 7));
    Put(raw.Bytes, UnitOffset::k_ActiveWeaponSlot, std::uint8_t{ 1 });
    Put(raw.Bytes, UnitOffset::k_FragGrenadeCount, std::uint8_t{ 2 });
    Put(raw.Bytes, UnitOffset::k_PlasmaGrenadeCount, std::uint8_t{ 1 });
    Put(raw.Bytes, UnitOffset::k_Damage0Tick, std::uint32_t{ 900 });
    Put(raw.Bytes, UnitOffset::k_Damage0Health, 0.25f);
    Put(raw.Bytes, UnitOffset::k_Damage0Shield, 0.5f);
    Put(raw.Bytes, UnitOffset::k_Damage0BipedHandle, MakeHandle(0xEC06, 8));
    Put(raw.Bytes, UnitOffset::k_Damage0PlayerHandle, MakeHandle(0xE001, 1));
    Put(raw.Bytes, UnitOffset::k_Damage0ReportingType, std::uint16_t{ 3 });
    Put(raw.Bytes, UnitOffset::k_LastDamageTick, std::uint32_t{ 900 });
    Put(raw.Bytes, ObjeOffset::k_LastUpdateTick, std::uint32_t{ 960 });

    const auto table{ this->UpdateObjects(WithObjects({ raw })) };
    ASSERT_NE(table, nullptr);
    const AliveObject& object{ table->at(k_BipedHandle) };

    EXPECT_FLOAT_EQ(object.Origin.X, 10.0f);
    EXPECT_FLOAT_EQ(object.Origin.Z, 30.0f);
    EXPECT_FLOAT_EQ(object.LinearVelocity.Y, 2.0f);

    const Biped* biped{ std::get_if<Biped>(&object.Specific) };
    ASSERT_NE(biped, nullptr);
    EXPECT_EQ(biped->TicksAlive, 1234u);
    EXPECT_FLOAT_EQ(biped->Aim.Y, 0.8f);
    EXPECT_EQ(biped->PrimaryWeaponHandle, k_WeaponHandle);
    EXPECT_EQ(biped->SecondaryWeaponHandle, MakeHandle(0xEC05, 7));
    // Slot 1 is the secondary weapon.
    EXPECT_EQ(biped->EquippedWeaponHandle, MakeHandle(0xEC05, 7));
    EXPECT_EQ(biped->FragGrenades, 2);
    EXPECT_EQ(biped->PlasmaGrenades, 1);

    ASSERT_EQ(biped->Damages.size(), 1u);
    EXPECT_EQ(biped->Damages[0].Tick, 900u);
    EXPECT_FLOAT_EQ(biped->Damages[0].Health, 0.25f);
    EXPECT_FLOAT_EQ(biped->Damages[0].Shield, 0.5f);
    EXPECT_EQ(biped->Damages[0].BipedHandle, MakeHandle(0xEC06, 8));
    EXPECT_EQ(biped->Damages[0].PlayerHandle, MakeHandle(0xE001, 1));
    EXPECT_EQ(static_cast<std::uint16_t>(biped->Damages[0].ReportingType), 3);

    EXPECT_EQ(biped->LastDamageTick, 900u);
    ASSERT_TRUE(biped->TicksSinceDamage.has_value());
    EXPECT_EQ(*biped->TicksSinceDamage, 60u);
}

TEST_F(RuntimeTests, WeaponInAVehicleSlotIsNotEquipped)
{
    RawObject raw{ MakeBiped() };
    Put(raw.Bytes, UnitOffset::k_PrimaryWeaponHandle, k_WeaponHandle);
    // note: The engine writes 0xFF in a vehicle, where it hides the weapons.
    Put(raw.Bytes, UnitOffset::k_ActiveWeaponSlot, std::uint8_t{ 0xFF });

    const auto table{ this->UpdateObjects(WithObjects({ raw })) };
    ASSERT_NE(table, nullptr);
    const Biped* biped{ std::get_if<Biped>(&table->at(k_BipedHandle).Specific) };
    ASSERT_NE(biped, nullptr);
    EXPECT_EQ(biped->EquippedWeaponHandle, k_NoTick);
}

TEST_F(RuntimeTests, ParentedObjectIsPlacedThroughItsParent)
{
    RawObject weapon{ MakeObject(k_WeaponHandle, ObjectKind::Weapon, k_RifleTag) };
    Put(weapon.Bytes, ObjeOffset::k_ParentHandle, k_BipedHandle);
    Put(weapon.Bytes, ObjeOffset::k_Origin, Vec3{ 1.0f, 2.0f, 3.0f });

    const auto table{ this->UpdateObjects(WithObjects({ MakeBiped(), weapon })) };
    ASSERT_NE(table, nullptr);

    // The parent looks along +Y with +Z up, so its left is -X: local (1, 2, 3) is world (10 - 2, 20 + 1, 30 + 3).
    const AliveObject& child{ table->at(k_WeaponHandle) };
    EXPECT_EQ(child.ParentHandle, k_BipedHandle);
    EXPECT_FLOAT_EQ(child.Origin.X, 1.0f);
    EXPECT_FLOAT_EQ(child.WorldOrigin.X, 8.0f);
    EXPECT_FLOAT_EQ(child.WorldOrigin.Y, 21.0f);
    EXPECT_FLOAT_EQ(child.WorldOrigin.Z, 33.0f);

    const AliveObject& parent{ table->at(k_BipedHandle) };
    EXPECT_FLOAT_EQ(parent.WorldOrigin.X, 10.0f);
    EXPECT_FLOAT_EQ(parent.WorldOrigin.Y, 20.0f);
    EXPECT_FLOAT_EQ(parent.WorldOrigin.Z, 30.0f);
}

TEST_F(RuntimeTests, WeaponFieldsComeFromTheDatum)
{
    RawObject raw{ MakeObject(k_WeaponHandle, ObjectKind::Weapon, k_RifleTag) };
    Put(raw.Bytes, WeapOffset::k_TotalHeat, 0.5f);
    Put(raw.Bytes, WeapOffset::k_TotalEnergy, 0.25f);
    Put(raw.Bytes, WeapOffset::k_Magazine0TotalAmmo, std::uint16_t{ 96 });
    Put(raw.Bytes, WeapOffset::k_Magazine0CurrentAmmo, std::uint16_t{ 32 });
    Put(raw.Bytes, WeapOffset::k_IsTracking, std::uint8_t{ 1 });
    Put(raw.Bytes, WeapOffset::k_TrackedBipedHandle, k_BipedHandle);

    const auto table{ this->UpdateObjects(WithObjects({ raw })) };
    ASSERT_NE(table, nullptr);
    const Weapon* weapon{ std::get_if<Weapon>(&table->at(k_WeaponHandle).Specific) };
    ASSERT_NE(weapon, nullptr);

    EXPECT_FLOAT_EQ(weapon->TotalHeat, 0.5f);
    // The engine stores the energy used: the table holds what is left.
    EXPECT_FLOAT_EQ(weapon->TotalEnergy, 0.75f);
    EXPECT_EQ(weapon->TotalAmmo, 96);
    EXPECT_EQ(weapon->CurrentAmmo, 32);
    EXPECT_EQ(weapon->IsTracking, 1);
    EXPECT_EQ(weapon->TrackedBipedHandle, k_BipedHandle);
}

TEST_F(RuntimeTests, BoneMatricesAreCopiedForThisTickOnly)
{
    constexpr std::uint16_t k_BonesOffset{ 0xE80 };

    RawObject raw{ MakeBiped() };
    Put(raw.Bytes, ObjeOffset::k_NodeMatricesSize, static_cast<std::uint16_t>(2 * sizeof(BoneMatrix)));
    Put(raw.Bytes, ObjeOffset::k_NodeMatricesOffset, k_BonesOffset);

    BoneMatrix first{};
    first.Scale = 1.0f;
    first.Translation = { 1.0f, 2.0f, 3.0f };
    BoneMatrix second{};
    second.Scale = 2.0f;
    Put(raw.Bytes, k_BonesOffset, first);
    Put(raw.Bytes, k_BonesOffset + sizeof(BoneMatrix), second);

    this->UpdateObjects(WithObjects({ raw }));
    const BoneMatrixTable* bones{ m_Tables.m_BoneMatricesStore.Get(k_BipedHandle) };
    ASSERT_NE(bones, nullptr);
    ASSERT_EQ(bones->Matrices.size(), 2u);
    EXPECT_FLOAT_EQ(bones->Matrices[0].Translation[2], 3.0f);
    EXPECT_FLOAT_EQ(bones->Matrices[1].Scale, 2.0f);

    // The next tick without bones leaves nothing behind.
    this->UpdateObjects(WithObjects({ MakeBiped() }));
    EXPECT_EQ(m_Tables.m_BoneMatricesStore.Get(k_BipedHandle), nullptr);
}

TEST_F(RuntimeTests, ObjectThatLeavesTheSnapshotIsRemoved)
{
    ASSERT_EQ(this->UpdateObjects(WithObjects({ MakeBiped(), MakeObject(k_WeaponHandle, ObjectKind::Weapon, k_RifleTag) }))->size(), 2u);

    const auto table{ this->UpdateObjects(WithObjects({ MakeBiped() })) };
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(table->size(), 1u);
    EXPECT_TRUE(table->contains(k_BipedHandle));
}

// --- Players ---

TEST_F(RuntimeTests, PlayersNeedThePlayerArray)
{
    const auto table{ this->UpdatePlayers(Snapshot{}) };
    ASSERT_NE(table, nullptr);
    EXPECT_TRUE(table->empty());
}

TEST_F(RuntimeTests, ConnectedPlayerIsAdded)
{
    Snapshot snapshot{};
    std::vector<std::uint8_t>& players{ CaptureRegion(snapshot, RegionId::PlayerArray, RegionSize(RegionId::PlayerArray)) };
    std::vector<std::uint8_t>& mapping{ CaptureEmptyMapping(snapshot) };

    const std::size_t entry{ PutPlayer(players, 2, 0xE5A1, ConnectionState::Connected) };
    PutText(players, entry + PlayerOffset::k_GamerTag, u"Julián");
    PutText(players, entry + PlayerOffset::k_Tag, u"ART");
    Put(players, entry + PlayerOffset::k_Team, Team::Blue);
    Put(players, entry + PlayerOffset::k_AliveBipedHandle, k_BipedHandle);
    Put(players, entry + PlayerOffset::k_CurrentBipedHandle, k_BipedHandle);
    Put(players, entry + PlayerOffset::k_DeadBipedHandle, k_NoHandle);
    Put(players, entry + PlayerOffset::k_PrimaryWeaponHandle, k_WeaponHandle);
    Put(players, entry + PlayerOffset::k_CameraPosition, Vec3{ 1.0f, 2.0f, 3.0f });
    Put(mapping, PlayerMapping::k_PlayerInputUser + (2 * PlayerMapping::k_EntryStride), std::int32_t{ 1 });

    const auto table{ this->UpdatePlayers(snapshot) };
    ASSERT_NE(table, nullptr);
    ASSERT_EQ(table->size(), 1u);

    const std::uint32_t handle{ MakeHandle(0xE5A1, 2) };
    ASSERT_TRUE(table->contains(handle));
    const AlivePlayer& player{ table->at(handle) };

    EXPECT_EQ(player.Handle, handle);
    EXPECT_EQ(player.ConnectionState, ConnectionState::Connected);
    EXPECT_EQ(player.Gamertag, "Juli\xC3\xA1n");
    EXPECT_EQ(player.Tag, "ART");
    EXPECT_EQ(player.Team, Team::Blue);
    EXPECT_EQ(player.AliveBipedHandle, k_BipedHandle);
    EXPECT_EQ(player.CurrentBipedHandle, k_BipedHandle);
    EXPECT_EQ(player.DeadBipedHandle, k_NoHandle);
    EXPECT_EQ(player.PrimaryWeaponHandle, k_WeaponHandle);
    EXPECT_FLOAT_EQ(player.CameraPosition.Z, 3.0f);
    ASSERT_TRUE(player.LocalIndex.has_value());
    EXPECT_EQ(*player.LocalIndex, 1);
}

TEST_F(RuntimeTests, PlayerWithAnUnknownConnectionStateIsLeftOut)
{
    Snapshot snapshot{};
    std::vector<std::uint8_t>& players{ CaptureRegion(snapshot, RegionId::PlayerArray, RegionSize(RegionId::PlayerArray)) };
    PutPlayer(players, 0, 0xE5A1, static_cast<ConnectionState>(0x04));

    const auto table{ this->UpdatePlayers(snapshot) };
    ASSERT_NE(table, nullptr);
    EXPECT_TRUE(table->empty());
}

TEST_F(RuntimeTests, SlotWithANewSaltReplacesThePlayer)
{
    Snapshot before{};
    PutPlayer(CaptureRegion(before, RegionId::PlayerArray, RegionSize(RegionId::PlayerArray)), 5, 0xE5A1, ConnectionState::Connected);
    ASSERT_TRUE(this->UpdatePlayers(before)->contains(MakeHandle(0xE5A1, 5)));

    Snapshot after{};
    PutPlayer(CaptureRegion(after, RegionId::PlayerArray, RegionSize(RegionId::PlayerArray)), 5, 0xE5A2, ConnectionState::Connected);
    const auto table{ this->UpdatePlayers(after) };
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(table->size(), 1u);
    EXPECT_TRUE(table->contains(MakeHandle(0xE5A2, 5)));
}

TEST_F(RuntimeTests, LocalIndexNeedsAValidInputUser)
{
    Snapshot snapshot{};
    std::vector<std::uint8_t>& players{ CaptureRegion(snapshot, RegionId::PlayerArray, RegionSize(RegionId::PlayerArray)) };
    std::vector<std::uint8_t>& mapping{ CaptureEmptyMapping(snapshot) };
    PutPlayer(players, 0, 0xE5A1, ConnectionState::Connected);
    PutPlayer(players, 1, 0xE5A2, ConnectionState::Connected);
    Put(mapping, PlayerMapping::k_PlayerInputUser + (1 * PlayerMapping::k_EntryStride), static_cast<std::int32_t>(k_MaxLocalUsers));

    const auto table{ this->UpdatePlayers(snapshot) };
    ASSERT_NE(table, nullptr);
    EXPECT_FALSE(table->at(MakeHandle(0xE5A1, 0)).LocalIndex.has_value());
    EXPECT_FALSE(table->at(MakeHandle(0xE5A2, 1)).LocalIndex.has_value());

    // Without the mapping no player is local.
    Snapshot unmapped{};
    PutPlayer(CaptureRegion(unmapped, RegionId::PlayerArray, RegionSize(RegionId::PlayerArray)), 0, 0xE5A1, ConnectionState::Connected);
    EXPECT_FALSE(this->UpdatePlayers(unmapped)->at(MakeHandle(0xE5A1, 0)).LocalIndex.has_value());
}

// --- Interactions ---

TEST_F(RuntimeTests, InteractionsNeedBothRegions)
{
    Snapshot snapshot{};
    CaptureRegion(snapshot, RegionId::Interaction, RegionSize(RegionId::Interaction));

    m_Tables.m_InteractionService.UpdateInteractionTable(snapshot);
    const auto interactions{ m_Tables.m_InteractionStore.Acquire() };
    ASSERT_NE(interactions, nullptr);
    for (const auto& interaction : *interactions)
    {
        EXPECT_EQ(interaction.PlayerHandle, k_NoHandle);
        EXPECT_EQ(interaction.Kind, InteractionKind::None);
    }
}

TEST_F(RuntimeTests, EachViewGoesToTheLocalUserOfItsPlayer)
{
    const std::uint32_t playerHandle{ MakeHandle(0xE5A1, 3) };

    Snapshot snapshot{};
    std::vector<std::uint8_t>& mapping{ CaptureEmptyMapping(snapshot) };
    Put(mapping, PlayerMapping::k_OutputUserPlayer + (1 * PlayerMapping::k_EntryStride), playerHandle);
    Put(mapping, PlayerMapping::k_PlayerInputUser + (3 * PlayerMapping::k_EntryStride), std::int32_t{ 2 });

    std::vector<std::uint8_t>& views{ CaptureRegion(snapshot, RegionId::Interaction, RegionSize(RegionId::Interaction)) };
    const std::size_t view{ (1 * InteractionOffset::k_OutputUserStride) + InteractionOffset::k_Interaction };
    Put(views, view + InteractionOffset::k_Kind, InteractionKind::GrabWeapon);
    Put(views, view + InteractionOffset::k_TargetObjectHandle, k_WeaponHandle);
    Put(views, view + InteractionOffset::k_IsMeleeAvailable, std::uint8_t{ 0x0E });
    Put(views, view + InteractionOffset::k_AimHitLocalPosition, Vec3{ 0.5f, 0.0f, 1.0f });

    m_Tables.m_InteractionService.UpdateInteractionTable(snapshot);
    const auto interactions{ m_Tables.m_InteractionStore.Acquire() };
    ASSERT_NE(interactions, nullptr);

    const auto& local{ (*interactions)[2] };
    EXPECT_EQ(local.PlayerHandle, playerHandle);
    EXPECT_EQ(local.Kind, InteractionKind::GrabWeapon);
    EXPECT_EQ(local.TargetObjectHandle, k_WeaponHandle);
    EXPECT_EQ(local.IsMeleeAvailable, 0x0E);
    EXPECT_FLOAT_EQ(local.AimHitLocalPosition.Z, 1.0f);

    for (const std::size_t other : { 0u, 1u, 3u })
    {
        EXPECT_EQ((*interactions)[other].PlayerHandle, k_NoHandle) << "local user " << other;
    }
}

// --- Game ---

TEST_F(RuntimeTests, GameNeedsTheThreadContext)
{
    Snapshot snapshot{ WithEvents(1, 3) };
    snapshot.HasThreadContext = false;

    EXPECT_EQ(this->UpdateGame(snapshot), nullptr);
}

TEST_F(RuntimeTests, ClockPhaseAndScoresAndEveryPhaseChangeIsLogged)
{
    Snapshot snapshot{ WithEvents(0, 0) };
    Put(snapshot.Regions[static_cast<std::size_t>(RegionId::TimeGlobals)].Bytes, k_TimeGlobalsEngineTickOffset, std::uint32_t{ 4321 });
    std::vector<std::uint8_t>& engine{ snapshot.Regions[static_cast<std::size_t>(RegionId::GameEngineGlobals)].Bytes };
    Put(engine, GameOffset::k_GameEnginePhase, std::uint32_t{ 2 });
    Put(engine, GameOffset::k_GameEnginePlayerScores, std::int16_t{ 5 });
    Put(engine, GameOffset::k_GameEnginePlayerScores + ((k_MaxPlayers - 1) * GameOffset::k_PlayerScoreStride), std::int16_t{ -1 });

    const auto game{ this->UpdateGame(snapshot) };
    ASSERT_NE(game, nullptr);
    EXPECT_EQ(game->EngineTick, 4321u);
    EXPECT_EQ(game->Phase, 2u);
    EXPECT_EQ(game->PlayerScores[0], 5);
    EXPECT_EQ(game->PlayerScores[k_MaxPlayers - 1], -1);
    EXPECT_TRUE(game->Events.empty());

    this->UpdateGame(snapshot);
    EXPECT_EQ(this->CountOf(LogLevel::Info, "[GameService]", "Game engine phase 2 at engine tick 4321."), 1u);

    Put(engine, GameOffset::k_GameEnginePhase, std::uint32_t{ 4 });
    this->UpdateGame(snapshot);
    EXPECT_EQ(this->CountOf(LogLevel::Info, "[GameService]", "Game engine phase 4 at engine tick 4321."), 1u);
}

TEST_F(RuntimeTests, FirstReadTakesTheWholeRingWithoutCountingALoss)
{
    const std::uint32_t last{ GameOffset::k_EventRingSlots + 5 };

    const auto game{ this->UpdateGame(WithEvents(1, last)) };
    ASSERT_NE(game, nullptr);
    ASSERT_EQ(game->Events.size(), GameOffset::k_EventRingSlots);
    EXPECT_EQ(game->Events.front().Sequence, 6u);
    EXPECT_EQ(game->Events.back().Sequence, last);
    EXPECT_EQ(game->LastSequence, last);
    EXPECT_EQ(game->DroppedEvents, 0u);
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 0u);
}

TEST_F(RuntimeTests, LaterReadsTakeOnlyTheNewEvents)
{
    ASSERT_EQ(this->UpdateGame(WithEvents(1, 3))->Events.size(), 3u);

    Snapshot snapshot{ WithEvents(1, 5) };
    PutEvent(snapshot.Regions[static_cast<std::size_t>(RegionId::EventGlobals)].Bytes,
        RingEvent{ .Sequence = 5, .Key = 0x7, .GameTick = 2000, .CausePlayer = MakeHandle(0xE5A1, 2), .EffectObject = k_BipedHandle,
            .Modifier = 0x01 }, 5);

    const auto game{ this->UpdateGame(snapshot) };
    ASSERT_NE(game, nullptr);
    ASSERT_EQ(game->Events.size(), 2u);
    EXPECT_EQ(game->Events[0].Sequence, 4u);
    EXPECT_EQ(game->Events[0].Key, 0x104u);
    EXPECT_EQ(game->Events[0].GameTick, 1004u);

    const auto& event{ game->Events[1] };
    EXPECT_EQ(event.Sequence, 5u);
    EXPECT_EQ(event.Key, 0x7u);
    EXPECT_EQ(event.GameTick, 2000u);
    EXPECT_EQ(event.CausePlayer, MakeHandle(0xE5A1, 2));
    EXPECT_EQ(event.EffectObject, k_BipedHandle);
    EXPECT_EQ(event.Modifier, 0x01);

    // Nothing new: no events, the cursor stays.
    EXPECT_TRUE(this->UpdateGame(snapshot)->Events.empty());
}

TEST_F(RuntimeTests, OverwrittenEventsAreCountedAndWarnedOnce)
{
    this->UpdateGame(WithEvents(1, 3));

    const std::uint32_t last{ 3 + GameOffset::k_EventRingSlots + 4 };
    const auto game{ this->UpdateGame(WithEvents(1, last)) };
    ASSERT_NE(game, nullptr);
    EXPECT_EQ(game->Events.size(), GameOffset::k_EventRingSlots);
    EXPECT_EQ(game->Events.front().Sequence, 8u);
    EXPECT_EQ(game->DroppedEvents, 4u);
    EXPECT_EQ(this->CountOf(LogLevel::Warning, "[GameService]", "4 events of the ring were overwritten before they were read."), 1u);

    // The count is kept for the match.
    EXPECT_EQ(this->UpdateGame(WithEvents(1, last))->DroppedEvents, 4u);
}

TEST_F(RuntimeTests, ANewEngineStartsTheSequenceAgain)
{
    this->UpdateGame(WithEvents(1, 10));

    const auto game{ this->UpdateGame(WithEvents(1, 2)) };
    ASSERT_NE(game, nullptr);
    ASSERT_EQ(game->Events.size(), 2u);
    EXPECT_EQ(game->Events[0].Sequence, 1u);
    EXPECT_EQ(game->DroppedEvents, 0u);
}

TEST_F(RuntimeTests, EntryWithAnotherSequenceIsSkipped)
{
    Snapshot snapshot{ WithEvents(1, 3) };
    // The slot of 2 holds an entry of the previous turn of the ring.
    PutEvent(snapshot.Regions[static_cast<std::size_t>(RegionId::EventGlobals)].Bytes, RingEvent{ .Sequence = 2 },
        2 + GameOffset::k_EventRingSlots);

    const auto game{ this->UpdateGame(snapshot) };
    ASSERT_NE(game, nullptr);
    ASSERT_EQ(game->Events.size(), 2u);
    EXPECT_EQ(game->Events[0].Sequence, 1u);
    EXPECT_EQ(game->Events[1].Sequence, 3u);
    EXPECT_EQ(this->CountOf(LogLevel::Warning, "[GameService]",
        "1 entries of the event ring held another sequence (layout changed?)."), 1u);

    // The cursor still moved to the last sequence.
    EXPECT_TRUE(this->UpdateGame(snapshot)->Events.empty());
}

// --- Layer (D178: pure, the owner calls Cleanup) ---

TEST_F(RuntimeTests, CleanupForgetsEveryTable)
{
    Snapshot snapshot{ WithEvents(1, 3) };
    snapshot.HasObjectTable = true;
    snapshot.RawObjects.emplace(k_BipedHandle, MakeBiped());
    PutPlayer(CaptureRegion(snapshot, RegionId::PlayerArray, RegionSize(RegionId::PlayerArray)), 0, 0xE5A1, ConnectionState::Connected);
    CaptureEmptyMapping(snapshot);
    CaptureRegion(snapshot, RegionId::Interaction, RegionSize(RegionId::Interaction));

    m_Tables.m_ObjectService.UpdateObjectTable(snapshot);
    m_Tables.m_PlayerService.UpdatePlayerTable(snapshot);
    m_Tables.m_InteractionService.UpdateInteractionTable(snapshot);
    m_Tables.m_GameService.Update(snapshot);

    m_Tables.Cleanup();

    EXPECT_EQ(m_Tables.m_ObjectStore.Acquire(), nullptr);
    EXPECT_EQ(m_Tables.m_PlayerStore.Acquire(), nullptr);
    EXPECT_EQ(m_Tables.m_InteractionStore.Acquire(), nullptr);
    EXPECT_EQ(m_Tables.m_GameStore.Acquire(), nullptr);
    EXPECT_EQ(m_Tables.m_GameStore.GetEventCursor(), 0u);

    EXPECT_EQ(this->CountOf(LogLevel::Info, "[ObjectTableService]", "Cleanup completed."), 1u);
    EXPECT_EQ(this->CountOf(LogLevel::Info, "[PlayerTableService]", "Cleanup completed."), 1u);
    EXPECT_EQ(this->CountOf(LogLevel::Info, "[InteractionService]", "Cleanup completed."), 1u);

    // After a cleanup the same objects are discovered again (a new map).
    EXPECT_EQ(this->UpdateObjects(snapshot)->size(), 1u);
    EXPECT_EQ(this->UpdateGame(snapshot)->Events.size(), 3u);
}