export module Tables.Object.Type:Size;

import std;

export namespace Tables::Object::Type::Size
{
    // TODO: Confirm if this is true.
    constexpr size_t k_Base{ 0x330 };

    // For some reason only spartan bipeds seems to have this size in memory.
    constexpr size_t k_Biped{ 0x18C8 };

}