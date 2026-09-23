export module Tables.Object.Type:BoneMatrix;

import std;

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
}