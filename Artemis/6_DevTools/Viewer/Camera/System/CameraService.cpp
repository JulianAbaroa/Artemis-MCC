module DevTools.Viewer.Camera.System;

import Common.Math.Type;
import Common.Math.System;
import DevTools.Viewer.Options.Type;
import std;

namespace
{
    using std::chrono::duration;

    namespace VecMath = Common::Math::System;

    using DevTools::Viewer::Camera::Type::Key;
    using DevTools::Viewer::Camera::Type::Mode;
    using DevTools::Viewer::Camera::Type::Follow;
    using DevTools::Viewer::Options::Type::Scalar;
    using Common::Math::Type::Vec3;

    using Common::Math::Type::k_WorldUp;

    constexpr float k_LookSpeed{ 0.0025f };
    constexpr float k_MaxPitch{ 1.55334f };
    constexpr float k_MaxDeltaTime{ 0.1f };

    constexpr float k_MoveSpeed{ 3.0f };
    constexpr float k_FastSpeed{ 30.0f };

    constexpr float k_DegreesToRadians{ 0.0174532925f };

    // Speed of the field of view transition. Higher is faster.
    constexpr float k_FovSmoothing{ 12.0f };
}

namespace DevTools::Viewer::Camera::System
{
    auto CameraService::Update(const std::shared_ptr<const Tick>& tick,
        const Viewport& viewport, bool acceptInput,
        std::optional<std::uint32_t> followHandle) -> void
    {
        const auto now = steady_clock::now();

        float deltaTime = 0.0f;
        if (m_IsLastUpdateSet)
        {
            deltaTime = duration<float>(now - m_LastUpdate).count();
        }

        m_LastUpdate = now;
        m_IsLastUpdateSet = true;
        deltaTime = std::clamp(deltaTime, 0.0f, k_MaxDeltaTime);

        m_Viewport = viewport;

        if (!m_IsActive)
        {
            m_IsActive = true;
            this->SeatOnPlayer(tick);
        }

        float mouseX = 0.0f, mouseY = 0.0f;
        m_CameraStore.ConsumeMouseDelta(mouseX, mouseY);

        const bool wasOrbiting{ m_IsOrbiting };
        m_IsOrbiting = false;

        Vec3 targetPosition{}, targetForward{};
        if (m_CameraStore.GetMode() == Mode::Game)
        {
            this->FollowSelf(tick);
        }
        else if (const Follow follow{ m_CameraStore.GetFollow() }; followHandle && follow == Follow::FirstPerson &&
            this->FindFollowTarget(tick, *followHandle, targetPosition, targetForward))
        {
            this->AimAlong(targetForward);
            m_Eye = targetPosition;
        }
        else if (followHandle && follow == Follow::Orbit &&
            this->OrbitAround(tick, *followHandle, wasOrbiting, mouseX, mouseY, acceptInput))
        {
            m_IsOrbiting = true;
        }
        else
        {
            this->Fly(deltaTime, mouseX, mouseY, acceptInput);
        }

        this->UpdateLens(deltaTime);
        this->Build();
    }

