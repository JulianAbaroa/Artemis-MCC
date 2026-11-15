export module Perception.Collidable.Type;

import Common.Math.Type;
import Blam.MapModel.Definitions.Type;
import Blam.MapModel.World.Type;
import Blam.Runtime.Object.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Blam::MapModel::Definitions::Type::Coll::CollMesh;
    using Blam::MapModel::Definitions::Type::Coll::Coll;
    using Blam::MapModel::World::Type::RegionStates::RegionStates;
    using Blam::Runtime::Object::Type::BoneMatrix::BoneMatrix;
    using Blam::Runtime::Object::Type::BoneMatrix::BoneMatrixTable;
    using Blam::Runtime::Object::Type::DamageSection::DamageSectionTable;
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

    // Everything the parts of one object are built from (D299): the fields of its row that place it, its valid bone matrices, the bytes
    // of its region block and the collision model and region states its tag resolves to.
    // note: Equal keys give equal parts, so whoever keeps parts keeps the key next to them. Equality is by bytes, a float that changes
    // its sign or becomes a NaN is a different key. The row fields are the position (BoundingCenter), forward and up that CollectRigid
    // and CollectSkeletal read. Scale is not read. The bones are kept only when at least one matrix is valid, as BuildInstance does.
    struct CollidablePartsKey
    {
        std::uint32_t Handle{};

        // Datum index of the tag of the row. Its name only when the row has none.
        std::uint32_t DatumIndex{};
        std::string TagName{};

        Vec3 Position{};
        Vec3 Forward{};
        Vec3 Up{};

        bool HasBones{ false };
        std::vector<BoneMatrix> Bones{};

        bool HasDamage{ false };
        std::vector<std::uint8_t> DamageRegions{};

        const Coll* CollModel{ nullptr };
        const RegionStates* States{ nullptr };

        auto operator==(const CollidablePartsKey& other) const -> bool
        {
            const auto same = [](const auto& a, const auto& b) { return std::memcmp(&a, &b, sizeof(a)) == 0; };
            const auto sameBytes = [](const auto& a, const auto& b)
            {
                return a.size() == b.size() && (a.empty() || std::memcmp(a.data(), b.data(), a.size() * sizeof(*a.data())) == 0);
            };

            return Handle == other.Handle && DatumIndex == other.DatumIndex && TagName == other.TagName
                && same(Position, other.Position) && same(Forward, other.Forward) && same(Up, other.Up)
                && HasBones == other.HasBones && sameBytes(Bones, other.Bones)
                && HasDamage == other.HasDamage && sameBytes(DamageRegions, other.DamageRegions)
                && CollModel == other.CollModel && States == other.States;
        }
    };

    // What an object resolves to before its parts are built: the collision model, its region states and the tables of the object.
    // note: Bones is null when the object has none or none of its matrices is valid. Damage is null without a table.
    struct CollidableInputs
    {
        Context Resolved{};
        const BoneMatrixTable* Bones{ nullptr };
        const DamageSectionTable* Damage{ nullptr };
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