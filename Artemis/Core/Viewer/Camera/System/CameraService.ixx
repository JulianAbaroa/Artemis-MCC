export module Viewer.Camera.System;

import Export.Tick.Type;
import Viewer.Camera.Type;
import Viewer.Camera.State;
import Viewer.Selection.State;
import std;

export namespace Viewer::Camera::System
{
	class CameraService
	{
	private:
		using SteadyClock = std::chrono::steady_clock;
		using Tick = Export::Tick::Type::Tick;
		using Vec2 = Viewer::Camera::Type::Vec2;
		using Vec3 = Viewer::Camera::Type::Vec3;
		using Matrix = Viewer::Camera::Type::Matrix;
		using Lens = Viewer::Camera::Type::Lens;
		using Viewport = Viewer::Camera::Type::Viewport;
		using Ray = Viewer::Camera::Type::Ray;
		using CameraStore = Viewer::Camera::State::CameraStore;
		using SelectionStore = Viewer::Selection::State::SelectionStore;

	public:
		CameraService(CameraStore& cameraStore, SelectionStore& selectionStore) :
			m_CameraStore(cameraStore), m_SelectionStore(selectionStore) {}
		~CameraService() = default;

		CameraService(const CameraService&) = delete;
		CameraService& operator=(const CameraService&) = delete;

		auto Update(const std::shared_ptr<const Tick>& tick,
			const Viewport& viewport, bool acceptInput) -> void;

		auto Deactivate() -> void;

		auto GetEye() const -> Vec3;
		auto GetForward() const -> Vec3;
		auto GetRight() const -> Vec3;
		auto GetUp() const -> Vec3;
		auto GetYaw() const -> float;
		auto GetPitch() const -> float;

		auto GetViewport() const -> const Viewport&;
		auto GetViewProjection() const -> const Matrix&;

		auto ToScreen(const Vec3& world, Vec2& outPixel) const -> bool;

		auto ScreenToRay(const Vec2& pixel) const -> Ray;

		auto GetCenterRay() const -> Ray;

	private:
		CameraStore& m_CameraStore;
		SelectionStore& m_SelectionStore;

		Vec3 m_Eye{};
		float m_Yaw{ 0.0f };
		float m_Pitch{ 0.0f };
		Lens m_Lens{};

		Vec3 m_Forward{ 1.0f, 0.0f, 0.0f };
		Vec3 m_Right{ 0.0f, -1.0f, 0.0f };
		Vec3 m_Up{ 0.0f, 0.0f, 1.0f };

		Viewport m_Viewport{};
		Matrix m_ViewProjection{};

		bool m_WasActive{ false };
		bool m_HasLastUpdate{ false };
		SteadyClock::time_point m_LastUpdate{};

		auto SeatOnPlayer(const std::shared_ptr<const Tick>& tick) -> void;
		auto FindFollowTarget(const std::shared_ptr<const Tick>& tick,
			Vec3& outPosition, Vec3& outForward) const -> bool;
		auto Fly(float deltaTime, float mouseX, float mouseY, bool acceptInput) -> void;

		auto SetPose(const Vec3& eye, float yaw, float pitch) -> void;
		auto AimAlong(const Vec3& forward) -> void;
		auto Build() -> void;
	};
}