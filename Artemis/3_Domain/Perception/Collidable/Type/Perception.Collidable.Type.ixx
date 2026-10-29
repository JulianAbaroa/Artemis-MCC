export module Perception.Collidable.Type;

import Common.Math.Type;
import Blam.MapModel.Definitions.Type;
import Blam.MapModel.World.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Blam::MapModel::Definitions::Type::Coll::CollMesh;
    using Blam::MapModel::Definitions::Type::Coll::Coll;
    using Blam::MapModel::World::Type::RegionStates::RegionStates;
}

export namespace Perception::Collidable::Type
{
    // Layout of the region block of an object: one state entry per region of the render model, then one permutation byte per
    // region. A permutation byte of k_NoPermutation means the region has none.
    inline constexpr std::size_t k_RegionStateStride{ 0x04 };
    inline constexpr std::size_t k_RegionStateValue{ 0x01 };
    inline constexpr std::uint8_t k_NoPermutation{ 0xFF };

    struct Context
    {
        const Coll* Coll{ nullptr };

        const RegionStates* States{ nullptr };
    };

    struct CollidablePart
    {
        const CollMesh* Source{ nullptr };
        std::array<float, 12> Transform{};
    };

    // Damage state the engine keeps for one region of the variant, next to what the viewer shows.
    struct RegionDiagnostic
    {
        std::uint32_t Name{};

        // Collision region with the same name, or -1.
        int CollRegion{ -1 };

        // Damage state and permutation of the engine, or -1.
        int EngineState{ -1 };
        int EnginePermutation{ -1 };

        // Collision permutation with the name of the engine permutation, or -1.
        int MappedPermutation{ -1 };

        // Meshes the render model draws for the engine permutation, or -1.
        int EngineMeshCount{ -1 };

        // Permutations of the collision region that the viewer shows.
        std::vector<int> ShownPermutations{};
    };

    struct Collidable
    {
        std::uint32_t Handle{};
        std::string TagName{};

        Vec3 Position{};
        Vec3 Forward{};
        Vec3 Up{};

        CollMesh WorldMesh{};
        std::vector<CollidablePart> Parts{};

        std::uint16_t RegionBlockSize{};
        std::uint16_t RegionBlockOffset{};
        std::vector<std::uint8_t> RegionBlock{};
        std::vector<RegionDiagnostic> Regions{};
    };

    // Collidables of the tick.
    struct Collidables : std::vector<Collidable>
    {
        using vector::vector;
    };
}