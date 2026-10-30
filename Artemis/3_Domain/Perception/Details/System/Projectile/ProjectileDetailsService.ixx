export module Perception.Details.System:ProjectileDetails;

import Common.Log.System;
import Blam.Runtime.Object.State;
import Perception.Details.Type;
import Perception.Details.State;
import std;

export namespace Perception::Details::System
{
    // Builds the ProjectileDetails: owner, shooter root and what it is stuck to, of every live projectile (D254).
    class ProjectileDetailsService
    {
    private:
        using Log = Common::Log::System::Log;

        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;

        using ProjectileDetailsStore = Perception::Details::State::ProjectileDetailsStore;

    public:
        ProjectileDetailsService(Log& log, ObjectTableStore& objectStore, ProjectileDetailsStore& projectileDetailsStore) :
            m_Log(log), m_ObjectStore(objectStore), m_ProjectileDetailsStore(projectileDetailsStore) {}
        ~ProjectileDetailsService() = default;

        // Publishes the projectile detail of every live object of Kind Projectile.
        // note: Reads only the object table, so it does not depend on the classifier. StuckTo is the parent handle unless it is the owner biped (InHand).
        auto Update() -> void;

        // Drops the published details.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        ObjectTableStore& m_ObjectStore;
        ProjectileDetailsStore& m_ProjectileDetailsStore;
    };
}