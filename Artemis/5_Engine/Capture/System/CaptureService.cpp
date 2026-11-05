module Engine.Capture.System;

import Common.Engine.Type;
import Blam.Layout.Object.Type;
import std;

namespace
{
    using Blam::Layout::Snapshot::Type::RawObject;
    using Blam::Layout::Snapshot::Type::Region;
    using Blam::Layout::Snapshot::Type::RegionSize;

    using Common::Engine::Type::k_MaxObjects;
    using Common::Engine::Type::k_MaxPlayers;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextObjectDataOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextTelemetryDataOffset;
    using Blam::Layout::Object::Type::Constant::k_DataArrayCapacity;
    using Blam::Layout::Object::Type::Constant::k_EntrySize;
    using Blam::Layout::Object::Type::Constant::k_EntrySaltOffset;
    using Blam::Layout::Object::Type::Constant::k_EntryKindOffset;
    using Blam::Layout::Object::Type::Constant::k_EntryDatumSizeOffset;
    using Blam::Layout::Object::Type::Constant::k_EntryAddressOffset;
    using Blam::Layout::Object::Type::Constant::k_MinDatumSize;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextTimeGlobalsOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextGameEngineOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextEventGlobalsOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextGameModeOffset;
    using Blam::Layout::Object::Type::Constant::k_GameModeTypeOffset;
    using Blam::Layout::Object::Type::Constant::k_GameModeMultiplayer;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextMapVariantOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantAdjust;
    using Blam::Layout::Object::Type::Constant::k_MapVariantAlignMask;
    using Blam::Layout::Object::Type::Constant::k_PlacementsOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextMegaloObjectsOffset;
    using Blam::Layout::Object::Type::Constant::k_DataArrayData;
}

namespace Engine::Capture::System
{
    auto CaptureService::Capture() -> std::shared_ptr<const Snapshot>
    {
        auto snapshot{ std::make_shared<Snapshot>() };

        auto& reader = m_MemoryReaderService;

        const std::uintptr_t threadContext{ m_CaptureStore.GetThreadContext() };
        const std::uintptr_t objectTableBase{ m_CaptureStore.GetObjectTableBase() };

        snapshot->TeardownEpoch = m_LifecycleStore.GetTeardownEpoch();
        snapshot->HasThreadContext = threadContext != 0;
        snapshot->HasObjectTable = objectTableBase != 0;

        if (threadContext != 0 && m_CheckedEpoch.load(std::memory_order_relaxed) != snapshot->TeardownEpoch)
        {
            this->CheckCapacities(threadContext, snapshot->TeardownEpoch);
        }

        if (objectTableBase != 0) this->CopyObjects(*snapshot, objectTableBase);

        this->CopyRegion(*snapshot, RegionId::PlayerArray, m_CaptureStore.GetPlayerTableBase());
        this->CopyRegion(*snapshot, RegionId::PlayerMapping, m_CaptureStore.GetMappingBase());
        this->CopyRegion(*snapshot, RegionId::Interaction, m_CaptureStore.GetInteractionTableBase());

        if (threadContext != 0)
        {
            this->CopyRegion(*snapshot, RegionId::TimeGlobals, reader.Read<std::uintptr_t>(threadContext, k_ThreadContextTimeGlobalsOffset));
            this->CopyRegion(*snapshot, RegionId::GameEngineGlobals, reader.Read<std::uintptr_t>(threadContext, k_ThreadContextGameEngineOffset));
            this->CopyRegion(*snapshot, RegionId::EventGlobals, reader.Read<std::uintptr_t>(threadContext, k_ThreadContextEventGlobalsOffset));
            this->CopyRegion(*snapshot, RegionId::GameGlobals, reader.Read<std::uintptr_t>(threadContext, k_ThreadContextGameModeOffset));

            const std::uintptr_t megaloObjects{ reader.Read<std::uintptr_t>(threadContext, k_ThreadContextMegaloObjectsOffset) };
            if (megaloObjects != 0) this->CopyRegion(*snapshot, RegionId::MegaloObjects, reader.Read<std::uintptr_t>(megaloObjects, k_DataArrayData));
        }

        this->CopyRegion(*snapshot, RegionId::Placements, this->FindPlacements(threadContext));

        m_CaptureStore.Publish(snapshot);

        return snapshot;
    }

