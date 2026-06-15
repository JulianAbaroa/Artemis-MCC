#include "pch.h"

#include "System_Self.h"

#include "Core/Types/Sources/Tables/Player/LivePlayer.h"

#include "Core/States/Sources/Tables/Player/State_PlayerTable.h"
#include "Core/States/Egocentric/Self/State_Self.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

#include <cmath>

namespace
{
    // Placeholder agent name. Centralized here so the day an in-game
    // local-player resolution is found, only ResolveSelfHandle changes.
    constexpr const char* k_SelfPlayerName = "PlaceHolder021";
}

void System_Self::Update()
{
    auto playerTablePtr = m_Deps.State_PlayerTable.Acquire();
    if (!playerTablePtr) return;

    const uint32_t selfHandle = 
        this->ResolveSelfHandle(*playerTablePtr);
    if (selfHandle == 0) return;

    auto it = playerTablePtr->find(selfHandle);
    if (it == playerTablePtr->end()) return;
    const LivePlayer& self = it->second;

    std::array<float, 3> forward{}, right{}, up{};
    this->BuildFrame(self.WeaponForward, forward, right, up);

    Self state;
    state.Handle = self.Handle;
    state.BipedHandle = self.CurrentBipedHandle;
    state.IsAlive = self.AliveBipedHandle != 0xFFFFFFFF;
    state.Team = self.Team;
    state.Position = self.WeaponPosition;
    state.Forward = forward;
    state.Right = right;
    state.Up = up;

    m_Deps.State_Self.Publish(std::move(state));
}

uint32_t System_Self::ResolveSelfHandle(const PlayerTable& players) const
{
    for (const auto& [handle, player] : players)
    {
        if (player.Gamertag == k_SelfPlayerName) return handle;
    }

    return 0;
}

void System_Self::BuildFrame(const std::array<float, 3>& forwardIn,
    std::array<float, 3>& outForward,
    std::array<float, 3>& outRight,
    std::array<float, 3>& outUp) const
{
    constexpr std::array<float, 3> kWorldUp{ 0.0f, 0.0f, 1.0f };
    constexpr float kParallelEpsilon = 1e-4f;

    outForward = this->Normalize(forwardIn);

    std::array<float, 3> right = this->Cross(outForward, kWorldUp);

    const float rightLenSq =
        right[0] * right[0] + right[1] * right[1] + right[2] * right[2];

    if (rightLenSq < kParallelEpsilon)
    {
        constexpr std::array<float, 3> kWorldX{ 1.0f, 0.0f, 0.0f };
        right = this->Cross(outForward, kWorldX);
    }

    outRight = this->Normalize(right);
    outUp = this->Cross(outRight, outForward);
}

std::array<float, 3> System_Self::Cross(const std::array<float, 3>& a,
    const std::array<float, 3>& b) const
{
    return {
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]
    };
}

std::array<float, 3> System_Self::Normalize(const std::array<float, 3>& v) const
{
    const float lenSq = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    if (lenSq <= 0.0f) return { 0.0f, 0.0f, 0.0f };

    const float invLen = 1.0f / std::sqrt(lenSq);
    return { v[0] * invLen, v[1] * invLen, v[2] * invLen };
}

void System_Self::Cleanup()
{
    m_Deps.State_Self.Cleanup();

    m_Deps.System_Logs.Log("[Self] INFO: Cleanup completed.");
}