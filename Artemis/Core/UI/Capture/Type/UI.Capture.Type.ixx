export module UI.Capture.Type;

import std;

export namespace UI::Capture::Type
{
    // Size of the label text buffer.
    inline constexpr std::size_t k_LabelSize{ 64 };

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
}