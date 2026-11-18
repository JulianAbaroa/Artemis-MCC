module;

#include <windows.h>

module Engine.Base.Memory.System;
import :Camera;

import Common.Math.Type;
import Common.Math.System;
import Engine.Base.Host.Type;
import Engine.Base.Memory.Type;
import std;

namespace
{
    namespace VecMath = Common::Math::System;

    using Common::Math::Type::Vec3;
    using Engine::Base::Memory::Type::GameCamera::GameCamera;

    using Engine::Base::Host::Type::k_GameModuleName;
    using Engine::Base::Memory::Type::GameCamera::k_AxisTolerance;
    using Engine::Base::Memory::Type::GameCamera::k_ProjectionFloatCount;
    using Engine::Base::Memory::Type::GameCamera::k_ProjectionM00;
    using Engine::Base::Memory::Type::GameCamera::k_ProjectionM11;
    using Engine::Base::Memory::Type::GameCamera::k_ProjectionM20;
    using Engine::Base::Memory::Type::GameCamera::k_ProjectionM21;
    using Engine::Base::Memory::Type::GameCamera::k_ViewToWorldBack;
    using Engine::Base::Memory::Type::GameCamera::k_ViewToWorldPosition;
    using Engine::Base::Memory::Type::GameCamera::k_ViewToWorldRight;
    using Engine::Base::Memory::Type::GameCamera::k_ViewToWorldUp;
    using Engine::Base::Memory::Type::ObserverCamera::CameraOverride;
    using Engine::Base::Memory::Type::ObserverCamera::k_OverrideMaxEyeLength;
    using Engine::Base::Memory::Type::ObserverCamera::k_OverrideTolerance;
    using Engine::Base::Memory::Type::Signature::k_RenderProjectionRva;

    auto VecAt(const std::array<float, k_ProjectionFloatCount>& floats, std::size_t index) -> Vec3
    {
        return Vec3{ floats[index], floats[index + 1], floats[index + 2] };
    }

    auto IsUnit(const Vec3& axis) -> bool
    {
        return std::abs(VecMath::Length(axis) - 1.0f) <= k_AxisTolerance;
    }

    auto IsPerpendicular(const Vec3& a, const Vec3& b) -> bool
    {
        return std::abs(VecMath::Dot(a, b)) <= k_AxisTolerance;
    }

    auto IsFinite(const Vec3& value) -> bool
    {
        return std::isfinite(value.X) && std::isfinite(value.Y) && std::isfinite(value.Z);
    }
}

namespace Engine::Base::Memory::System
{
    auto GameCameraService::Read() -> std::optional<GameCamera>
    {
        const HMODULE module{ GetModuleHandleW(k_GameModuleName) };
        if (!module) return std::nullopt;

        std::array<float, k_ProjectionFloatCount> floats{};
        const std::uintptr_t address{ reinterpret_cast<std::uintptr_t>(module) + k_RenderProjectionRva };
        if (!m_Reader.ReadRaw(address, floats.data(), sizeof(floats))) return std::nullopt;

        return this->Parse(floats);
    }

    auto GameCameraService::Parse(const std::array<float, k_ProjectionFloatCount>& floats) -> std::optional<GameCamera>
    {
        // note: Only the floats the camera uses. The bounds and the pixel scale of the block are not read.
        for (std::size_t i = k_ViewToWorldRight; i < k_ViewToWorldPosition + 3; ++i)
        {
            if (!std::isfinite(floats[i])) return std::nullopt;
        }

        for (const std::size_t index : { k_ProjectionM00, k_ProjectionM11, k_ProjectionM20, k_ProjectionM21 })
        {
            if (!std::isfinite(floats[index])) return std::nullopt;
        }

        const Vec3 right{ VecAt(floats, k_ViewToWorldRight) };
        const Vec3 up{ VecAt(floats, k_ViewToWorldUp) };
        const Vec3 back{ VecAt(floats, k_ViewToWorldBack) };

        if (!IsUnit(right) || !IsUnit(up) || !IsUnit(back)) return std::nullopt;
        if (!IsPerpendicular(right, up) || !IsPerpendicular(right, back) || !IsPerpendicular(up, back)) return std::nullopt;

        if (floats[k_ProjectionM00] <= 0.0f || floats[k_ProjectionM11] <= 0.0f) return std::nullopt;

        return GameCamera{
            .Position = VecAt(floats, k_ViewToWorldPosition),
            .Right = right,
            .Up = up,
            .Forward = VecMath::Scale(back, -1.0f),
            .M00 = floats[k_ProjectionM00],
            .M11 = floats[k_ProjectionM11],
            .M20 = floats[k_ProjectionM20],
            .M21 = floats[k_ProjectionM21],
        };
    }

    auto GameCameraService::SetOverride(const Vec3& eye, const Vec3& forward, const Vec3& up) -> bool
    {
        const CameraOverride pose{ .Eye = eye, .Forward = forward, .Up = up };

        if (!this->IsValidOverride(pose))
        {
            m_OverrideStore.Clear();
            return false;
        }

        m_OverrideStore.Publish(pose);
        return true;
    }

    auto GameCameraService::ClearOverride() -> void
    {
        m_OverrideStore.Clear();
    }

    auto GameCameraService::IsOverrideActive() const -> bool
    {
        return m_OverrideStore.IsActive();
    }

    auto GameCameraService::GetOverrideFaultCount() const -> std::uint32_t
    {
        return m_OverrideStore.GetFaultCount();
    }

    auto GameCameraService::IsValidOverride(const CameraOverride& pose) -> bool
    {
        if (!IsFinite(pose.Eye) || !IsFinite(pose.Forward) || !IsFinite(pose.Up)) return false;

        if (VecMath::Length(pose.Eye) >= k_OverrideMaxEyeLength) return false;

        const auto isUnit = [](const Vec3& axis) { return std::abs(VecMath::Length(axis) - 1.0f) <= k_OverrideTolerance; };
        if (!isUnit(pose.Forward) || !isUnit(pose.Up)) return false;

        return std::abs(VecMath::Dot(pose.Forward, pose.Up)) <= k_OverrideTolerance;
    }
}