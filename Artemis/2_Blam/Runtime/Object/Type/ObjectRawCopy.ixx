export module Blam.Runtime.Object.Type:RawCopy;

import Common.Engine.Type;
import std;

namespace
{
    using Common::Engine::Type::k_MaxObjects;
    using Common::Engine::Type::k_NoHandle;
}

export namespace Blam::Runtime::Object::Type::RawCopy
{
    // The bytes of the datum of one object as the last tick that read it saw them. The object is clean when the next tick has the same
    // handle and the same bytes (OT.1, D303).
    struct RawCopy
    {
        // Handle the bytes belong to, or none while the slot has no copy.
        std::uint32_t Handle{ k_NoHandle };
        // Kept between ticks: a new copy is written over the buffer, so a steady tick allocates nothing.
        std::vector<std::uint8_t> Bytes{};
        // Tick of the service that last saw the object of this slot in the snapshot.
        std::uint64_t SeenTick{};
    };

    // One copy per slot of the object table (the handle index, without the salt).
    struct RawCopies : std::array<RawCopy, k_MaxObjects>
    {
    };
}