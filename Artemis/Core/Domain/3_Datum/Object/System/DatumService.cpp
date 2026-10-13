module Datum.Object.System;

import Resolved.Definitions.Type;
import Datum.Object.Type;
import std;

namespace
{
    using ObjectKind = Resolved::Definitions::Type::Object::ObjectKind;
    using RawObject = Datum::Object::Type::Raw::RawObject;
    using Snapshot = Datum::Object::Type::Raw::Snapshot;

    using Datum::Object::Type::Constant::k_MaxObjects;
    using Datum::Object::Type::Constant::k_EntrySize;
    using Datum::Object::Type::Constant::k_EntrySaltOffset;
    using Datum::Object::Type::Constant::k_EntryKindOffset;
    using Datum::Object::Type::Constant::k_EntryDatumSizeOffset;
    using Datum::Object::Type::Constant::k_EntryAddressOffset;
    using Datum::Object::Type::Constant::k_MinDatumSize;
    using Datum::Object::Type::Constant::k_ThreadContextGameModeOffset;
    using Datum::Object::Type::Constant::k_GameModeTypeOffset;
    using Datum::Object::Type::Constant::k_GameModeMultiplayer;
    using Datum::Object::Type::Constant::k_ThreadContextMapVariantOffset;
    using Datum::Object::Type::Constant::k_MapVariantAdjust;
    using Datum::Object::Type::Constant::k_MapVariantAlignMask;
    using Datum::Object::Type::Constant::k_PlacementsOffset;
    using Datum::Object::Type::Constant::k_MaxPlacements;
    using Datum::Object::Type::Constant::k_PlacementSize;
}

namespace Datum::Object::System
{
    auto DatumService::UpdateObjectTable() -> void
    {
        std::uintptr_t tableBase{ m_DatumStore.GetBase() };
        if (tableBase == 0) return;

        auto& reader = m_MemoryReaderService;

        Snapshot snapshot{};

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
            object.Kind = kindId <= static_cast<std::uint8_t>(ObjectKind::EffectScenery)
                ? static_cast<ObjectKind>(kindId) : ObjectKind::Invalid;

            object.Bytes.resize(datumSize);
            if (!reader.ReadRaw(address, object.Bytes.data(), datumSize)) continue;

            snapshot.RawObjects.emplace(object.Handle, std::move(object));
        }

        snapshot.Placements = this->ReadPlacements();

        m_DatumStore.Publish(std::move(snapshot));
    }

    auto DatumService::ReadPlacements() -> std::vector<std::uint8_t>
    {
        const std::uintptr_t threadContext{ m_DatumStore.GetThreadContext() };
        if (threadContext == 0) return {};

        auto& reader = m_MemoryReaderService;

        const std::uintptr_t gameMode{ reader.Read<std::uintptr_t>(threadContext, k_ThreadContextGameModeOffset) };
        if (gameMode == 0) return {};

        if (reader.Read<std::uint8_t>(gameMode, k_GameModeTypeOffset) != k_GameModeMultiplayer) return {};

        const std::uintptr_t pointer{ reader.Read<std::uintptr_t>(threadContext, k_ThreadContextMapVariantOffset) };
        if (pointer == 0) return {};

        const std::uintptr_t mapVariant{ (pointer + k_MapVariantAdjust) & ~k_MapVariantAlignMask };

        std::vector<std::uint8_t> placements{};
        placements.resize(k_MaxPlacements * k_PlacementSize);

        if (!reader.ReadRaw(mapVariant + k_PlacementsOffset, placements.data(), placements.size())) return {};

        return placements;
    }

    auto DatumService::Cleanup() -> void
    {
        m_DatumStore.Cleanup();

        m_LogsService.Message("[DatumService] INFO: Cleanup completed.");
    }
}