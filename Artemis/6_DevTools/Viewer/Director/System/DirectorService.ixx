export module DevTools.Viewer.Director.System;

export import :Shots;

import Common.Engine.Type;
import DevTools.Viewer.Director.Type;
import DevTools.Viewer.Director.State;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

export namespace DevTools::Viewer::Director::System
{
    // Plays the shots of the map in real time, one after the other, and keeps the file of the map (VW.14).
    // note: The pose of a shot only depends on the real clock (steady_clock), never on the game time, so the camera is smooth while the training runs at
    // x9. Render thread only. The camera, the selection and the POV request are applied by the caller from the DirectorFrame.
    class DirectorService
    {
    private:
        using DirectorFrame = DevTools::Viewer::Director::Type::DirectorFrame;
        using DirectorInput = DevTools::Viewer::Director::Type::DirectorInput;
        using RoundKey = DevTools::Viewer::Director::Type::RoundKey;
        using ShotList = DevTools::Viewer::Director::Type::ShotList;
        using DirectorStore = DevTools::Viewer::Director::State::DirectorStore;

    public:
        explicit DirectorService(DirectorStore& store) : m_Store(store) {}
        ~DirectorService() = default;

        DirectorService(const DirectorService&) = delete;
        auto operator=(const DirectorService&) -> DirectorService& = delete;

        // Loads the file of the map when its key changes and saves it after every edit.
        // param directory: The Artemis folder of LOCALAPPDATA. Empty keeps the list in memory only. The file is <directory>\Director\<key>.txt, with the
        // key made from the map name of the store (SanitizeMapKey).
        // note: Called at Present, so an edit made with the Viewer inactive is saved too. A file that is missing is an empty list, a file that cannot be
        // read or written leaves its message in the file info of the store.
        auto Persist(const std::string& directory) -> void;

        // Answers the requests of the store (play or stop, next or previous shot), moves to the next shot when the current one is over and gives the pose
        // of the camera for this frame. Without playing it gives an empty frame.
        // note: The POV agent is chosen when a POV shot starts. A list with nothing to play makes it wait and try again every k_RetrySeconds, so the camera
        // stays where it is and nothing loops in a frame.
        auto Update(const DirectorInput& input) -> DirectorFrame;

        // Stops playing and drops the pending requests. Called when the Viewer is deactivated.
        auto Stop() -> void;

    private:
        DirectorStore& m_Store;

        // Persistence.
        std::string m_LoadedKey{};
        std::string m_LoadedDirectory{};
        bool m_IsLoaded{ false };
        std::uint64_t m_SavedRevision{ 0 };

        // Playback.
        bool m_IsRunning{ false };
        bool m_IsHolding{ false };
        std::size_t m_Index{ 0 };
        std::uint64_t m_SeenLoadSerial{ 0 };
        std::uint32_t m_PovHandle{ k_NoHandle };
        std::uint32_t m_LastPovAgent{ k_NoHandle };
        std::optional<RoundKey> m_LastRound{};
        std::chrono::steady_clock::time_point m_ShotStart{};
        std::chrono::steady_clock::time_point m_RetryAt{};

        // Reads the file of the key into the store.
        auto Load(const std::string& directory, const std::string& key) -> void;

        // Writes the list to the file of the key.
        auto Save(const std::string& directory, const std::string& key) -> void;

        // Starts the first shot that can be played, looking at first and then going by direction. Without one it holds and tries again later.
        auto Begin(const ShotList& shots, std::size_t first, int direction, const DirectorInput& input) -> void;

        // Starts playing. return: False when the list is empty.
        auto Start(const DirectorInput& input, const std::optional<RoundKey>& round) -> bool;

        // return: The frame of a shot that is playing, or the frame of a wait when there is none.
        auto Publish(const DirectorInput& input) -> DirectorFrame;
    };
}