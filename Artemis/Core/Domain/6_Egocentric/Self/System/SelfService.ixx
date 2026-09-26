export module Egocentric.Self.System;

import Service.Logs.System;
import Common.Math.Type;
import Common.Math.System;
import Tables.Player.Type;
import Tables.Player.State;
import Egocentric.Self.State;
import std;

export namespace Egocentric::Self::System
{
    class SelfService
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using AlivePlayer = Tables::Player::Type::Alive::Player;
        using PlayerTable = std::unordered_map<std::uint32_t, AlivePlayer>;

        using LogsService = Service::Logs::System::LogsService;
        using PlayerTableStore = Tables::Player::State::PlayerTableStore;
        using SelfStore = Egocentric::Self::State::SelfStore;

    public:
        SelfService(LogsService& logsService, PlayerTableStore& playerStore, 
            SelfStore& selfStore) : m_LogsService(logsService), 
            m_PlayerStore(playerStore), m_SelfStore(selfStore) {}
        ~SelfService() = default;

        auto Update() -> void;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        PlayerTableStore& m_PlayerStore;
        SelfStore& m_SelfStore;

        // TODO: replace name lookup with in-game local-player resolution.
        auto ResolveSelfHandle(const PlayerTable& playerTable) const -> std::uint32_t;
    };
}