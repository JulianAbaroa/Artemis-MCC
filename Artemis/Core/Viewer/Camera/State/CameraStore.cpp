module Viewer.Camera.State;

namespace Viewer::Camera::State
{
	auto CameraStore::IsActive() const -> bool
	{
		return m_IsActive.load();
	}

	auto CameraStore::SetActive(bool value) -> void
	{
		m_IsActive.store(value);
	}

	auto CameraStore::IsFollowEnabled() const -> bool
	{
		return m_IsFollowEnabled.load(std::memory_order_relaxed);
	}

	auto CameraStore::SetFollowEnabled(bool value) -> void
	{
		m_IsFollowEnabled.store(value, std::memory_order_relaxed);
	}

	auto CameraStore::IsKeyDown(Key key) const -> bool
	{
		return m_Keys[static_cast<std::size_t>(key)].load(std::memory_order_relaxed);
	}

	auto CameraStore::SetKey(Key key, bool down) -> void
	{
		m_Keys[static_cast<std::size_t>(key)].store(down, std::memory_order_relaxed);
	}

	auto CameraStore::ResetKeys() -> void
	{
		for (auto& key : m_Keys) key.store(false, std::memory_order_relaxed);
	}

	auto CameraStore::AccumulateMouseDelta(float dx, float dy) -> void
	{
		m_MouseDeltaX.fetch_add(dx, std::memory_order_relaxed);
		m_MouseDeltaY.fetch_add(dy, std::memory_order_relaxed);
	}

	auto CameraStore::ConsumeMouseDelta(float& outX, float& outY) -> void
	{
		outX = m_MouseDeltaX.exchange(0.0f, std::memory_order_relaxed);
		outY = m_MouseDeltaY.exchange(0.0f, std::memory_order_relaxed);
	}
}