    auto CaptureService::CheckCapacities(std::uintptr_t threadContext, std::uint64_t epoch) -> void
    {
        auto& reader = m_MemoryReaderService;

        const std::uintptr_t objectArray{ reader.Read<std::uintptr_t>(threadContext, k_ThreadContextObjectDataOffset) };
        const std::uintptr_t playerArray{ reader.Read<std::uintptr_t>(threadContext, k_ThreadContextTelemetryDataOffset) };
        if (objectArray == 0 || playerArray == 0) return;

        const std::int32_t objectCapacity{ reader.Read<std::int32_t>(objectArray, k_DataArrayCapacity) };
        const std::int32_t playerCapacity{ reader.Read<std::int32_t>(playerArray, k_DataArrayCapacity) };

        // A failed read or an array not allocated yet gives 0: nothing is checked, the next tick tries again.
        if (objectCapacity <= 0 || playerCapacity <= 0) return;

        m_CheckedEpoch.store(epoch, std::memory_order_relaxed);

        if (objectCapacity != static_cast<std::int64_t>(k_MaxObjects))
        {
            m_Log.Message("[CaptureService] WARNING: The object table of the engine has {} slots and Artemis assumes {}.",
                objectCapacity, k_MaxObjects);
        }

        if (playerCapacity != static_cast<std::int64_t>(k_MaxPlayers))
        {
            m_Log.Message("[CaptureService] WARNING: The player table of the engine has {} slots and Artemis assumes {}.",
                playerCapacity, k_MaxPlayers);
        }

        m_Log.Message("[CaptureService] INFO: Table capacities checked: objects {}, players {}.", objectCapacity, playerCapacity);
    }

    auto CaptureService::CopyObjects(Snapshot& snapshot, std::uintptr_t tableBase) -> void
    {
        auto& reader = m_MemoryReaderService;

        for (std::uint32_t index{ 0 }; index < k_MaxObjects; ++index)
        {
            std::uintptr_t entryAddress{ tableBase + (index * k_EntrySize) };

            std::uint16_t salt{ reader.Read<std::uint16_t>(entryAddress, k_EntrySaltOffset) };
            if (salt == 0) continue;

            std::uintptr_t address{ reader.Read<std::uintptr_t>(entryAddress, k_EntryAddressOffset) };
            if (address == 0) continue;

            std::uint16_t datumSize{ reader.Read<std::uint16_t>(entryAddress, k_EntryDatumSizeOffset) };
            if (datumSize < k_MinDatumSize) continue;

            std::uint8_t kindId{ reader.Read<std::uint8_t>(entryAddress, k_EntryKindOffset) };

            RawObject object{};
            object.Handle = (static_cast<std::uint32_t>(salt) << 16) | index;
            object.Address = address;
            object.KindId = kindId;

            object.Bytes.resize(datumSize);
            if (!reader.ReadRaw(address, object.Bytes.data(), datumSize)) continue;

            snapshot.RawObjects.emplace(object.Handle, std::move(object));
        }
    }

    auto CaptureService::CopyRegion(Snapshot& snapshot, RegionId id, std::uintptr_t address) -> void
    {
        if (address == 0) return;

        Region& region{ snapshot.Regions[static_cast<std::size_t>(id)] };
        region.OriginalAddress = address;
        region.Bytes.resize(RegionSize(id));

        if (!m_MemoryReaderService.ReadRaw(address, region.Bytes.data(), region.Bytes.size()))
        {
            region.Bytes.clear();
            region.OriginalAddress = 0;
        }
    }

    auto CaptureService::FindPlacements(std::uintptr_t threadContext) -> std::uintptr_t
    {
        if (threadContext == 0) return 0;

        auto& reader = m_MemoryReaderService;

        const std::uintptr_t gameMode{ reader.Read<std::uintptr_t>(threadContext, k_ThreadContextGameModeOffset) };
        if (gameMode == 0) return 0;

        if (reader.Read<std::uint8_t>(gameMode, k_GameModeTypeOffset) != k_GameModeMultiplayer) return 0;

        const std::uintptr_t pointer{ reader.Read<std::uintptr_t>(threadContext, k_ThreadContextMapVariantOffset) };
        if (pointer == 0) return 0;

        const std::uintptr_t mapVariant{ (pointer + k_MapVariantAdjust) & ~k_MapVariantAlignMask };

        return mapVariant + k_PlacementsOffset;
    }

    auto CaptureService::Cleanup() -> void
    {
        m_CaptureStore.Cleanup();

        m_Log.Message("[CaptureService] INFO: Cleanup completed.");
    }
}