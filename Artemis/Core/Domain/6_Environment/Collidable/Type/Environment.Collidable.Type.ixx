export module Environment.Collidable.Type;

import Common.Math.Type;
import Resolved.Definitions.Type;
import Resolved.World.Type;
import std;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using Mesh = Resolved::Definitions::Type::Coll::Mesh;
    using ResolvedColl = Resolved::Definitions::Type::Coll::Coll;
    using ResolvedRegionStates = Resolved::World::Type::RegionStates::RegionStates;
}

export namespace Environment::Collidable::Type
{
    struct Context
    {
        const ResolvedColl* Coll{ nullptr };

        const ResolvedRegionStates* States{ nullptr };
    };

    struct CollidablePart
    {
        const Mesh* Source{ nullptr };
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

        Mesh WorldMesh{};
        std::vector<CollidablePart> Parts{};

        std::uint16_t RegionBlockSize{};
        std::uint16_t RegionBlockOffset{};
        std::vector<std::uint8_t> RegionBlock{};
        std::vector<RegionDiagnostic> Regions{};
    };
}