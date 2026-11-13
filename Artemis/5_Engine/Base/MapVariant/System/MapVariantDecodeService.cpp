module Engine.Base.MapVariant.System;
import :Decode;

import Common.Text.System;
import Engine.Base.Memory.Type;
import Engine.Base.MapVariant.Type;
import Blam.Layout.Object.Type;
import Blam.Layout.MapVariantFile.Type;
import std;

namespace
{
    namespace Signature = Engine::Base::Memory::Type::Signature;
    namespace LayoutMapVariant = Blam::Layout::Object::Type::MapVariant;
    namespace FileParse = Blam::Layout::MapVariantFile::Type::Parse;

    using Engine::Base::MapVariant::Type::DecodedMapVariant;
    using Engine::Base::MapVariant::Type::k_DecodeBufferSize;
    using Engine::Base::MapVariant::Type::k_DecodeStatusOffset;
    using Engine::Base::MapVariant::Type::k_DecodeStatusSize;

    using Blam::Layout::Object::Type::Constant::k_MapVariantSize;
    using Blam::Layout::Object::Type::Constant::k_MapVariantVersion;
    using FileParse::k_MapVariantChunkMinSize;

    // return: The bytes as hex text.
    auto ToHex(std::span<const std::byte> bytes) -> std::string
    {
        std::string text{};
        for (const std::byte value : bytes) text += std::format("{:02X}", static_cast<std::uint8_t>(value));
        return text;
    }
}

namespace Engine::Base::MapVariant::System
{
    auto MapVariantDecodeService::Decode(std::span<const std::byte> file) -> DecodedMapVariant
    {
        DecodedMapVariant decoded{};

        try
        {
            const FileParse::MapVariantFileResult parsed{ FileParse::Read(file) };

            if (!parsed.IsOk()) return this->Fail(decoded, parsed.Error);

            decoded.Name = Common::Text::System::WideToUtf8(parsed.File.Name);
            decoded.Description = Common::Text::System::WideToUtf8(parsed.File.Description);

            if (parsed.File.MvarSize == 0) return this->Fail(decoded, "the file has no map variant chunk");
            if (parsed.File.MvarSize < k_MapVariantChunkMinSize) return this->Fail(decoded, "the map variant chunk is too small");
            if (!this->Resolve()) return this->Fail(decoded, "the engine functions of the decode were not found");

            // The engine reads the chunk from its header on, and the buffer is its own copy so it never reads past the file.
            const std::vector<std::byte> chunk(file.begin() + static_cast<std::ptrdiff_t>(parsed.File.MvarOffset),
                file.begin() + static_cast<std::ptrdiff_t>(parsed.File.MvarOffset + parsed.File.MvarSize));
            std::vector<std::byte> buffer(k_DecodeBufferSize, std::byte{ 0 });

            m_Init(buffer.data());
            const std::uint8_t isDecoded{ m_Decode(chunk.data(), buffer.data()) };

            if (isDecoded == 0)
            {
                const std::span<const std::byte> status{ buffer.data() + k_DecodeStatusOffset, k_DecodeStatusSize };
                return this->Fail(decoded, std::format("the engine refused the map variant chunk (status {})", ToHex(status)));
            }

            decoded.Block.assign(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(k_MapVariantSize));

            const auto header{ LayoutMapVariant::Read(decoded.Block) };

            if (!header) return this->Fail(decoded, "the decoded block is not a map variant");

            decoded.Version = header->Version;
            decoded.Quotas = header->Quotas;
            decoded.MapId = header->MapId;
            decoded.PlacementsInUse = header->PlacementsInUse;

            if (header->Version != k_MapVariantVersion)
            {
                return this->Fail(decoded, std::format("the decoded map variant has version 0x{:X} instead of 0x{:X}", header->Version,
                    k_MapVariantVersion));
            }

            return decoded;
        }
        catch (const std::exception& exception)
        {
            return this->Fail(decoded, exception.what());
        }
    }

    auto MapVariantDecodeService::Resolve() -> bool
    {
        if (m_Init != nullptr && m_Decode != nullptr) return true;

        const std::uintptr_t init{ m_AOBService.FindPattern(Signature::k_MapVariantInit) };
        const std::uintptr_t decode{ m_AOBService.FindPattern(Signature::k_MapVariantDecode) };

        if (init == 0 || decode == 0)
        {
            m_Log.Message("[MapVariantDecodeService] ERROR: Failed to find the map variant {} of the engine.", init == 0 ? "initializer" : "decode");
            return false;
        }

        m_Init = reinterpret_cast<MapVariantInitFunction>(init);
        m_Decode = reinterpret_cast<MapVariantDecodeFunction>(decode);
        return true;
    }

    auto MapVariantDecodeService::Fail(DecodedMapVariant& decoded, std::string reason) const -> DecodedMapVariant
    {
        m_Log.Message("[MapVariantDecodeService] WARNING: Map variant file not decoded ({}).", reason);
        decoded.Error = std::move(reason);
        return std::move(decoded);
    }
}