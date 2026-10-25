module Platform.Capture.System;

import Common.Engine.Type;
import Datum.Object.Type;
import std;

namespace
{
    using Layout::Snapshot::Type::RawObject;
    using Layout::Snapshot::Type::Region;
    using Layout::Snapshot::Type::RegionSize;

    using Common::Engine::Type::k_MaxObjects;
    using Datum::Object::Type::Constant::k_EntrySize;
    using Datum::Object::Type::Constant::k_EntrySaltOffset;
    using Datum::Object::Type::Constant::k_EntryKindOffset;
    using Datum::Object::Type::Constant::k_EntryDatumSizeOffset;
    using Datum::Object::Type::Constant::k_EntryAddressOffset;
    using Datum::Object::Type::Constant::k_MinDatumSize;
    using Datum::Object::Type::Constant::k_ThreadContextTimeGlobalsOffset;
    using Datum::Object::Type::Constant::k_ThreadContextGameEngineOffset;
    using Datum::Object::Type::Constant::k_ThreadContextEventGlobalsOffset;
    using Datum::Object::Type::Constant::k_ThreadContextGameModeOffset;
    using Datum::Object::Type::Constant::k_GameModeTypeOffset;
    using Datum::Object::Type::Constant::k_GameModeMultiplayer;
    using Datum::Object::Type::Constant::k_ThreadContextMapVariantOffset;
    using Datum::Object::Type::Constant::k_MapVariantAdjust;
    using Datum::Object::Type::Constant::k_MapVariantAlignMask;
    using Datum::Object::Type::Constant::k_PlacementsOffset;
}

namespace Platform::Capture::System
{
    auto CaptureService::Capture() -> std::shared_ptr<const Snapshot>
    {
        auto snapshot{ std::make_shared<Snapshot>() };

        auto& reader = m_MemoryReaderService;

        const std::uintptr_t threadContext{ m_CaptureStore.GetThreadContext() };
        const std::uintptr_t objectTableBase{ m_CaptureStore.GetObjectTableBase() };

        snapshot->HasThreadContext = threadContext != 0;
        snapshot->HasObjectTable = objectTableBase != 0;

        if (objectTableBase != 0) this->CopyObjects(*snapshot, objectTableBase);

        this->CopyRegion(*snapshot, RegionId::PlayerArray, m_CaptureStore.GetPlayerTableBase());
        this->CopyRegion(*snapshot, RegionId::PlayerMapping, m_CaptureStore.GetMappingBase());
        this->CopyRegion(*snapshot, RegionId::Interaction, m_CaptureStore.GetInteractionTableBase());

        if (threadContext != 0)
        {
            this->CopyRegion(*snapshot, RegionId::TimeGlobals, reader.Read<std::uintptr_t>(threadContext, k_ThreadContextTimeGlobalsOffset));
            this->CopyRegion(*snapshot, RegionId::GameEngineGlobals, reader.Read<std::uintptr_t>(threadContext, k_ThreadContextGameEngineOffset));
            this->CopyRegion(*snapshot, RegionId::EventGlobals, reader.Read<std::uintptr_t>(threadContext, k_ThreadContextEventGlobalsOffset));
        }

        this->CopyRegion(*snapshot, RegionId::Placements, this->FindPlacements(threadContext));

        m_CaptureStore.Publish(snapshot);

        return snapshot;
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