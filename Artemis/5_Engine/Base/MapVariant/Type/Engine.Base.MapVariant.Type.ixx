export module Engine.Base.MapVariant.Type;

import std;

export namespace Engine::Base::MapVariant::Type
{
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
        // What goes into the options: the block of the pick, or the one the start carried.
        std::vector<std::byte> Block{};
        // What goes into the runtime map variant: the block over the world bounds the runtime already has.
        std::vector<std::byte> Composed{};
        std::uint32_t PlacementsInUse{ 0 };
    };
}