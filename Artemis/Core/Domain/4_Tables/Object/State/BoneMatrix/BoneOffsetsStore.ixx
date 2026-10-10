export module Tables.Object.State:BoneOffsets;

import Tables.Object.Type;
import std;

export namespace Tables::Object::State
{
    class BoneOffsetsStore
    {
    private:
        using BonesHeader = Tables::Object::Type::BoneMatrix::BonesHeader;
        using BonesHeaders = std::unordered_map<std::uint32_t, BonesHeader>;

    public:
        BoneOffsetsStore() = default;
        ~BoneOffsetsStore() = default;

        auto Get(std::uint32_t handle) const -> std::optional<BonesHeader>;
        auto Set(std::uint32_t handle, BonesHeader info) -> bool;

        auto Cleanup() -> void;

    private:
        BonesHeaders m_BonesHeaders{};
        mutable std::mutex m_Mutex{};
    };
}