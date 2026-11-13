export module Engine.Base.MapVariant.System:Decode;

import Common.Log.System;
import Engine.Base.Memory.System;
import Engine.Base.MapVariant.Type;
import std;

export namespace Engine::Base::MapVariant::System
{
    // Decodes a .mvar file of Forge with the decode of the engine, without a map loaded and without the game thread (Q106).
    // note: The engine functions only read the bytes they are given, hash them with BCrypt and write the buffer: they do not look at the
    // scenario, the tags or the thread context (R5 reading of 0x17adac and 0x6bff8). The one that calls it still runs it on the MCC
    // thread of the lifecycle update, never on the UI thread. It does not call the validation of the load, so the result keeps the bounds,
    // budget and quotas the file carries.
    class MapVariantDecodeService
    {
    private:
        using Log = Common::Log::System::Log;

        using AOBService = Engine::Base::Memory::System::AOBService;

        using DecodedMapVariant = Engine::Base::MapVariant::Type::DecodedMapVariant;
        using MapVariantInitFunction = Engine::Base::MapVariant::Type::MapVariantInitFunction;
        using MapVariantDecodeFunction = Engine::Base::MapVariant::Type::MapVariantDecodeFunction;

    public:
        MapVariantDecodeService(Log& log, AOBService& aobService) : m_Log(log), m_AOBService(aobService) {}
        ~MapVariantDecodeService() = default;

        MapVariantDecodeService(const MapVariantDecodeService&) = delete;
        auto operator=(const MapVariantDecodeService&) -> MapVariantDecodeService& = delete;

        // Finds the two engine functions by signature the first time, then decodes.
        // param file: The whole .mvar file.
        // return: The map variant with its header, or the reason it was not decoded (also logged as a WARNING): not a BLF file, no map
        // variant chunk, a signature not found, the engine refused it (the SHA1 of the chunk or its bitstream) or the result is not a map variant.
        // note: MCC thread only. Never throws.
        auto Decode(std::span<const std::byte> file) -> DecodedMapVariant;

    private:
        Log& m_Log;
        AOBService& m_AOBService;

        MapVariantInitFunction m_Init{ nullptr };
        MapVariantDecodeFunction m_Decode{ nullptr };

        // return: Whether both functions are known. Looks for them until it finds them.
        auto Resolve() -> bool;

        auto Fail(DecodedMapVariant& decoded, std::string reason) const -> DecodedMapVariant;
    };
}