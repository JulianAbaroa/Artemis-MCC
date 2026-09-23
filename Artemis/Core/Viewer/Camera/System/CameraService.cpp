module Viewer.Camera.System;

import Viewer.Math.System;
import std;

namespace
{
	namespace VecMath = Viewer::Math::System;

	using Key = Viewer::Camera::Type::Key;
	using Vec3 = Viewer::Camera::Type::Vec3;

	constexpr float k_LookSpeed = 0.0025f;
	constexpr float k_MaxPitch = 1.55334f;
	constexpr float k_MaxDeltaTime = 0.1f;

	constexpr float k_MoveSpeed = 3.0f;
	constexpr float k_FastSpeed = 30.0f;
	constexpr float k_SlowSpeed = 0.6f;

	constexpr Vec3 k_WorldUp{ 0.0f, 0.0f, 1.0f };
}

namespace Viewer::Camera::System
{
	auto CameraService::Update(const std::shared_ptr<const Tick>& tick,
		const Viewport& viewport, bool acceptInput) -> void
	{
		const auto now = SteadyClock::now();

		float deltaTime = 0.0f;
		if (m_HasLastUpdate)
		{
			deltaTime = std::chrono::duration<float>(now - m_LastUpdate).count();
		}

		m_LastUpdate = now;
		m_HasLastUpdate = true;
		deltaTime = std::clamp(deltaTime, 0.0f, k_MaxDeltaTime);

		m_Viewport = viewport;

		if (!m_WasActive)
		{
			m_WasActive = true;
			this->SeatOnPlayer(tick);
		}

		float mouseX = 0.0f, mouseY = 0.0f;
		m_CameraStore.ConsumeMouseDelta(mouseX, mouseY);

		Vec3 targetPosition{}, targetForward{};
		if (m_CameraStore.IsFollowEnabled() &&
			this->FindFollowTarget(tick, targetPosition, targetForward))
		{
			this->AimAlong(targetForward);
			m_Eye = targetPosition;
		}
		else
		{
			this->Fly(deltaTime, mouseX, mouseY, acceptInput);
		}

		this->Build();
	}

	auto CameraService::Deactivate() -> void
	{
		m_WasActive = false;
		m_HasLastUpdate = false;
	}

	auto CameraService::SeatOnPlayer(const std::shared_ptr<const Tick>& tick) -> void
	{
		Vec3 eye{};

		m_Yaw = 0.0f;
		m_Pitch = 0.0f;

		if (tick && tick->Self)
		{
			eye = tick->Self->Position;
			this->AimAlong(tick->Self->Forward);
		}

		m_Eye = eye;
	}

	auto CameraService::FindFollowTarget(const std::shared_ptr<const Tick>& tick,
		Vec3& outPosition, Vec3& outForward) const -> bool
	{
		using Viewer::Selection::State::k_NoSelection;

		const std::uint32_t selected = m_SelectionStore.GetSelected();
		if (!tick || selected == k_NoSelection) return false;

		if (tick->PlayerTable)
		{
			for (const auto& entry : *tick->PlayerTable)
			{
				const auto& player = entry.second;
				if (player.AliveBipedHandle != selected) continue;

				outPosition = player.WeaponPosition;
				outForward = player.WeaponForward;
				return true;
			}
		}

		if (tick->Collidables)
		{
			for (const auto& collidable : *tick->Collidables)
			{
				if (collidable.Handle != selected) continue;

				outPosition = collidable.Position;
				outForward = collidable.Forward;
				return true;
			}
		}

		return false;
	}

	auto CameraService::Fly(float deltaTime, float mouseX, float mouseY,
		bool acceptInput) -> void
	{
		if (!acceptInput)
		{
			m_CameraStore.ResetKeys();
			return;
		}

		this->SetPose(m_Eye, m_Yaw - mouseX * k_LookSpeed, m_Pitch - mouseY * k_LookSpeed);
		this->Build();

		float speed = k_MoveSpeed;
		if (m_CameraStore.IsKeyDown(Key::Fast)) speed = k_FastSpeed;
		if (m_CameraStore.IsKeyDown(Key::Slow)) speed = k_SlowSpeed;

		const float step = speed * deltaTime;

		Vec3 eye = m_Eye;
		if (m_CameraStore.IsKeyDown(Key::Forward)) eye = VecMath::Add(eye, VecMath::Scale(m_Forward, step));
		if (m_CameraStore.IsKeyDown(Key::Backward)) eye = VecMath::Subtract(eye, VecMath::Scale(m_Forward, step));
		if (m_CameraStore.IsKeyDown(Key::Right)) eye = VecMath::Add(eye, VecMath::Scale(m_Right, step));
		if (m_CameraStore.IsKeyDown(Key::Left)) eye = VecMath::Subtract(eye, VecMath::Scale(m_Right, step));
		if (m_CameraStore.IsKeyDown(Key::Up)) eye = VecMath::Add(eye, VecMath::Scale(k_WorldUp, step));
		if (m_CameraStore.IsKeyDown(Key::Down)) eye = VecMath::Subtract(eye, VecMath::Scale(k_WorldUp, step));

		m_Eye = eye;
	}

