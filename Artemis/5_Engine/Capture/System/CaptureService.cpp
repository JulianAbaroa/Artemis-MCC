module Engine.Capture.System;

import Common.Engine.Type;
import Blam.Layout.Object.Type;
import std;

namespace
{
    using Blam::Layout::Snapshot::Type::Region;
    using Blam::Layout::Snapshot::Type::RegionSize;

    using Common::Engine::Type::k_MaxObjects;
    using Common::Engine::Type::k_MaxPlayers;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextObjectDataOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextTelemetryDataOffset;
    using Blam::Layout::Object::Type::Constant::k_DataArrayCapacity;
    using Blam::Layout::Object::Type::Constant::k_DataArrayFirstUnusedIndex;
    using Blam::Layout::Object::Type::Constant::k_DataArrayLiveCount;
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

    // return: Slots of the object table to walk. The first-unused mark when it is readable, else the capacity, never more than the
    // capacity (when it is known) or k_MaxObjects.
    auto ObjectSlotLimit(std::int32_t capacity, std::int32_t firstUnused) -> std::uint32_t
    {
        std::uint32_t limit{ k_MaxObjects };
        if (capacity > 0) limit = (std::min)(limit, static_cast<std::uint32_t>(capacity));
        if (firstUnused > 0) limit = (std::min)(limit, static_cast<std::uint32_t>(firstUnused));
        return limit;
    }
}

namespace Engine::Capture::System
{
    auto CaptureService::Capture() -> std::shared_ptr<const Snapshot>
    {
        // The epoch is read once, before anything is copied (see m_CheckedEpoch).
        const std::uint64_t epoch{ m_LifecycleStore.GetTeardownEpoch() };
        const std::shared_ptr<Snapshot> snapshot{ this->AcquireSnapshot(epoch) };

        auto& reader = m_MemoryReaderService;

        const std::uintptr_t threadContext{ m_CaptureStore.GetThreadContext() };
        const std::uintptr_t objectTableBase{ m_CaptureStore.GetObjectTableBase() };

        snapshot->TeardownEpoch = epoch;
        snapshot->HasThreadContext = threadContext != 0;
        snapshot->HasObjectTable = objectTableBase != 0;

        if (threadContext != 0 && m_CheckedEpoch.load(std::memory_order_relaxed) != snapshot->TeardownEpoch)
        {
            this->CheckCapacities(threadContext, snapshot->TeardownEpoch);
        }

        if (objectTableBase != 0) this->CopyObjects(*snapshot, threadContext, objectTableBase);

        this->CopyRegion(*snapshot, RegionId::PlayerArray, m_CaptureStore.GetPlayerTableBase());
        this->CopyRegion(*snapshot, RegionId::PlayerMapping, m_CaptureStore.GetMappingBase());
        this->CopyRegion(*snapshot, RegionId::Interaction, m_CaptureStore.GetInteractionTableBase());

        // Every region goes through CopyRegion on every capture, with address 0 when its pointer is missing: a reused Snapshot must not keep
        // the region of the previous capture.
        const auto contextPointer = [&](std::size_t offset) -> std::uintptr_t
        {
            return threadContext != 0 ? reader.Read<std::uintptr_t>(threadContext, offset) : 0;
        };

        this->CopyRegion(*snapshot, RegionId::TimeGlobals, contextPointer(k_ThreadContextTimeGlobalsOffset));
        this->CopyRegion(*snapshot, RegionId::GameEngineGlobals, contextPointer(k_ThreadContextGameEngineOffset));
        this->CopyRegion(*snapshot, RegionId::EventGlobals, contextPointer(k_ThreadContextEventGlobalsOffset));
        this->CopyRegion(*snapshot, RegionId::GameGlobals, contextPointer(k_ThreadContextGameModeOffset));

        const std::uintptr_t megaloObjects{ contextPointer(k_ThreadContextMegaloObjectsOffset) };
        this->CopyRegion(*snapshot, RegionId::MegaloObjects, megaloObjects != 0 ? reader.Read<std::uintptr_t>(megaloObjects, k_DataArrayData) : 0);

        this->CopyRegion(*snapshot, RegionId::Placements, this->FindPlacements(threadContext));

        m_CaptureStore.Publish(snapshot);

        return snapshot;
    }

