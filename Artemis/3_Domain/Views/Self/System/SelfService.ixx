export module Views.Self.System;

import Common.Math.Type;
import Common.Math.System;
import Common.Log.System;
import Blam.Runtime.Player.Type;
import Blam.Runtime.Player.State;
import Views.Self.State;
import std;

export namespace Views::Self::System
{
    class SelfService
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using AlivePlayer = Blam::Runtime::Player::Type::Alive::AlivePlayer;
        using PlayerTable = Blam::Runtime::Player::Type::Alive::PlayerTable;

        using Log = Common::Log::System::Log;
        using PlayerTableStore = Blam::Runtime::Player::State::PlayerTableStore;
        using SelfStore = Views::Self::State::SelfStore;

    public:
        SelfService(Log& log, PlayerTableStore& playerStore,
            SelfStore& selfStore) : m_Log(log),
            m_PlayerStore(playerStore), m_SelfStore(selfStore) {}
        ~SelfService() = default;

        auto Update() -> void;

        auto Cleanup() -> void;

    private:
        Log& m_Log;
        PlayerTableStore& m_PlayerStore;
        SelfStore& m_SelfStore;

        // return: Handle of the local player of the agent (A9 M3), or 0 when it is not in the table.
        auto ResolveSelfHandle(const PlayerTable& playerTable) const -> std::uint32_t;
    };
}