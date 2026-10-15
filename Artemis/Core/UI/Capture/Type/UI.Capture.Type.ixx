export module UI.Capture.Type;

import std;

export namespace UI::Capture::Type
{
    // Size of the label text buffer.
    inline constexpr std::size_t k_LabelSize{ 64 };

    // Layers of the Havok group filter (hkpGroupFilter lookup table rows).
    inline constexpr std::size_t k_HavokLayerCount{ 32 };

    // Range of one component inside a datum, in absolute datum offsets. End is the byte after the last one.
    struct ComponentRange
    {
        const char* Name{ "" };
        std::size_t Start{};
        std::size_t End{};
    };

    // Why an object is part of a capture.
    enum class Source : std::uint8_t
    {
        // Biped of the local player.
        Self,

        // Weapon, armor ability or vehicle of a captured biped.
        Linked,

        // Biped of another player.
        Player,

        // Object pinned from the object table.
        Pinned,
    };

    // Object a capture copies.
    struct Target
    {
        std::uint32_t Handle{};
        Source Source{ Source::Self };
    };

    // Run of consecutive bytes that changed since the previous capture of the same object.
    struct ByteChange
    {
        std::uint32_t Handle{};
        std::string FourCC{};
        const char* Component{ "" };
        std::size_t Offset{};
        std::vector<std::uint8_t> Before{};
        std::vector<std::uint8_t> After{};
    };

    // Collision filter of the Havok world (A10).
    struct HavokFilter
    {
        std::uintptr_t World{};
        std::uintptr_t Filter{};

        // Row per layer: bit j set if layer j collides with the row layer.
        std::array<std::uint32_t, k_HavokLayerCount> Lookup{};
    };

    // Havok rigid body of an object (A10).
    struct HavokBody
    {
        std::uintptr_t Address{};

        // hkpGroupFilter info: bits 0-4 layer, 5-9 subsystem, 10-14 subsystem it ignores, 16-31 system group.
        std::uint32_t FilterInfo{};

        // Havok motion type: 1 dynamic, 2 sphere inertia, 3 box inertia, 4 keyframed, 5 fixed, 6 thin box, 7 character.
        std::uint8_t MotionType{};
    };

    // Havok component of an object, read live from the component data array (A10).
    struct HavokComponent
    {
        std::uint32_t Handle{};
        std::uintptr_t Address{};

        // First u16 of the element. Matches the high half of Handle if the data array is the right one.
        std::uint16_t Salt{};
        std::uint32_t Flags{};

        // Object that owns the component, expected to be the captured object.
        std::uint32_t Owner{};

        // Size class of the phmo (0 default, 1 tiny ... 6 extra huge).
        std::int8_t Size{};

        // Count as read from memory. Bodies stays empty when it is out of range.
        std::int32_t BodyCount{};
        std::vector<HavokBody> Bodies{};
    };
}