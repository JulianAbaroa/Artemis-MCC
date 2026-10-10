module Tables.Object.State;
import :BoneOffsets;

namespace Tables::Object::State
{
    auto BoneOffsetsStore::Get(std::uint32_t handle) const -> std::optional<BonesHeader>
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_BonesHeaders.find(handle);
        if (it == m_BonesHeaders.end()) return std::nullopt;
        return it->second;
    }

    auto BoneOffsetsStore::Set(std::uint32_t handle, BonesHeader info) -> bool
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto [it, inserted] = m_BonesHeaders.try_emplace(handle, info);
        return inserted;
    }

    auto BoneOffsetsStore::Cleanup() -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_BonesHeaders.clear();
    }
}