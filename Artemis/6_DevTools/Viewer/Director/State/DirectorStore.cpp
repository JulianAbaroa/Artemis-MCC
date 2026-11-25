module DevTools.Viewer.Director.State;

namespace
{
    using DevTools::Viewer::Director::Type::ShotKind;
}

namespace DevTools::Viewer::Director::State
{
    auto DirectorStore::CopyShots() const -> ShotList
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_Shots;
    }

    auto DirectorStore::GetShotCount() const -> std::size_t
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_Shots.size();
    }

    auto DirectorStore::GetShot(std::size_t index) const -> std::optional<Shot>
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        if (index >= m_Shots.size()) return std::nullopt;

        return m_Shots[index];
    }

    auto DirectorStore::SetShots(ShotList shots) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Shots = std::move(shots);
        m_LoadSerial.fetch_add(1);
    }

    auto DirectorStore::AddShot(Shot shot) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Shots.push_back(std::move(shot));
        m_Revision.fetch_add(1);
    }

    auto DirectorStore::RemoveShot(std::size_t index) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        if (index >= m_Shots.size()) return;

        m_Shots.erase(m_Shots.begin() + static_cast<std::ptrdiff_t>(index));
        m_Revision.fetch_add(1);
    }

    auto DirectorStore::MoveShot(std::size_t index, int delta) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };

        const std::ptrdiff_t target{ static_cast<std::ptrdiff_t>(index) + delta };
        if (index >= m_Shots.size() || target < 0 || target >= static_cast<std::ptrdiff_t>(m_Shots.size())) return;

        std::swap(m_Shots[index], m_Shots[static_cast<std::size_t>(target)]);
        m_Revision.fetch_add(1);
    }

    auto DirectorStore::SetShotName(std::size_t index, std::string name) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        if (index >= m_Shots.size()) return;

        m_Shots[index].Name = std::move(name);
        m_Revision.fetch_add(1);
    }

    auto DirectorStore::SetShotSeconds(std::size_t index, float seconds) -> void
    {
        if (!(seconds > 0.0f) || !std::isfinite(seconds)) return;

        std::lock_guard<std::mutex> lock{ m_Mutex };
        if (index >= m_Shots.size()) return;

        m_Shots[index].Seconds = seconds;
        m_Revision.fetch_add(1);
    }

    auto DirectorStore::SetShotDegreesPerSecond(std::size_t index, float degreesPerSecond) -> void
    {
        if (!std::isfinite(degreesPerSecond)) return;

        std::lock_guard<std::mutex> lock{ m_Mutex };
        if (index >= m_Shots.size() || m_Shots[index].Kind != ShotKind::Orbit) return;

        m_Shots[index].DegreesPerSecond = degreesPerSecond;
        m_Revision.fetch_add(1);
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

    auto DirectorStore::ClearRequests() -> void
    {
        m_IsTogglePending.store(false);
        m_IsAddFixedPending.store(false);
        m_IsAddOrbitPending.store(false);
        m_PendingSteps.store(0);
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