export module DevTools.Workbench.Capture.Type;

import std;

export namespace DevTools::Workbench::Capture::Type
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

        // Projectile alive at the time of the capture.
        Projectile,
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

    // Havok phantom of an object: a volume that reports overlaps without a contact response (lifts, pickups candidates, A10).
    struct HavokPhantom
    {
        // Offset of the phantom pointer inside the component.
        std::uintptr_t Slot{};
        std::uintptr_t Address{};

        // hkpGroupFilter info, same layout as HavokBody::FilterInfo.
        std::uint32_t FilterInfo{};
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

        // Phantoms the component holds (only the slots that are set).
        std::vector<HavokPhantom> Phantoms{};
    };

    // RVA of the hkpWorld pointer (DAT_184e2fe68 in Ghidra).
    inline constexpr std::uintptr_t k_HavokWorldRva{ 0x4E2FE68 };

    // Collision filter pointer inside the hkpWorld (FUN_18017d680, FUN_18012e500).
    inline constexpr std::uintptr_t k_WorldFilterOffset{ 0xD0 };

    // Layer table inside the hkpGroupFilter (FUN_180882080, its isCollisionEnabled).
    inline constexpr std::uintptr_t k_FilterLookupOffset{ 0x4C };

    // RVA of the pointer to the Havok component data array (DAT_180c519b8 in Ghidra).
    inline constexpr std::uintptr_t k_HavokComponentArrayRva{ 0xC519B8 };



    // Havok component layout (FUN_18012e1e8, FUN_18012e7cc, FUN_18017d680).
    inline constexpr std::uintptr_t k_HavokComponentSize{ 0xC0 };
    inline constexpr std::uintptr_t k_ComponentFlagsOffset{ 0x18 };
    inline constexpr std::uintptr_t k_ComponentOwnerOffset{ 0x1C };
    inline constexpr std::uintptr_t k_ComponentSizeOffset{ 0x22 };
    inline constexpr std::uintptr_t k_ComponentBodiesOffset{ 0x30 };
    inline constexpr std::uintptr_t k_ComponentBodyCountOffset{ 0x38 };
    inline constexpr std::uintptr_t k_BodyEntrySize{ 0x50 };
    inline constexpr std::uintptr_t k_BodyEntryRigidBodyOffset{ 0x40 };

    // Phantom pointers of the component, removed from the world by FUN_18017fd78 and queried by FUN_180180c04.
    inline constexpr std::array<std::uintptr_t, 2> k_ComponentPhantomOffsets{ 0xA8, 0xB0 };

    // hkpRigidBody fields: collidable filter info and motion type. The filter info is at the same offset in a phantom (hkpWorldObject).
    inline constexpr std::uintptr_t k_RigidBodyFilterInfoOffset{ 0x4C };
    inline constexpr std::uintptr_t k_RigidBodyMotionTypeOffset{ 0x160 };

    // RVA of the u64 flags of the collision test of projectiles (DAT_1828ffcf0 in Ghidra, read by FUN_1804f4e88).
    inline constexpr std::uintptr_t k_CollisionTestFlagsRva{ 0x28FFCF0 };


    // Fields of the hkpGroupFilter info: 5 bits each, the system group in the high half.
    inline constexpr std::uint32_t k_FilterFieldMask{ 0x1F };
    inline constexpr std::uint32_t k_SubsystemShift{ 5 };
    inline constexpr std::uint32_t k_IgnoredSubsystemShift{ 10 };
    inline constexpr std::uint32_t k_SystemGroupShift{ 16 };
}