    auto CameraService::Deactivate() -> void
    {
        m_IsActive = false;
        m_IsLastUpdateSet = false;
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

    auto CameraService::UpdateLens(float deltaTime) -> void
    {
        Scalar scalar{ Scalar::FovNormal };

        if (m_CameraStore.GetMode() == Mode::Game) scalar = Scalar::GameFov;
        else if (m_CameraStore.IsZoomActive()) scalar = Scalar::FovZoom;

        const float target{ m_OptionsStore.GetScalar(scalar) * k_DegreesToRadians };

        if (!m_IsFovSet)
        {
            m_Lens.FovY = target;
            m_IsFovSet = true;
            return;
        }

        const float blend{ 1.0f - std::exp(-k_FovSmoothing * deltaTime) };
        m_Lens.FovY += (target - m_Lens.FovY) * blend;
    }

    auto CameraService::FollowSelf(const std::shared_ptr<const Tick>& tick) -> void
    {
        if (!tick || !tick->Self || !tick->Self->IsAlive) return;

        m_Eye = tick->Self->Position;
        this->AimAlong(tick->Self->Forward);
    }

    auto CameraService::FindFollowTarget(const std::shared_ptr<const Tick>& tick,
        std::uint32_t selected, Vec3& outPosition, Vec3& outForward) const -> bool
    {
        if (!tick) return false;

        if (tick->PlayerTable)
        {
            for (const auto& entry : *tick->PlayerTable)
            {
                const auto& player = entry.second;
                if (player.AliveBipedHandle != selected) continue;

                outPosition = player.CameraPosition;
                outForward = player.CameraForward;
                return true;
            }
        }

        if (tick->Entities)
        {
            if (const auto* entity{ tick->Entities->Find(selected) }; entity != nullptr)
            {
                outPosition = entity->BoundingCenter;
                outForward = entity->Forward;
                return true;
            }
        }

        return false;
    }

    auto CameraService::Look(float mouseX, float mouseY) -> void
    {
        // The look speed follows the field of view, so the aim keeps its angular feel while zoomed.
        float lookScale{ 1.0f };
        const float normalHalf{ std::tan(m_OptionsStore.GetScalar(Scalar::FovNormal) * k_DegreesToRadians * 0.5f) };
        if (normalHalf > 1e-4f) lookScale = std::tan(m_Lens.FovY * 0.5f) / normalHalf;

        const float look{ k_LookSpeed * lookScale };
        this->SetPose(m_Eye, m_Yaw - mouseX * look, m_Pitch - mouseY * look);
        this->Build();
    }

    auto CameraService::FindPivot(const std::shared_ptr<const Tick>& tick,
        std::uint32_t selected, Vec3& outPivot) const -> bool
    {
        if (!tick) return false;

        if (tick->Entities)
        {
            if (const auto* entity{ tick->Entities->Find(selected) }; entity != nullptr)
            {
                outPivot = entity->BoundingCenter;
                return true;
            }
        }

        if (tick->PlayerTable)
        {
            for (const auto& entry : *tick->PlayerTable)
            {
                const auto& player = entry.second;
                if (player.AliveBipedHandle != selected) continue;

                outPivot = player.CameraPosition;
                return true;
            }
        }

        return false;
    }

    auto CameraService::OrbitAround(const std::shared_ptr<const Tick>& tick, std::uint32_t selected,
        bool wasOrbiting, float mouseX, float mouseY, bool acceptInput) -> bool
    {
        Vec3 pivot{};
        if (!this->FindPivot(tick, selected, pivot)) return false;

        if (!wasOrbiting)
        {
            // Starts from where the camera is: it turns toward the object and keeps its distance, so the view does not jump.
            const Vec3 toPivot{ VecMath::Subtract(pivot, m_Eye) };
            const float distance{ VecMath::Length(toPivot) };

            if (distance > 1e-3f)
            {
                m_CameraStore.SetOrbitDistance(distance);
                this->AimAlong(toPivot);
            }
        }

        if (acceptInput) this->Look(mouseX, mouseY);
        else this->Build();

        m_Eye = VecMath::Subtract(pivot, VecMath::Scale(m_Forward, m_CameraStore.GetOrbitDistance()));
        return true;
    }

    auto CameraService::Fly(float deltaTime, float mouseX, float mouseY,
        bool acceptInput) -> void
    {
        if (!acceptInput)
        {
            m_CameraStore.ResetKeys();
            return;
        }

        this->Look(mouseX, mouseY);

        float speed = k_MoveSpeed;
        if (m_CameraStore.IsKeyDown(Key::Fast)) speed = k_FastSpeed;
        if (m_CameraStore.IsKeyDown(Key::Custom)) speed = m_OptionsStore.GetScalar(Scalar::SpeedCustom);

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