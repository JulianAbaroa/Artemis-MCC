export module Viewer.Camera.State;

import Viewer.Camera.Type;
import std;

export namespace Viewer::Camera::State
{
	class CameraStore
	{
	private:
		using Key = Viewer::Camera::Type::Key;

		static constexpr std::size_t k_KeyCount = static_cast<std::size_t>(Key::Count);

	public:
		CameraStore() = default;
		~CameraStore() = default;

		auto IsActive() const -> bool;
		auto SetActive(bool value) -> void;

		auto IsFollowEnabled() const -> bool;
		auto SetFollowEnabled(bool value) -> void;

		auto IsKeyDown(Key key) const -> bool;
		auto SetKey(Key key, bool down) -> void;
		auto ResetKeys() -> void;

		auto AccumulateMouseDelta(float dx, float dy) -> void;

		auto ConsumeMouseDelta(float& outX, float& outY) -> void;

	private:
		std::atomic<bool> m_IsActive{ false };
		std::atomic<bool> m_IsFollowEnabled{ false };

		std::array<std::atomic<bool>, k_KeyCount> m_Keys{};

		std::atomic<float> m_MouseDeltaX{ 0.0f };
		std::atomic<float> m_MouseDeltaY{ 0.0f };
	};
}