    auto CaptureService::AcquireSnapshot(std::uint64_t epoch) -> std::shared_ptr<Snapshot>
    {
        // A new map load starts a new pool: what the old one holds goes with the old map.
        if (m_PoolEpoch != epoch)
        {
            m_Pool.clear();
            m_PoolEpoch = epoch;
        }

        // A pooled Snapshot has one reference (the pool) when nobody else holds it. The published one has another (the store), so it is
        // never chosen. A reader that let go cannot take it again: it only gets the published Snapshot from the store.
        for (const std::shared_ptr<Snapshot>& pooled : m_Pool)
        {
            if (pooled.use_count() != 1) continue;

            // The flags, the epoch and every region address are written by Capture, and every region goes through CopyRegion, which empties it when its address is 0.
            pooled->RawObjects.Clear();
            return pooled;
        }

        auto snapshot{ std::make_shared<Snapshot>() };
        if (m_Pool.size() < k_SnapshotPoolCapacity) m_Pool.push_back(snapshot);
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

    auto CaptureService::CopyObjects(Snapshot& snapshot, std::uintptr_t threadContext, std::uintptr_t tableBase) -> void
    {
        auto& reader = m_MemoryReaderService;

        std::int32_t capacity{ 0 };
        std::int32_t firstUnused{ 0 };
        const std::uintptr_t objectArray{ threadContext != 0 ? reader.Read<std::uintptr_t>(threadContext, k_ThreadContextObjectDataOffset) : 0 };
        if (objectArray != 0)
        {
            capacity = reader.Read<std::int32_t>(objectArray, k_DataArrayCapacity);
            firstUnused = reader.Read<std::int32_t>(objectArray, k_DataArrayFirstUnusedIndex);
            const std::int32_t liveCount{ reader.Read<std::int32_t>(objectArray, k_DataArrayLiveCount) };
            m_TelemetryStore.RecordObjectTable(firstUnused > 0 ? static_cast<std::uint64_t>(firstUnused) : 0,
                liveCount > 0 ? static_cast<std::uint64_t>(liveCount) : 0);
        }

        const std::uint32_t slotLimit{ ObjectSlotLimit(capacity, firstUnused) };

        for (std::uint32_t index{ 0 }; index < slotLimit; ++index)
        {
            std::uintptr_t entryAddress{ tableBase + (index * k_EntrySize) };

            std::uint16_t salt{ reader.Read<std::uint16_t>(entryAddress, k_EntrySaltOffset) };
            if (salt == 0) continue;

            std::uintptr_t address{ reader.Read<std::uintptr_t>(entryAddress, k_EntryAddressOffset) };
            if (address == 0) continue;

            std::uint16_t datumSize{ reader.Read<std::uint16_t>(entryAddress, k_EntryDatumSizeOffset) };
            if (datumSize < k_MinDatumSize) continue;

            std::uint8_t kindId{ reader.Read<std::uint8_t>(entryAddress, k_EntryKindOffset) };

            // The datum is read straight into the arena of the snapshot.
            const std::uint32_t handle{ (static_cast<std::uint32_t>(salt) << 16) | index };
            const std::optional<std::span<std::uint8_t>> bytes{ snapshot.RawObjects.Emplace(handle, address, kindId, datumSize) };
            if (!bytes) continue;

            if (!reader.ReadRaw(address, bytes->data(), datumSize)) snapshot.RawObjects.PopLast();
        }
    }

    auto CaptureService::CopyRegion(Snapshot& snapshot, RegionId id, std::uintptr_t address) -> void
    {
        Region& region{ snapshot.Regions[static_cast<std::size_t>(id)] };
        if (address == 0)
        {
            // A reused snapshot may hold the region of an older tick.
            region.OriginalAddress = 0;
            region.Bytes.clear();
            return;
        }

        region.OriginalAddress = address;
        // The same size keeps the bytes of the older tick, which ReadRaw overwrites whole.
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