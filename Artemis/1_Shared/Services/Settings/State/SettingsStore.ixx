export module Services.Settings.State;

import std;

export namespace Services::Settings::State
{
    // One unit of the every-tick demand of the SettingsStore (Q101), held while the owner needs the pipeline on every tick.
    // note: Move-only. It lowers the demand once, when it is reset or destroyed, so a demand cannot be lowered twice. It must not outlive the
    // SettingsStore that gave it.
    class EveryTickDemandToken
    {
    public:
        EveryTickDemandToken() = default;
        ~EveryTickDemandToken()
        {
            this->Reset();
        }

        EveryTickDemandToken(const EveryTickDemandToken&) = delete;
        auto operator=(const EveryTickDemandToken&) -> EveryTickDemandToken& = delete;

        EveryTickDemandToken(EveryTickDemandToken&& other) noexcept : m_Counter(std::exchange(other.m_Counter, nullptr)) {}

        // Lowers the demand this token holds, then takes the one of the other.
        auto operator=(EveryTickDemandToken&& other) noexcept -> EveryTickDemandToken&
        {
            if (this != &other)
            {
                this->Reset();
                m_Counter = std::exchange(other.m_Counter, nullptr);
            }

            return *this;
        }

        // return: Whether the token holds a demand.
        auto IsHeld() const -> bool
        {
            return m_Counter != nullptr;
        }

        // Lowers the demand this token holds. Does nothing when it holds none.
        auto Reset() -> void
        {
            if (std::atomic<std::uint32_t>* counter{ std::exchange(m_Counter, nullptr) }) counter->fetch_sub(1);
        }

    private:
        friend class SettingsStore;

        explicit EveryTickDemandToken(std::atomic<std::uint32_t>& counter) : m_Counter(&counter)
        {
            counter.fetch_add(1);
        }

        std::atomic<std::uint32_t>* m_Counter{ nullptr };
    };

    // Shared runtime settings.
    // note: Thread-safe. Flags are atomics and path strings are guarded by a mutex.
    class SettingsStore
    {
    public:
        SettingsStore() = default;
        ~SettingsStore() = default;

        // Whether user data is stored in the AppData folder.
        auto IsAppDataEnabled() const -> bool;
        auto SetAppDataEnabled(bool value) -> void;

        // Whether the launcher or any tab is open.
        auto IsMenuVisible() const -> bool;
        auto SetMenuVisible(bool value) -> void;

        // Whether menu windows are locked in place.
        auto IsMenuLocked() const -> bool;
        auto SetMenuLocked(bool value) -> void;

        // One-shot request to reset the tabs. The launcher clears it once applied.
        auto IsMenuResetPending() const -> bool;
        auto SetMenuResetPending(bool value) -> void;

        // Whether game mouse input is swallowed while the menu is visible.
        auto IsMouseFreezeEnabled() const -> bool;
        auto SetMouseFreezeEnabled(bool value) -> void;

        // Whether the launcher opens automatically on start.
        auto IsOpenUIOnStartEnabled() const -> bool;
        auto SetOpenUIOnStartEnabled(bool value) -> void;

        // Opacity of the menu windows.
        auto GetMenuAlpha() const -> float;
        auto SetMenuAlpha(float value) -> void;

        // Global UI scale factor.
        auto GetUIScale() const -> float;
        auto SetUIScale(float value) -> void;

        // Whether the logs tab follows new entries.
        auto IsLogsAutoScrollEnabled() const -> bool;
        auto SetLogsAutoScrollEnabled(bool value) -> void;

        // Folder of the DLL.
        auto GetBaseDirectory() const -> std::string;
        auto SetBaseDirectory(const std::string& directory) -> void;

        // Artemis folder inside LOCALAPPDATA. Empty until created.
        auto GetAppDataDirectory() const -> std::string;
        auto SetAppDataDirectory(const std::string& directory) -> void;

        // Full path of the log file.
        auto GetLoggerPath() const -> std::string;
        auto SetLoggerPath(const std::string& path) -> void;

        auto ClearAppDataDirectory() -> void;
        auto IsAppDataDirectoryEmpty() const -> bool;

        // Recorders and tools that need the pipeline on every tick of the engine, even when the client asked for sparse mode (Q101).
        // return: The token of the new demand, which the caller keeps while it needs every tick. The demand ends when the token is reset or destroyed.
        // note: A counter of tokens, not a flag. The AI thread and the bridge thread read it.
        auto RaiseEveryTickDemand() -> EveryTickDemandToken;
        // return: How many users need every tick. 0 = nobody.
        auto GetEveryTickDemand() const -> std::uint32_t;

    private:
        std::atomic<bool> m_IsAppDataEnabled{ false };
        std::atomic<bool> m_IsMenuVisible{ false };
        std::atomic<bool> m_IsMenuLocked{ false };
        std::atomic<bool> m_IsMenuResetPending{ false };
        std::atomic<bool> m_IsMouseFreezeEnabled{ true };
        std::atomic<bool> m_IsOpenUIOnStartEnabled{ false };
        std::atomic<float> m_MenuAlpha{ 1.0f };
        std::atomic<float> m_UIScale{ 1.0f };
        std::atomic<bool> m_IsLogsAutoScrollEnabled{ true };
        std::atomic<std::uint32_t> m_EveryTickDemand{ 0 };

        std::string m_BaseDirectory{};
        std::string m_AppDataDirectory{};
        std::string m_LoggerPath{};
        // Guards the three path strings.
        mutable std::mutex m_Mutex{};
    };
}