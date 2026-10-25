export module Resolved.Definitions.Type:Phmo;

import std;

export namespace Resolved::Definitions::Type::Phmo
{
    // Collision group value that means "keep the group of the rigid body".
    inline constexpr std::uint8_t k_NoCollisionGroup{ 0xFF };

    // Kind of a physics model shape, same ids as the tag.
    enum class ShapeType : std::uint16_t
    {
        Sphere = 0x0000,
        Pill = 0x0001,
        Box = 0x0002,
        Triangle = 0x0003,
        Polyhedron = 0x0004,
        MultiSphere = 0x0005,
        Phantom = 0x0006,
        List = 0x000E,
        Mopp = 0x000F,

        Invalid = 0xFFFF,
    };

    // Primitive shape of a rigid body, reached through its lists, mopps and phantoms.
    struct Shape
    {
        ShapeType Type{ ShapeType::Invalid };

        // Index in the block of its type.
        std::int16_t Index{ -1 };

        std::uint32_t Name{};

        // Index in Phmo::Materials, -1 for none.
        std::int8_t MaterialIndex{ -1 };

        // Bit 0 suppresses effects, bit 1 forces collision with players.
        std::uint8_t MaterialFlags{};

        // Index in Phmo::PhantomTypes, -1 for none.
        std::int8_t PhantomTypeIndex{ -1 };

        // Havok layer override of this shape (A10). k_NoCollisionGroup keeps the group of the rigid body.
        std::uint8_t CollisionGroup{ k_NoCollisionGroup };

        // True when the shape hangs from a phantom shape: a volume that reports overlaps, not a solid.
        bool IsPhantom{};
    };

    struct Material
    {
        std::uint32_t Name{};
        std::uint32_t GlobalMaterialName{};

        // Bit 0 suppresses effects, bit 1 forces collision with players.
        std::uint8_t Flags{};

        // Index in Phmo::PhantomTypes, -1 for none.
        std::int16_t PhantomTypeIndex{ -1 };

        // The tag says this one does not affect anything.
        std::uint8_t ProxyCollisionGroup{};

        // Havok layer override of the shapes with this material (A10). k_NoCollisionGroup keeps the group of the rigid body.
        std::uint8_t CollisionGroup{ k_NoCollisionGroup };
    };

    struct RigidBody
    {
        std::int16_t NodeIndex{ -1 };
        std::int16_t RegionIndex{ -1 };
        std::int16_t PermutationIndex{ -1 };

        // Bit 6 forces keyframed.
        std::uint16_t Flags{};

        // Tag enum: 0 sphere, 1 stabilized sphere, 2 box, 3 stabilized box, 4 keyframed, 5 fixed.
        std::uint8_t MotionType{};

        // The tag says this one does not affect anything.
        std::uint8_t ProxyCollisionGroup{};

        // Tag enum: 0 default, 1 tiny ... 6 extra huge.
        std::uint16_t Size{};

        // Bit per Havok layer.
        std::uint32_t CollisionGroupMask{};

        float Mass{};

        ShapeType RootShapeType{ ShapeType::Invalid };
        std::int16_t RootShapeIndex{ -1 };

        // Primitive shapes under the root shape.
        std::vector<Shape> Shapes{};
    };

    // Motion a phantom volume applies to the objects inside it (lifts, man cannons, A12).
    struct PhantomType
    {
        // Bit 0 generates effects, 1 uses acceleration as force, 2 negates gravity, 3 ignores players, 4 ignores non-players,
        // 5 ignores bipeds, 6 ignores vehicles, 7 ignores weapons, 8 ignores equipment.
        std::uint32_t Flags{};

        std::uint8_t MinimumSize{};
        std::uint8_t MaximumSize{};

        // Marker of the motion. Empty means the up of the object.
        std::uint32_t MarkerName{};
        std::uint32_t AlignmentMarkerName{};

        float HookesLawE{};
        float LinearDeadRadius{};
        float CenterAcceleration{};
        float CenterMaximumVelocity{};
        float AxisAcceleration{};
        float AxisMaximumVelocity{};
        float DirectionAcceleration{};
        float DirectionMaximumVelocity{};
        float OrbitAcceleration{};
        float OrbitMaximumVelocity{};
        float AlignmentHookesLawE{};
        float AlignmentAcceleration{};
        float AlignmentMaximumVelocity{};
    };

    // Holds the physics model: the Havok rigid bodies of an object and their shapes.
    struct Phmo
    {
        std::string TagName{};

        std::vector<RigidBody> RigidBodies{};
        std::vector<Material> Materials{};
        std::vector<PhantomType> PhantomTypes{};
    };
}