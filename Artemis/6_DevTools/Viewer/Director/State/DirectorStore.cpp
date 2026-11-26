module DevTools.Viewer.Director.State;

namespace DevTools::Viewer::Director::State
{
    auto DirectorStore::CopyFile() const -> DirectorFile
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_File;
    }

    auto DirectorStore::SetFile(DirectorFile file) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_File = std::move(file);
        this->RaiseNextShotId();
        m_LoadSerial.fetch_add(1);
    }

    auto DirectorStore::Edit(const std::function<bool(DirectorFile&)>& edit) -> bool
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        if (!edit(m_File)) return false;

        this->RaiseNextShotId();
        m_Revision.fetch_add(1);
        return true;
    }

    auto DirectorStore::AllocateShotId() -> std::uint32_t
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        this->RaiseNextShotId();
        return m_NextShotId++;
    }

    auto DirectorStore::RaiseNextShotId() -> void
    {
        for (const auto& shot : m_File.Shots) m_NextShotId = (std::max)(m_NextShotId, shot.Id + 1);
    }

    auto DirectorStore::GetRevision() const -> std::uint64_t
    {
        return m_Revision.load();
    }

    auto DirectorStore::GetLoadSerial() const -> std::uint64_t
    {
        return m_LoadSerial.load();
    }

    auto DirectorStore::IsPlaying() const -> bool
    {
        return m_IsPlaying.load();
    }

    auto DirectorStore::SetPlaying(bool value) -> void
    {
        m_IsPlaying.store(value);
    }

    auto DirectorStore::RequestTogglePlay() -> void
    {
        m_IsTogglePending.store(true);
    }

    auto DirectorStore::ConsumeTogglePlay() -> bool
    {
        return m_IsTogglePending.exchange(false);
    }

    auto DirectorStore::RequestStep(int delta) -> void
    {
        m_PendingSteps.fetch_add(delta);
    }

    auto DirectorStore::ConsumeStep() -> int
    {
        return m_PendingSteps.exchange(0);
    }

    auto DirectorStore::RequestAddFixed() -> void
    {
        m_IsAddFixedPending.store(true);
    }

    auto DirectorStore::ConsumeAddFixed() -> bool
    {
        return m_IsAddFixedPending.exchange(false);
    }

    auto DirectorStore::RequestAddOrbit() -> void
    {
        m_IsAddOrbitPending.store(true);
    }

    auto DirectorStore::ConsumeAddOrbit() -> bool
    {
        return m_IsAddOrbitPending.exchange(false);
    }

    auto DirectorStore::RequestPlayFromStart(std::string sequence) -> void
    {
        {
            std::lock_guard<std::mutex> lock{ m_Mutex };
            m_PlayFromStart = std::move(sequence);
        }

        m_IsPlaying.store(true);
    }

    auto DirectorStore::ConsumePlayFromStart() -> std::optional<std::string>
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return std::exchange(m_PlayFromStart, std::nullopt);
    }

    auto DirectorStore::SetLoadRequest(std::string sequence, std::uint64_t afterStarts) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_LoadRequest = std::move(sequence);
        m_LoadRequestAfterStarts = afterStarts;
    }

    auto DirectorStore::TakeLoadRequest(std::uint64_t observedStarts) -> std::optional<std::string>
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        if (!m_LoadRequest || observedStarts <= m_LoadRequestAfterStarts) return std::nullopt;

        return std::exchange(m_LoadRequest, std::nullopt);
    }

    auto DirectorStore::ClearLoadRequest() -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_LoadRequest.reset();
    }

    auto DirectorStore::ClearRequests() -> void
    {
        m_IsTogglePending.store(false);
        m_IsAddFixedPending.store(false);
        m_IsAddOrbitPending.store(false);
        m_PendingSteps.store(0);

        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_PlayFromStart.reset();
    }

    auto DirectorStore::SetMapName(std::string name) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_MapName = std::move(name);
    }

    auto DirectorStore::GetMapName() const -> std::string
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_MapName;
    }

    auto DirectorStore::SetMatchId(std::uint64_t matchId) -> void
    {
        m_MatchId.store(matchId);
    }

    auto DirectorStore::GetMatchId() const -> std::uint64_t
    {
        return m_MatchId.load();
    }

    auto DirectorStore::SetFileInfo(DirectorFileInfo info) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_FileInfo = std::move(info);
    }

    auto DirectorStore::GetFileInfo() const -> DirectorFileInfo
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_FileInfo;
    }

    auto DirectorStore::SetStatus(const DirectorStatus& status) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Status = status;
    }

    auto DirectorStore::GetStatus() const -> DirectorStatus
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };

        DirectorStatus status{ m_Status };
        status.IsPlaying = m_IsPlaying.load();
        return status;
    }

    auto DirectorStore::GetPovPlayer() const -> std::uint32_t
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_Status.PovPlayer;
    }
}