	auto CameraService::SetPose(const Vec3& eye, float yaw, float pitch) -> void
	{
		m_Eye = eye;
		m_Yaw = yaw;
		m_Pitch = std::clamp(pitch, -k_MaxPitch, k_MaxPitch);
	}

	auto CameraService::AimAlong(const Vec3& forward) -> void
	{
		const float length = VecMath::Length(forward);
		if (length < 1e-6f) return;

		m_Yaw = std::atan2(forward.Y, forward.X);
		m_Pitch = std::clamp(std::asin(std::clamp(forward.Z / length, -1.0f, 1.0f)),
			-k_MaxPitch, k_MaxPitch);
	}

	auto CameraService::Build() -> void
	{
		const float cosPitch = std::cos(m_Pitch);
		const float sinPitch = std::sin(m_Pitch);

		m_Forward = VecMath::Normalize(Vec3{
			cosPitch * std::cos(m_Yaw), cosPitch * std::sin(m_Yaw), sinPitch });

		m_Right = VecMath::Normalize(VecMath::Cross(m_Forward, k_WorldUp));
		m_Up = VecMath::Cross(m_Right, m_Forward);

		const Vec3& f = m_Forward;
		const Vec3& r = m_Right;
		const Vec3& u = m_Up;

		const Matrix view = {
			r.X, u.X, -f.X, 0.0f,
			r.Y, u.Y, -f.Y, 0.0f,
			r.Z, u.Z, -f.Z, 0.0f,
			-VecMath::Dot(r, m_Eye), -VecMath::Dot(u, m_Eye), VecMath::Dot(f, m_Eye), 1.0f,
		};

		const float aspect = m_Viewport.Size.Y > 0.0f ?
			m_Viewport.Size.X / m_Viewport.Size.Y : 1.0f;

		const float focal = 1.0f / std::tan(m_Lens.FovY * 0.5f);
		const float range = m_Lens.Near - m_Lens.Far;

		const Matrix projection = {
			focal / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, focal, 0.0f, 0.0f,
			0.0f, 0.0f, m_Lens.Far / range, -1.0f,
			0.0f, 0.0f, m_Lens.Near * m_Lens.Far / range, 0.0f,
		};

		for (int row = 0; row < 4; ++row)
		{
			for (int column = 0; column < 4; ++column)
			{
				float sum = 0.0f;

				for (int k = 0; k < 4; ++k)
				{
					sum += view[row * 4 + k] * projection[k * 4 + column];
				}

				m_ViewProjection[row * 4 + column] = sum;
			}
		}
	}

	auto CameraService::GetEye() const -> Vec3
	{
		return m_Eye;
	}

	auto CameraService::GetForward() const -> Vec3
	{
		return m_Forward;
	}

	auto CameraService::GetRight() const -> Vec3
	{
		return m_Right;
	}

	auto CameraService::GetUp() const -> Vec3
	{
		return m_Up;
	}

	auto CameraService::GetYaw() const -> float
	{
		return m_Yaw;
	}

	auto CameraService::GetPitch() const -> float
	{
		return m_Pitch;
	}

	auto CameraService::GetViewport() const -> const Viewport&
	{
		return m_Viewport;
	}

	auto CameraService::GetViewProjection() const -> const Matrix&
	{
		return m_ViewProjection;
	}

	auto CameraService::ToScreen(const Vec3& world, Vec2& outPixel) const -> bool
	{
		const Matrix& m = m_ViewProjection;

		const float clipX = world.X * m[0] + world.Y * m[4] + world.Z * m[8] + m[12];
		const float clipY = world.X * m[1] + world.Y * m[5] + world.Z * m[9] + m[13];
		const float clipW = world.X * m[3] + world.Y * m[7] + world.Z * m[11] + m[15];

		if (clipW <= 1e-6f) return false;

		const float ndcX = clipX / clipW;
		const float ndcY = clipY / clipW;

		outPixel.X = m_Viewport.Position.X + (ndcX * 0.5f + 0.5f) * m_Viewport.Size.X;
		outPixel.Y = m_Viewport.Position.Y + (1.0f - (ndcY * 0.5f + 0.5f)) * m_Viewport.Size.Y;

		return true;
	}

	auto CameraService::ScreenToRay(const Vec2& pixel) const -> Ray
	{
		if (m_Viewport.Size.X <= 0.0f || m_Viewport.Size.Y <= 0.0f)
		{
			return Ray{ m_Eye, m_Forward };
		}

		const float u = (pixel.X - m_Viewport.Position.X) / m_Viewport.Size.X;
		const float v = (pixel.Y - m_Viewport.Position.Y) / m_Viewport.Size.Y;

		const float ndcX = u * 2.0f - 1.0f;
		const float ndcY = (1.0f - v) * 2.0f - 1.0f;

		const float tanHalf = std::tan(m_Lens.FovY * 0.5f);
		const float aspect = m_Viewport.Size.X / m_Viewport.Size.Y;

		const Vec3 direction = VecMath::Add(m_Forward,
			VecMath::Add(VecMath::Scale(m_Right, ndcX * tanHalf * aspect),
				VecMath::Scale(m_Up, ndcY * tanHalf)));

		return Ray{ m_Eye, VecMath::Normalize(direction, m_Forward) };
	}

	auto CameraService::GetCenterRay() const -> Ray
	{
		return Ray{ m_Eye, m_Forward };
	}
}