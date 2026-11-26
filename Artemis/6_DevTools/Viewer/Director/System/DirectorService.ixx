export module DevTools.Viewer.Director.System;

export import :Shots;
export import :Library;

import Common.Engine.Type;
import Common.Log.System;
import DevTools.Viewer.Director.Type;
import DevTools.Viewer.Director.State;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

export namespace DevTools::Viewer::Director::System
{
    // Plays the playlist of the map (the active sequence, or the library) in real time, one shot after the other, and keeps the file of the map (VW.14,
    // VW.16).
    // note: The pose of a shot only depends on the real clock (steady_clock), never on the game time, so the camera is smooth while the training runs at
    // x9. Update is for the render thread at the scene point, Persist and Observe for the one at Present: they share only the store. The camera, the
    // selection and the POV request are applied by the caller from the DirectorFrame.
    class DirectorService
    {
    private:
        using Log = Common::Log::System::Log;

        using DirectorFile = DevTools::Viewer::Director::Type::DirectorFile;
        using DirectorFrame = DevTools::Viewer::Director::Type::DirectorFrame;
        using DirectorInput = DevTools::Viewer::Director::Type::DirectorInput;
        using RoundKey = DevTools::Viewer::Director::Type::RoundKey;
        using ShotIndexList = DevTools::Viewer::Director::Type::ShotIndexList;
        using DirectorStore = DevTools::Viewer::Director::State::DirectorStore;

    public:
        DirectorService(DirectorStore& store, Log& log) : m_Store(store), m_Log(log) {}
        ~DirectorService() = default;

        DirectorService(const DirectorService&) = delete;
        auto operator=(const DirectorService&) -> DirectorService& = delete;

        // Loads the file of the map when its key changes and saves it after every edit.
        // param directory: The Artemis folder of LOCALAPPDATA. Empty keeps the file in memory only. The file is <directory>\Director\<key>.txt, with the
        // key made from the map name of the store (SanitizeMapKey).
        // note: Called at Present, so an edit made with the Viewer inactive is saved too. A file that is missing is an empty one, a file that cannot be
        // read or written leaves its message in the file info of the store. A file of the first version is migrated when it is read, and the first save
        // keeps the old text next to it as <key>.txt.v1.bak.
        auto Persist(const std::string& directory) -> void;

        // Watches the match key (the engine start count) and, when a match starts that the director must play from the load, asks the store to play from
        // the first shot of the playlist. It does that for Auto-play and for the sequence the bench asked for (SetLoadRequest of the store). The first key
        // seen is a start too, except zero, which is no match.
        // return: Whether it asked to play, so the caller makes the Viewer active before the game shows the camera of the local player.
        // note: Called at Present, after Persist, so the file of the map is already there.
        auto Observe(std::uint64_t matchId, bool isAutoPlay) -> bool;

        // Answers the requests of the store (play or stop, next or previous shot, play from the start), moves to the next shot when the current one is
        // over and gives the pose of the camera for this frame. Without playing it gives an empty frame.
        // note: The POV agent is chosen when a POV shot starts. A playlist with nothing to play makes it wait and try again every k_RetrySeconds (every frame
        // while there is no Tick, so a POV shot starts the frame the roster is known), the camera stays where it is and nothing loops in a frame. A Play
        // from the panel with an empty playlist does not start, a start for a new match waits for shots.
        auto Update(const DirectorInput& input) -> DirectorFrame;

        // Stops playing and drops the pending requests. Called when the Viewer is deactivated.
        auto Stop() -> void;

    private:
        DirectorStore& m_Store;
        Log& m_Log;

        // Persistence.
        std::string m_LoadedKey{};
        std::string m_LoadedDirectory{};
        bool m_IsLoaded{ false };
        bool m_IsFirstVersionOnDisk{ false };
        std::uint64_t m_SavedRevision{ 0 };

        // Match start.
        std::optional<std::uint64_t> m_ObservedMatch{};

        // Playback. The file and the playlist are a copy of the store, taken again when it changes.
        bool m_IsRunning{ false };
        bool m_IsHolding{ false };
        bool m_IsCacheDirty{ true };
        std::size_t m_Index{ 0 };
        std::uint32_t m_ShotId{ 0 };
        std::uint32_t m_RoundChanges{ 0 };
        std::uint64_t m_CacheRevision{ 0 };
        std::uint64_t m_CacheLoadSerial{ 0 };
        DirectorFile m_File{};
        ShotIndexList m_Playlist{};
        std::string m_RequestedSequence{};
        std::string m_WarnedSequence{};
        std::uint64_t m_UpdateMatchId{ 0 };
        std::uint32_t m_PovHandle{ k_NoHandle };
        std::uint32_t m_LastPovAgent{ k_NoHandle };
        std::optional<RoundKey> m_LastRound{};
        std::chrono::steady_clock::time_point m_ShotStart{};
        std::chrono::steady_clock::time_point m_RetryAt{};

        // Reads the file of the key into the store.
        auto Load(const std::string& directory, const std::string& key) -> void;

        // Writes the file of the key.
        auto Save(const std::string& directory, const std::string& key) -> void;

        // Forgets the sequence the bench asked for, so the active one plays again.
        auto ClearRequestedSequence() -> void;

        // Takes the file of the store again when it changed (an edit, a load or a new requested sequence) and makes the playlist from it.
        // return: Whether the library was replaced by a load, so the playback starts again from the first shot.
        auto Refresh() -> bool;

        // return: The playlist position of the shot that plays, which an edit may have moved, or nothing when the edit took it out of the playlist.
        auto LocateCurrent() -> std::optional<std::size_t>;

        // Starts the first shot that can be played, looking at the playlist position first and then going by direction. Without one it holds and tries
        // again later.
        auto Begin(std::size_t first, int direction, const DirectorInput& input) -> void;

        // Starts playing from the first shot. return: False when the playlist is empty and the start is not for a new match.
        auto Start(const DirectorInput& input, const std::optional<RoundKey>& round, bool isForNewMatch) -> bool;

        // return: The frame of a shot that is playing, or the frame of a wait when there is none.
        auto Publish(const DirectorInput& input) -> DirectorFrame;
    };
}