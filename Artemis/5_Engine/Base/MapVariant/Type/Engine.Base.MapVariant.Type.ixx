export module Engine.Base.MapVariant.Type;

import std;

export namespace Engine::Base::MapVariant::Type
{
    // Signature of the engine function that prepares an empty map variant in a buffer (returns the buffer).
    using MapVariantInitFunction = auto(__fastcall*)(std::byte* variant) -> void*;

    // Signature of the engine function that decodes the map variant chunk of a .mvar (from the start of its chunk header) into a buffer
    // MapVariantInitFunction prepared. The low byte of the result says whether it worked.
    using MapVariantDecodeFunction = auto(__fastcall*)(const std::byte* chunk, std::byte* variant) -> std::uint8_t;

    // Bytes of the buffer the engine decode works on (the map variant plus 4). The map variant is at its start.
    inline constexpr std::size_t k_DecodeBufferSize{ 0xD9C0 };

    // The 16 bytes of status the engine keeps in the buffer, where it leaves the reason of a failed decode.
    inline constexpr std::size_t k_DecodeStatusOffset{ 0x2B8 };
    inline constexpr std::size_t k_DecodeStatusSize{ 16 };

    // A .mvar file decoded by the engine (Q106), or why it was not.
    struct DecodedMapVariant
    {
        // Name and description of the content header of the file.
        std::string Name{};
        std::string Description{};
        // The map variant, k_MapVariantSize bytes, as the engine decodes it: before the load recalculates the bounds, the budget and the
        // quotas against the scenario.
        std::vector<std::byte> Block{};
        std::uint16_t Version{ 0 };
        std::uint16_t Quotas{ 0 };
        std::uint32_t MapId{ 0 };
        std::uint32_t PlacementsInUse{ 0 };
        // Why it could not be decoded. Empty when it was.
        std::string Error{};

        auto IsOk() const -> bool
        {
            return Error.empty();
        }
    };

    // A map variant read from a recorded match configuration (Q98, D266): the name of the configuration and the bytes of its block.
    struct ActiveMapVariant
    {
        std::string Name{};
        std::vector<std::byte> Block{};
    };

    // A map variant checked against the live one of a match and not written yet.
    struct MapVariantLivePlan
    {
        // The map variant asked for. Null asks for the one the match loaded with.
        std::shared_ptr<const ActiveMapVariant> Pick{};
        // What goes into the options: the block of the pick, or the one the start carried, with its labels in the order of the live map variant.
        std::vector<std::byte> Block{};
        // What goes into the runtime map variant: the block over the world bounds the runtime already has.
        std::vector<std::byte> Composed{};
        std::uint32_t PlacementsInUse{ 0 };
        // What RemapLabels did so the match keeps the index of its labels: labels moved, appended and kept from the live map variant.
        std::uint32_t LabelsMoved{ 0 };
        std::uint32_t LabelsAppended{ 0 };
        std::uint32_t LabelsKept{ 0 };
    };
}