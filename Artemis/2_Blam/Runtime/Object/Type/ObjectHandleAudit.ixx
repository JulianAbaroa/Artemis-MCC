export module Blam.Runtime.Object.Type:HandleAudit;

import std;

export namespace Blam::Runtime::Object::Type::HandleAudit
{
    // What the object links of one tick resolved to (F1 C.5 d). The table of the engine reuses a slot with a new salt when it deletes an
    // object, so a handle kept by another object can be old: Artemis looks handles up whole (salt included), an old one finds nothing.
    // note: Counted per tick, not accumulated. The links are the object handles the runtime follows (ObjectTableService::AuditHandles).
    // A link of 0 or NONE is no link. Checked == Matching + Stale + Free.
    struct HandleAudit
    {
        // Links that were not NONE.
        std::uint32_t Checked{};
        // Links whose slot holds a live object with the same salt.
        std::uint32_t Matching{};
        // Links whose slot holds a live object with another salt: the handle is old (the object was deleted and the slot reused).
        std::uint32_t Stale{};
        // Links whose slot holds no live object.
        std::uint32_t Free{};
        // Links with salt 0, which no live datum has. A subset: each one is also counted in Stale or Free (its slot may hold a live
        // object with another salt, or none). Not an old handle: the engine never writes it.
        std::uint32_t ZeroSalt{};
    };
}