export module DevTools.Viewer.Director.State;

import DevTools.Viewer.Director.Type;
import std;

export namespace DevTools::Viewer::Director::State
{
    // Holds the shots of the map, whether the director plays, the requests of the panel and the keys, the map it plays for and where playback is (VW.14).
    // note: Thread-safe. The window thread asks and the render thread answers. The list and the strings are behind one short mutex, the rest is atomic.
    class DirectorStore
    {
    private:
        using DirectorFileInfo = DevTools::Viewer::Director::Type::DirectorFileInfo;
        using DirectorStatus = DevTools::Viewer::Director::Type::DirectorStatus;
        using Shot = DevTools::Viewer::Director::Type::Shot;
        using ShotList = DevTools::Viewer::Director::Type::ShotList;

    public:
        DirectorStore() = default;
        ~DirectorStore() = default;

        DirectorStore(const DirectorStore&) = delete;
        auto operator=(const DirectorStore&) -> DirectorStore& = delete;

        // return: A copy of the list.
        auto CopyShots() const -> ShotList;
        auto GetShotCount() const -> std::size_t;

        // return: A copy of the shot, or nothing when the index is out of range.
        auto GetShot(std::size_t index) const -> std::optional<Shot>;

        // Replaces the list with one that was loaded. It changes the load serial and not the revision, so it is not saved back.
        auto SetShots(ShotList shots) -> void;

        // Each edit changes the revision, which makes the file be saved.
        auto AddShot(Shot shot) -> void;
        auto RemoveShot(std::size_t index) -> void;

        // Swaps the shot with its neighbour delta places away. Does nothing when that is out of range.
        auto MoveShot(std::size_t index, int delta) -> void;
        auto SetShotName(std::size_t index, std::string name) -> void;

        // Does nothing for a value that is not above zero or not finite.
        auto SetShotSeconds(std::size_t index, float seconds) -> void;

        // Only an orbit has a turn speed.
        auto SetShotDegreesPerSecond(std::size_t index, float degreesPerSecond) -> void;

        // return: How many edits the list had, and how many times it was replaced by a load.
        auto GetRevision() const -> std::uint64_t;
        auto GetLoadSerial() const -> std::uint64_t;

        auto IsPlaying() const -> bool;
        auto SetPlaying(bool value) -> void;

        // Requests of the panel and the keys. They are consumed by the render thread, once each.
        auto RequestTogglePlay() -> void;
        auto ConsumeTogglePlay() -> bool;

        // Steps add up. A positive one goes to the next shot and a negative one to the previous.
        auto RequestStep(int delta) -> void;
        auto ConsumeStep() -> int;
        auto RequestAddFixed() -> void;
        auto ConsumeAddFixed() -> bool;
        auto RequestAddOrbit() -> void;
        auto ConsumeAddOrbit() -> bool;

        // Drops every pending request.
        auto ClearRequests() -> void;

        // The map variant that is loaded (empty for none) and the engine starts, as the render thread last saw them. The file of the map follows the name.
        auto SetMapName(std::string name) -> void;
        auto GetMapName() const -> std::string;
        auto SetMatchId(std::uint64_t matchId) -> void;
        auto GetMatchId() const -> std::uint64_t;

        auto SetFileInfo(DirectorFileInfo info) -> void;
        auto GetFileInfo() const -> DirectorFileInfo;

        auto SetStatus(const DirectorStatus& status) -> void;
        auto GetStatus() const -> DirectorStatus;

        // return: The player the director shows in first person, or k_NoHandle.
        auto GetPovPlayer() const -> std::uint32_t;

    private:
        mutable std::mutex m_Mutex{};
        ShotList m_Shots{};
        std::string m_MapName{};
        DirectorFileInfo m_FileInfo{};
        DirectorStatus m_Status{};

        std::atomic<std::uint64_t> m_Revision{ 0 };
        std::atomic<std::uint64_t> m_LoadSerial{ 0 };
        std::atomic<std::uint64_t> m_MatchId{ 0 };
        std::atomic<bool> m_IsPlaying{ false };
        std::atomic<bool> m_IsTogglePending{ false };
        std::atomic<bool> m_IsAddFixedPending{ false };
        std::atomic<bool> m_IsAddOrbitPending{ false };
        std::atomic<int> m_PendingSteps{ 0 };
    };
}