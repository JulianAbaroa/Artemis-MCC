export module Viewer.Selection.System;

import Environment.Collidable.State;
import Viewer.Selection.State;
import Viewer.Selection.Type;
import Viewer.Camera.Type;
import std;

export namespace Viewer::Selection::System
{
    // Selects the object under a ray, using the bounding boxes of the debug collidables.
    // The world boxes are rebuilt when the collidables are published again. The mesh boxes are kept until Reset.
    // note: The collidables are not part of the Tick (07, D84). The AI thread builds them only while the free camera is active.
    class SelectionService
    {
    private:
        using Collidables = Environment::Collidable::State::Collidables;
        using CollidableStore = Environment::Collidable::State::CollidableStore;

        using SelectionStore = Viewer::Selection::State::SelectionStore;
        using Ray = Viewer::Camera::Type::Ray;
        using ObjectBounds = Viewer::Selection::Type::ObjectBounds;
        using LocalBounds = Viewer::Selection::Type::LocalBounds;

    public:
        SelectionService(CollidableStore& collidableStore, SelectionStore& selectionStore) :
            m_CollidableStore(collidableStore), m_SelectionStore(selectionStore) {}
        ~SelectionService() = default;

        SelectionService(const SelectionService&) = delete;
        auto operator=(const SelectionService&) -> SelectionService& = delete;

        // Rebuilds the boxes if the collidables are new, then resolves a pending pick with the ray.
        auto Update(const Ray& ray) -> void;

        auto GetSelected() const -> std::uint32_t;
        auto Clear() -> void;

        // Drops the cached boxes. The next update rebuilds them.
        auto Reset() -> void;

    private:
        CollidableStore& m_CollidableStore;
        SelectionStore& m_SelectionStore;

        std::unordered_map<const void*, LocalBounds> m_MeshBounds{};
        std::vector<ObjectBounds> m_Bounds{};

        // Snapshot the boxes were built from. Kept to know when a new one is published.
        std::shared_ptr<const Collidables> m_Collidables{};

        auto Rebuild(const std::shared_ptr<const Collidables>& collidables) -> void;
        auto Pick(const Ray& ray) -> void;

        // return: Distance along the ray to the box, or -1 if the ray misses it.
        static auto Intersect(const Ray& ray, const ObjectBounds& bounds) -> float;
    };
}