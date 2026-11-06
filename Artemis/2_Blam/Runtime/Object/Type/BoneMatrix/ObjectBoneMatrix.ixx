export module Blam.Runtime.Object.Type:BoneMatrix;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
}

export namespace Blam::Runtime::Object::Type::BoneMatrix
{
#pragma pack(push, 1)
    struct BoneMatrix
    {
        float Scale{};
        std::array<float, 9> Rotation{};
        std::array<float, 3> Translation{};
    };
    static_assert(sizeof(BoneMatrix) == 0x34, "BoneMatrix size mismatch");
#pragma pack(pop)

    struct BoneMatrixTable
    {
        std::vector<BoneMatrix> Matrices{};
    };

    struct BonesHeader
    {
        std::uintptr_t Offset{ 0 };
        std::uint32_t NodeCount{ 0 };
    };

    // Rotation = 3 basis rows (forward, left, up). Local (node space) -> world
    inline auto Transform(const BoneMatrix& m, float lx, float ly, float lz) -> Vec3
    {
        const auto& r = m.Rotation;
        return Vec3{
            r[0] * lx + r[3] * ly + r[6] * lz + m.Translation[0],
            r[1] * lx + r[4] * ly + r[7] * lz + m.Translation[1],
            r[2] * lx + r[5] * ly + r[8] * lz + m.Translation[2]
        };
    }

    inline auto IsBoneMatrixValid(const BoneMatrix& m) -> bool
    {
        if (!std::isfinite(m.Scale) || m.Scale < 1e-4f || m.Scale > 1e4f)
        {
            return false;
        }

        for (int row = 0; row < 3; ++row)
        {
            const float a = m.Rotation[row * 3 + 0];
            const float b = m.Rotation[row * 3 + 1];
            const float c = m.Rotation[row * 3 + 2];

            if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(c))
            {
                return false;
            }

            const float n2 = a * a + b * b + c * c;
            if (n2 < 0.25f || n2 > 4.0f)
            {
                return false;
            }
        }

        for (int i = 0; i < 3; ++i)
        {
            if (!std::isfinite(m.Translation[i]))
            {
                return false;
            }
        }

        return true;
    }

    // Finds the bone of a node when the index is in range and its matrix is valid.
    // return: The bone, or nullptr for a negative or out of range index and for an invalid matrix.
    inline auto FindValidBone(const std::vector<BoneMatrix>& matrices, std::int32_t nodeIndex) -> const BoneMatrix*
    {
        if (nodeIndex < 0 || static_cast<std::size_t>(nodeIndex) >= matrices.size())
        {
            return nullptr;
        }

        const BoneMatrix& bone = matrices[nodeIndex];
        return IsBoneMatrixValid(bone) ? &bone : nullptr;
    }

    // The table of an object and the Clear generation of the store it was written in (0 = never, or discarded).
    struct BoneMatrixEntry
    {
        BoneMatrixTable Table{};
        std::uint64_t Generation{};
    };

    // Bone matrix tables, by object handle. The entries of older generations are kept for their buffers and are not visible.
    struct BoneMatrixTables : std::unordered_map<std::uint32_t, BoneMatrixEntry>
    {
        using unordered_map::unordered_map;
    };
}