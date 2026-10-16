module UI.Capture.State;

namespace
{
    using UI::Capture::Type::k_LabelSize;
}

namespace UI::Capture::State
{
    auto CaptureUIStore::RequestCapture() -> void
    {
        m_IsCaptureRequested.store(true);
    }

    auto CaptureUIStore::ConsumeCaptureRequest() -> bool
    {
        return m_IsCaptureRequested.exchange(false);
    }

    auto CaptureUIStore::GetLabel() -> std::array<char, k_LabelSize>&
    {
        return m_Label;
    }

    auto CaptureUIStore::IsPinned(std::uint32_t handle) const -> bool
    {
        return std::ranges::find(m_Pinned, handle) != m_Pinned.end();
    }

    auto CaptureUIStore::SetPinned(std::uint32_t handle, bool value) -> void
    {
        if (value)
        {
            if (!this->IsPinned(handle)) m_Pinned.push_back(handle);
            return;
        }

        std::erase(m_Pinned, handle);
    }

    auto CaptureUIStore::GetPinned() const -> const std::vector<std::uint32_t>&
    {
        return m_Pinned;
    }

    auto CaptureUIStore::IsPlayersIncluded() const -> bool
    {
        return m_IsPlayersIncluded;
    }

    auto CaptureUIStore::SetPlayersIncluded(bool value) -> void
    {
        m_IsPlayersIncluded = value;
    }

    auto CaptureUIStore::IsProjectilesIncluded() const -> bool
    {
        return m_IsProjectilesIncluded;
    }

    auto CaptureUIStore::SetProjectilesIncluded(bool value) -> void
    {
        m_IsProjectilesIncluded = value;
    }

    auto CaptureUIStore::IsEngineIncluded() const -> bool
    {
        return m_IsEngineIncluded;
    }

    auto CaptureUIStore::SetEngineIncluded(bool value) -> void
    {
        m_IsEngineIncluded = value;
    }

    auto CaptureUIStore::FindPrevious(std::uint32_t handle) const -> const std::vector<std::uint8_t>*
    {
        const auto it{ m_Previous.find(handle) };
        return it == m_Previous.end() ? nullptr : &it->second;
    }

    auto CaptureUIStore::SetPrevious(std::uint32_t handle, std::vector<std::uint8_t> bytes) -> void
    {
        m_Previous[handle] = std::move(bytes);
    }

    auto CaptureUIStore::GetChanges() const -> const std::vector<ByteChange>&
    {
        return m_Changes;
    }

    auto CaptureUIStore::SetChanges(std::vector<ByteChange> changes) -> void
    {
        m_Changes = std::move(changes);
    }

    auto CaptureUIStore::GetCaptureCount() const -> std::uint32_t
    {
        return m_CaptureCount;
    }

    auto CaptureUIStore::SetCaptureCount(std::uint32_t value) -> void
    {
        m_CaptureCount = value;
    }

    auto CaptureUIStore::GetFilePath() const -> const std::filesystem::path&
    {
        return m_FilePath;
    }

    auto CaptureUIStore::SetFilePath(std::filesystem::path value) -> void
    {
        m_FilePath = std::move(value);
    }

    auto CaptureUIStore::GetStatus() const -> const std::string&
    {
        return m_Status;
    }

    auto CaptureUIStore::SetStatus(std::string value) -> void
    {
        m_Status = std::move(value);
    }

    auto CaptureUIStore::GetComponentFilter() -> int&
    {
        return m_ComponentFilter;
    }

    auto CaptureUIStore::ResetSession() -> void
    {
        m_Previous.clear();
        m_Changes.clear();
        m_CaptureCount = 0;
        m_FilePath.clear();
    }
}