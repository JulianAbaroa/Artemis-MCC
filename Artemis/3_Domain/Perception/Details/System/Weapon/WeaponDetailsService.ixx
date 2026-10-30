export module Perception.Details.System:WeaponDetails;

import Common.Log.System;
import Blam.MapModel.Definitions.State;
import Blam.Runtime.Game.State;
import Blam.Runtime.Object.State;
import Perception.Details.Type;
import Perception.Details.State;
import std;

export namespace Perception::Details::System
{
    // Builds the WeaponDetails: ammo, energy, heat, overheat, reload, charge and tracking of every live weapon (D254).
    class WeaponDetailsService
    {
    private:
        using Log = Common::Log::System::Log;

        using DefinitionsStore = Blam::MapModel::Definitions::State::DefinitionsStore;
        using GameStore = Blam::Runtime::Game::State::GameStore;
        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;

        using WeaponChargeMemoryStore = Perception::Details::State::WeaponChargeMemoryStore;
        using WeaponDetailsStore = Perception::Details::State::WeaponDetailsStore;

    public:
        WeaponDetailsService(Log& log, DefinitionsStore& definitionsStore, ObjectTableStore& objectStore, GameStore& gameStore,
            WeaponChargeMemoryStore& chargeMemoryStore, WeaponDetailsStore& weaponDetailsStore) :
            m_Log(log), m_DefinitionsStore(definitionsStore), m_ObjectStore(objectStore), m_GameStore(gameStore),
            m_ChargeMemoryStore(chargeMemoryStore), m_WeaponDetailsStore(weaponDetailsStore) {}
        ~WeaponDetailsService() = default;

        // Publishes the weapon detail of every live object of Kind Weapon.
        // note: Reads the object table, the tag of each weapon and the engine tick (for the charge), not the classifier.
        // IsReloading needs the reload state of the magazine and the busy timer. CanFireNow needs the busy timer at 0, which also covers
        // the time a weapon takes to be ready after it is drawn.
        auto Update() -> void;

        // Drops the published details and the charge memory.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        DefinitionsStore& m_DefinitionsStore;
        ObjectTableStore& m_ObjectStore;
        GameStore& m_GameStore;
        WeaponChargeMemoryStore& m_ChargeMemoryStore;
        WeaponDetailsStore& m_WeaponDetailsStore;
    };
}