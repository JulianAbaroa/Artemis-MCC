export module UI.Capture.State;

import UI.Capture.Type;
import std;

namespace
{
    using UI::Capture::Type::k_LabelSize;
}

export namespace UI::Capture::State
{
    // Inputs, session and last diff of the datum captures.
    // note: Only the capture request is atomic, because the hotkey sets it from the window thread. The rest is used from the render thread.
    class CaptureUIStore
    {
    private:
        using ByteChange = UI::Capture::Type::ByteChange;

    public:
        CaptureUIStore() = default;
        ~CaptureUIStore() = default;

        CaptureUIStore(const CaptureUIStore&) = delete;
        auto operator=(const CaptureUIStore&) -> CaptureUIStore& = delete;

        // Asks for a capture on the next frame.
        auto RequestCapture() -> void;

        // Reads and clears the capture request.
        // return: true if a capture was requested.
        auto ConsumeCaptureRequest() -> bool;

        // Returns the text buffer of the label of the next capture.
        auto GetLabel() -> std::array<char, k_LabelSize>&;

        // Checks whether the object is pinned for the captures.
        auto IsPinned(std::uint32_t handle) const -> bool;

        // Pins or unpins the object.
        auto SetPinned(std::uint32_t handle, bool value) -> void;

        // Returns the pinned handles in the order they were pinned.
        auto GetPinned() const -> const std::vector<std::uint32_t>&;

        // Checks whether the biped of every player is captured, not only the local one.
        auto IsPlayersIncluded() const -> bool;
        auto SetPlayersIncluded(bool value) -> void;

        // return: The bytes of the previous capture of the object, or null if it was not captured in this session.
        auto FindPrevious(std::uint32_t handle) const -> const std::vector<std::uint8_t>*;

        // Keeps the bytes of the object as the base of the next diff.
        auto SetPrevious(std::uint32_t handle, std::vector<std::uint8_t> bytes) -> void;

        // Returns the byte runs that changed in the last capture.
        auto GetChanges() const -> const std::vector<ByteChange>&;
        auto SetChanges(std::vector<ByteChange> changes) -> void;

        // Returns the captures written to the session file.
        auto GetCaptureCount() const -> std::uint32_t;
        auto SetCaptureCount(std::uint32_t value) -> void;

        // return: File of the session, or empty when the next capture starts a new one.
        auto GetFilePath() const -> const std::filesystem::path&;
        auto SetFilePath(std::filesystem::path value) -> void;

        // Returns the result of the last capture shown in the window.
        auto GetStatus() const -> const std::string&;
        auto SetStatus(std::string value) -> void;

        // Returns the index of the component filter of the change table. 0 shows every component.
        auto GetComponentFilter() -> int&;

        // Ends the session: the next capture opens a new file and has no previous bytes to diff against.
        auto ResetSession() -> void;

    private:
        std::atomic<bool> m_IsCaptureRequested{ false };

        std::array<char, k_LabelSize> m_Label{};
        std::vector<std::uint32_t> m_Pinned{};
        bool m_IsPlayersIncluded{ false };

        std::unordered_map<std::uint32_t, std::vector<std::uint8_t>> m_Previous{};
        std::vector<ByteChange> m_Changes{};

        std::uint32_t m_CaptureCount{ 0 };
        std::filesystem::path m_FilePath{};
        std::string m_Status{};
        int m_ComponentFilter{ 0 };
    };
}