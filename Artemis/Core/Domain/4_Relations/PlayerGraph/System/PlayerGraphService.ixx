export module Relations.PlayerGraph.System;

import Service.Logs.System;
import Tables.Object.Type;
import Tables.Object.State;
import Tables.Player.Type;
import Tables.Player.State;
import Relations.Classifier.State;
import Relations.ObjectGraph.Type;
import Relations.ObjectGraph.State;
import Relations.PlayerGraph.Type;
import Relations.PlayerGraph.State;
import std;

export namespace Relations::PlayerGraph::System
{
    class PlayerGraphService
    {
    private:
        using AliveObject = Tables::Object::Type::Alive::Object;
        using AlivePlayer = Tables::Player::Type::Alive::Player;
        using ObjectNode = Relations::ObjectGraph::Type::ObjectNode;
        using PlayerTree = Relations::PlayerGraph::Type::PlayerTree;

        using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;
        using PlayerTable = std::unordered_map<std::uint32_t, AlivePlayer>;
        using ObjectNodes = std::unordered_map<std::uint32_t, ObjectNode>;
        using PlayerTrees = std::vector<PlayerTree>;

        using LogsService = Service::Logs::System::LogsService;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;
        using PlayerTableStore = Tables::Player::State::PlayerTableStore;
        using ClassifierStore = Relations::Classifier::State::ClassifierStore;
        using ObjectGraphStore = Relations::ObjectGraph::State::ObjectGraphStore;
        using PlayerGraphStore = Relations::PlayerGraph::State::PlayerGraphStore;

    public:
        PlayerGraphService(LogsService& logsService, ObjectTableStore& objectStore,
            PlayerTableStore& playerStore, ClassifierStore& classifierStore,
            ObjectGraphStore& objectGraphStore, PlayerGraphStore& playerGraphStore) :
            m_LogsService(logsService), m_ObjectStore(objectStore),
            m_PlayerStore(playerStore), m_ClassifierStore(classifierStore),
            m_ObjectGraphStore(objectGraphStore), m_PlayerGraphStore(playerGraphStore) {}
        ~PlayerGraphService() = default;

        auto UpdateGraph() -> void;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        ObjectTableStore& m_ObjectStore;
        PlayerTableStore& m_PlayerStore;
        ClassifierStore& m_ClassifierStore;
        ObjectGraphStore& m_ObjectGraphStore;
        PlayerGraphStore& m_PlayerGraphStore;

        auto BuildPlayerTrees(PlayerTrees& playerGraph,
            const PlayerTable& playerTable, const ObjectTable& objectTable) -> void;

        auto BuildVehicle(PlayerTree& playerTree,
            const ObjectNodes& objectGraph, const ObjectTable& objectTable) -> void;
    };
}