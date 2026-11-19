export module DevTools.Viewer.Inspector.System:Collision;

import Perception.Collidable.Type;
import std;

export namespace DevTools::Viewer::Inspector::System
{
    // Draws the collision section of the selected collidable.
    class CollisionSection
    {
    private:
        using Collidable = Perception::Collidable::Type::Collidable;
        using Collidables = Perception::Collidable::Type::Collidables;

    public:
        CollisionSection() = default;
        ~CollisionSection() = default;

        // return: The collidable with that handle, or null when the object is not a collidable.
        static auto Find(const Collidables& collidables, std::uint32_t handle) -> const Collidable*;

        // Draws the transform, the world mesh and the bounds of the collidable.
        // note: The region states and the raw region block are in the Raw section.
        static auto Draw(const Collidable& instance) -> void;
    };
}