export module DevTools.Viewer.Inspector.System:Entity;

import Perception.Entities.Type;
import Bridge.Codec.Tick.Type;
import std;

export namespace DevTools::Viewer::Inspector::System
{
    // Draws the entity section of the selected object: function, binding, physics, team, vitality and capabilities.
    class EntitySection
    {
    private:
        using Tick = Bridge::Codec::Tick::Type::Tick;
        using Entity = Perception::Entities::Type::Entity;

    public:
        EntitySection() = default;
        ~EntitySection() = default;

        // return: The row of the object in Entities, or null when it has none.
        static auto Find(const Tick& tick, std::uint32_t handle) -> const Entity*;

        static auto Draw(const Entity& entity) -> void;
    };
}