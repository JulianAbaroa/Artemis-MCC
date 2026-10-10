module Template.Object.System;

import Resolved.Definitions.Type;
import Template.Object.Type;
import std;

namespace
{
    using ObjectKind = Resolved::Definitions::Type::Object::ObjectKind;
    using RawObject = Template::Object::Type::Raw::RawObject;
    using Snapshot = Template::Object::Type::Raw::Snapshot;

    using Template::Object::Type::Constant::k_MaxObjects;
    using Template::Object::Type::Constant::k_EntrySize;
    using Template::Object::Type::Constant::k_EntrySaltOffset;
    using Template::Object::Type::Constant::k_EntryKindOffset;
    using Template::Object::Type::Constant::k_EntryDatumSizeOffset;
    using Template::Object::Type::Constant::k_EntryAddressOffset;
    using Template::Object::Type::Constant::k_MinDatumSize;
}

namespace Template::Object::System
{
    auto TemplateService::UpdateObjectTable() -> void
    {
        std::uintptr_t tableBase{ m_TemplateStore.GetBase() };
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

        m_TemplateStore.Publish(std::move(snapshot));
    }

    auto TemplateService::Cleanup() -> void
    {
        m_TemplateStore.Cleanup();

        m_LogsService.Message("[TemplateService] INFO: Cleanup completed.");
    }
}