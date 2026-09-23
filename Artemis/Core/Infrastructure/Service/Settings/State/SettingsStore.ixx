export module Service.Settings.State;

import std;

export namespace Service::Settings::State
{
	class SettingsStore
	{
	public:
		SettingsStore() = default;
		~SettingsStore() = default;

		auto ShouldUseAppData() const -> bool;
		auto SetUseAppData(bool value) -> void;

		auto IsMenuVisible() const -> bool;
		auto SetMenuVisible(bool value) -> void;

		auto IsMenuLocked() const -> bool;
		auto SetMenuLocked(bool value) -> void;

		auto MustResetMenu() const -> bool;
		auto SetForceMenuReset(bool value) -> void;

		auto ShouldFreezeMouse() const -> bool;
		auto SetFreezeMouse(bool value) -> void;

		auto ShouldOpenUIOnStart() const -> bool;
		auto SetOpenUIOnStart(bool value) -> void;

		auto GetMenuAlpha() const -> float;
		auto SetMenuAlpha(float value) -> void;

		auto GetUIScale() const -> float;
		auto SetUIScale(float value) -> void;

		auto GetLogsAutoScroll() const -> bool;
		auto SetLogsAutoScroll(bool value) -> void;

		auto GetBaseDirectory() const -> std::string;
		auto SetBaseDirectory(const std::string& directory) -> void;

		auto GetAppDataDirectory() const -> std::string;
		auto SetAppDataDirectory(const std::string& directory) -> void;

		auto GetLoggerPath() const -> std::string;
		auto SetLoggerPath(const std::string& directory) -> void;

		auto ClearAppDataDirectory() -> void;
		auto IsAppDataDirectoryEmpty() const -> bool;

	private:
		std::atomic<bool> m_UseAppData{ false };
		std::atomic<bool> m_IsMenuVisible{ false };
		std::atomic<bool> m_IsMenuLocked{ false };
		std::atomic<bool> m_MustResetMenu{ false };
		std::atomic<bool> m_ShouldFreezeMouse{ true };
		std::atomic<bool> m_ShouldOpenUIOnStart{ false };
		std::atomic<float> m_MenuAlpha{ 1.0f };
		std::atomic<float> m_UIScale{ 1.0f };
		std::atomic<bool> m_LogsAutoScroll{ true };

		std::string m_BaseDirectory{};
		std::string m_AppDataDirectory{};
		std::string m_LoggerPath{};
		mutable std::mutex m_Mutex{};
	};
}