module Egocentric.Self.System;

import Egocentric.Self.Type;

namespace
{
    using Self = Egocentric::Self::Type::Self;

    using Egocentric::Self::Type::k_SelfLocalIndex;
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
        Common::Math::System::BuildFrame(player.CameraForward, forward, right, up);

        ::Self self;
        self.Handle = player.Handle;
        self.BipedHandle = player.CurrentBipedHandle;
        self.IsAlive = player.AliveBipedHandle != 0xFFFFFFFF;
        self.Team = player.Team;
        self.Position = player.CameraPosition;
        self.Forward = forward;
        self.Right = right;
        self.Up = up;

        m_SelfStore.Publish(std::move(self));
    }

    auto SelfService::ResolveSelfHandle(const PlayerTable& playerTable) const -> std::uint32_t
    {
        for (const auto& [handle, player] : playerTable)
        {
            if (player.LocalIndex == k_SelfLocalIndex) return handle;
        }

        return 0;
    }

    auto SelfService::Cleanup() -> void
    {
        m_SelfStore.Cleanup();

        m_LogsService.Message("[SelfService] INFO: Cleanup completed.");
    }
}