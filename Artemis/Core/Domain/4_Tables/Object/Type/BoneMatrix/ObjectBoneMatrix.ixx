export module Tables.Object.Type:BoneMatrix;

import Common.Math.Type;
import std;

namespace
{
	using Vec3 = Common::Math::Type::Vec3;
}

export namespace Tables::Object::Type::BoneMatrix
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
        std::uintptr_t BaseAddress{};
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
}