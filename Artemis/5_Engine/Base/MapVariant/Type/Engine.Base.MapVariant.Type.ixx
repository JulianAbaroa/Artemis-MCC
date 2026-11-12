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