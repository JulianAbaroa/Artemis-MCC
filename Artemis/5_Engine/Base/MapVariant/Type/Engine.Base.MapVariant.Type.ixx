export module Engine.Base.MapVariant.Type;

import std;

export namespace Engine::Base::MapVariant::Type
{
    // What the last load did with the active map variant.
    enum class MapVariantOutcome : std::uint8_t
    {
        // No load happened since the result was cleared.
        None,
        // The load had no map variant to write.
        NoMap,
        // The map variant was written into the start and read back equal.
        AppliedAtLoad,
        // The map variant does not fit the one the start carries (version, map id or quotas): the start went on with the one of its
        // configuration.
        Rejected,
        // The map variant could not be loaded or written: the start went on without it.
        Failed,
    };

    // Result of the last load or of the last request to set the active map variant.
    struct MapVariantResult
    {
        MapVariantOutcome Outcome{ MapVariantOutcome::None };
        std::string Name{};
        // Why it was not applied. Empty when it was.
        std::string Message{};
    };

    // A map variant read from a recorded match configuration (Q98, D266): the name of the configuration and the bytes of its block.
    struct ActiveMapVariant
    {
        std::string Name{};
        std::vector<std::byte> Block{};
    };
}