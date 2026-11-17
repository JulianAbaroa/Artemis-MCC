export module Views.View.System;

import Common.Log.System;
import Blam.MapModel.Definitions.State;
import Blam.Runtime.Object.State;
import Perception.Details.State;
import Perception.Entities.State;
import Perception.Players.State;
import Views.View.Type;
import Views.View.State;
import std;

export namespace Views::View::System
{
    // Builds the View of every agent of the set, one per row of Players (F4, D307).
    // note: F4.1 fills the head and Self. The other sections are zero until F4.2 to F4.6.
    class ViewsService
    {
    private:
        using Log = Common::Log::System::Log;
        using DefinitionsStore = Blam::MapModel::Definitions::State::DefinitionsStore;
        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;
        using ObjectiveDetailsStore = Perception::Details::State::ObjectiveDetailsStore;
        using WeaponDetailsStore = Perception::Details::State::WeaponDetailsStore;
        using EntitiesStore = Perception::Entities::State::EntitiesStore;
        using PlayersStore = Perception::Players::State::PlayersStore;
        using ViewsStore = Views::View::State::ViewsStore;
        using AgentSet = Views::View::Type::AgentSet;

    public:
        ViewsService(Log& log, ObjectTableStore& objectStore, DefinitionsStore& definitionsStore,
            EntitiesStore& entitiesStore, PlayersStore& playersStore, WeaponDetailsStore& weaponDetailsStore,
            ObjectiveDetailsStore& objectiveDetailsStore, ViewsStore& viewsStore) : m_Log(log),
            m_ObjectStore(objectStore), m_DefinitionsStore(definitionsStore), m_EntitiesStore(entitiesStore),
            m_PlayersStore(playersStore), m_WeaponDetailsStore(weaponDetailsStore),
            m_ObjectiveDetailsStore(objectiveDetailsStore), m_ViewsStore(viewsStore) {}
        ~ViewsService() = default;

        // Publishes the Views of the tick: a computed one for each player of the set, a zero one for the others.
        // note: Reads the stores the same tick published, so it runs after Perception.
        auto Update(const AgentSet& agents) -> void;

        auto Cleanup() -> void;

    private:
        Log& m_Log;
        ObjectTableStore& m_ObjectStore;
        DefinitionsStore& m_DefinitionsStore;
        EntitiesStore& m_EntitiesStore;
        PlayersStore& m_PlayersStore;
        WeaponDetailsStore& m_WeaponDetailsStore;
        ObjectiveDetailsStore& m_ObjectiveDetailsStore;
        ViewsStore& m_ViewsStore;
    };
}