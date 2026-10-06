export module Viewer.Selection.System;

import Export.Tick.Type;
import Viewer.Selection.State;
import Viewer.Selection.Type;
import Viewer.Camera.Type;
import std;

export namespace Viewer::Selection::System
{
    // Selects the object under a ray, using the bounding boxes of the collidables of the tick.
    // The world boxes are rebuilt when the tick generation changes. The mesh boxes are kept until Reset.
    class SelectionService
    {
    private:
        using Tick = Export::Tick::Type::Tick;
        using SelectionStore = Viewer::Selection::State::SelectionStore;
        using Ray = Viewer::Camera::Type::Ray;
        using ObjectBounds = Viewer::Selection::Type::ObjectBounds;
        using LocalBounds = Viewer::Selection::Type::LocalBounds;

    public:
        explicit SelectionService(SelectionStore& selectionStore) :
            m_SelectionStore(selectionStore) {}
        ~SelectionService() = default;

        SelectionService(const SelectionService&) = delete;
        auto operator=(const SelectionService&) -> SelectionService& = delete;

        // Rebuilds the boxes if the tick is new, then resolves a pending pick with the ray.
        auto Update(const std::shared_ptr<const Tick>& tick, const Ray& ray) -> void;

        auto GetSelected() const -> std::uint32_t;
        auto Clear() -> void;

        // Drops the cached boxes. The next update with a tick rebuilds them.
        auto Reset() -> void;

    private:
        SelectionStore& m_SelectionStore;

        std::unordered_map<const void*, LocalBounds> m_MeshBounds{};
        std::vector<ObjectBounds> m_Bounds{};

        std::uint64_t m_LastGeneration{ 0 };
        bool m_IsGenerationSet{ false };

        auto Rebuild(const std::shared_ptr<const Tick>& tick) -> void;
        auto Pick(const Ray& ray) -> void;

        // return: Distance along the ray to the box, or -1 if the ray misses it.
        static auto Intersect(const Ray& ray, const ObjectBounds& bounds) -> float;
    };
}