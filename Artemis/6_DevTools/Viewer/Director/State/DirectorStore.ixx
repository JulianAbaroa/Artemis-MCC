export module DevTools.Viewer.Director.State;

import DevTools.Viewer.Director.Type;
import std;

export namespace DevTools::Viewer::Director::State
{
    // Holds the file of the map (the shot library, the sequences and the active one), whether the director plays, the requests of the panel, the keys and the
    // bench, the map it plays for and where playback is (VW.14, VW.16).
    // note: Thread-safe. The window thread asks and the render thread answers. The file and the strings are behind one short mutex, the rest is atomic.
    class DirectorStore
    {
    private:
        using DirectorFile = DevTools::Viewer::Director::Type::DirectorFile;
        using DirectorFileInfo = DevTools::Viewer::Director::Type::DirectorFileInfo;
        using DirectorStatus = DevTools::Viewer::Director::Type::DirectorStatus;

    public:
        DirectorStore() = default;
        ~DirectorStore() = default;

        DirectorStore(const DirectorStore&) = delete;
        auto operator=(const DirectorStore&) -> DirectorStore& = delete;

        // return: A copy of the file.
        auto CopyFile() const -> DirectorFile;

        // Replaces the file with one that was loaded. It changes the load serial and not the revision, so it is not saved back.
        auto SetFile(DirectorFile file) -> void;

        // Runs the edit on the file under the lock. The pure functions of Director.System (MoveShot, AddShot, RemoveShot...) are the edits.
        // param edit: Changes the file and returns whether it did.
        // return: What the edit returned. A true makes the revision change, which makes the file be saved.
        auto Edit(const std::function<bool(DirectorFile&)>& edit) -> bool;

        // return: A shot id that is above every one of the file and was never given in this session, so an id is not reused after its shot is deleted.
        auto AllocateShotId() -> std::uint32_t;

        // return: How many edits the file had, and how many times it was replaced by a load.
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

        // Starts playing from the first shot of the playlist, for a match that just loaded. An empty sequence plays the active one of the file.
        auto RequestPlayFromStart(std::string sequence) -> void;
        auto ConsumePlayFromStart() -> std::optional<std::string>;

        // The sequence the bench asks for the match that follows the engine start count it gives. The request belongs to the first start above that
        // count: TakeLoadRequest gives it only to a start that is newer, so a start that was already there (or the one of another match) never takes it.
        // Nothing but a take, ClearLoadRequest or a new request removes it.
        auto SetLoadRequest(std::string sequence, std::uint64_t afterStarts) -> void;
        auto TakeLoadRequest(std::uint64_t observedStarts) -> std::optional<std::string>;
        auto ClearLoadRequest() -> void;

        // Drops every pending request of the panel and the keys (not the one of the bench).
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
        DirectorFile m_File{};
        std::string m_MapName{};
        DirectorFileInfo m_FileInfo{};
        DirectorStatus m_Status{};
        std::optional<std::string> m_PlayFromStart{};
        std::optional<std::string> m_LoadRequest{};
        std::uint64_t m_LoadRequestAfterStarts{ 0 };
        std::uint32_t m_NextShotId{ 1 };

        std::atomic<std::uint64_t> m_Revision{ 0 };
        std::atomic<std::uint64_t> m_LoadSerial{ 0 };
        std::atomic<std::uint64_t> m_MatchId{ 0 };
        std::atomic<bool> m_IsPlaying{ false };
        std::atomic<bool> m_IsTogglePending{ false };
        std::atomic<bool> m_IsAddFixedPending{ false };
        std::atomic<bool> m_IsAddOrbitPending{ false };
        std::atomic<int> m_PendingSteps{ 0 };

        // Keeps the next id above the ids of the file. The lock is held by the caller.
        auto RaiseNextShotId() -> void;
    };
}