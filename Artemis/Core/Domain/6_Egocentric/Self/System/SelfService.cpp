module;

#include <cmath>

module Egocentric.Self.System;

import Egocentric.Self.Type;

namespace
{
    // Placeholder agent name. Centralized here so the day an in-game
    // local-player resolution is found, only ResolveSelfHandle changes.
    constexpr const char* k_SelfPlayerName = "PlaceHolder021";

    using Self = Egocentric::Self::Type::Self;
}

namespace Egocentric::Self::System
{
    auto SelfService::Update() -> void
    {
        auto playerTablePtr = m_PlayerStore.Acquire();
        if (!playerTablePtr) return;

        const std::uint32_t playerHandle = this->ResolveSelfHandle(*playerTablePtr);
        if (playerHandle == 0) return;

        auto it = playerTablePtr->find(playerHandle);
        if (it == playerTablePtr->end()) return;
        const AlivePlayer& player = it->second;

        Vec3 forward{}, right{}, up{};
        this->BuildFrame(player.WeaponForward, forward, right, up);

        ::Self self;
        self.Handle = player.Handle;
        self.BipedHandle = player.CurrentBipedHandle;
        self.IsAlive = player.AliveBipedHandle != 0xFFFFFFFF;
        self.Team = player.Team;
        self.Position = player.WeaponPosition;
        self.Forward = forward;
        self.Right = right;
        self.Up = up;

        m_SelfStore.Publish(std::move(self));
    }

    auto SelfService::ResolveSelfHandle(const PlayerTable& playerTable) const -> std::uint32_t
    {
        for (const auto& [handle, player] : playerTable)
        {
            if (player.Gamertag == k_SelfPlayerName) return handle;
        }

        return 0;
    }

    auto SelfService::BuildFrame(const Vec3& forwardIn,
        Vec3& outForward,
        Vec3& outRight,
        Vec3& outUp) const -> void
    {
        constexpr Vec3 kWorldUp{ 0.0f, 0.0f, 1.0f };
        constexpr float kParallelEpsilon = 1e-4f;

        outForward = this->Normalize(forwardIn);

        Vec3 right = this->Cross(outForward, kWorldUp);

        const float rightLenSq = 
            right.X * right.X + right.Y * right.Y + right.Z * right.Z;

        if (rightLenSq < kParallelEpsilon)
        {
            constexpr Vec3 kWorldX{ 1.0f, 0.0f, 0.0f };
            right = this->Cross(outForward, kWorldX);
        }

        outRight = this->Normalize(right);
        outUp = this->Cross(outRight, outForward);
    }

    auto SelfService::Cross(const Vec3& a, const Vec3& b) const -> Vec3
    {
        return {
            a.Y * b.Z - a.Z * b.Y,
            a.Z * b.X - a.X * b.Z,
            a.X * b.Y - a.Y * b.X
        };
    }

    auto SelfService::Normalize(const Vec3& v) const -> Vec3
    {
        const float lenSq = v.X * v.X + v.Y * v.Y + v.Z * v.Z;
        if (lenSq <= 0.0f) return { 0.0f, 0.0f, 0.0f };

        const float invLen = 1.0f / std::sqrt(lenSq);
        return { v.X * invLen, v.Y * invLen, v.Z * invLen };
    }

    auto SelfService::Cleanup() -> void
    {
        m_SelfStore.Cleanup();

        m_LogsService.Message("[SelfService] INFO: Cleanup completed.");
    }
}