export module DevTools.Viewer.Inspector.System;

export import :Action;
export import :Body;
export import :Collision;
export import :Entity;
export import :Fixture;
export import :Interaction;
export import :Loadout;
export import :Name;
export import :Player;
export import :Raw;
export import :View;
export import :Vitality;

import Engine.Control.Input.State;
import Engine.Control.Simulation.State;
import DevTools.Viewer.Collidable.State;
import DevTools.Viewer.Scene.System;
import DevTools.Viewer.Selection.State;
import std;

export namespace DevTools::Viewer::Inspector::System
{
    // Draws the inspector of the selected object: a summary and the sections that apply to it, folded.
    // note: It has no window, the caller draws it inside its own child or tab. It runs on the thread that draws ImGui (Present).
    class InspectorService
    {
    private:
        using PlayerActionStore = Engine::Control::Input::State::PlayerActionStore;
        using BotStore = Engine::Control::Simulation::State::BotStore;
        using CollidableStore = DevTools::Viewer::Collidable::State::CollidableStore;
        using SelectionStore = DevTools::Viewer::Selection::State::SelectionStore;
        using SceneService = DevTools::Viewer::Scene::System::SceneService;

    public:
        // param playerActionStore: Only read, by the Action section.
        // param botStore: Only read, to tell whether a bot writes the action.
        InspectorService(SceneService& sceneService, CollidableStore& collidableStore, SelectionStore& selectionStore,
            const PlayerActionStore& playerActionStore, const BotStore& botStore) :
            m_SceneService(sceneService), m_CollidableStore(collidableStore), m_SelectionStore(selectionStore),
            m_PlayerActionStore(playerActionStore), m_BotStore(botStore) {}
        ~InspectorService() = default;

        InspectorService(const InspectorService&) = delete;
        auto operator=(const InspectorService&) -> InspectorService& = delete;

        // Draws the summary and the sections of the selected object.
        // note: Reads the scene of the frame once. Nothing is drawn but a disabled line without a selection or without a tick.
        auto Draw() -> void;

    private:
        SceneService& m_SceneService;
        CollidableStore& m_CollidableStore;
        SelectionStore& m_SelectionStore;
        const PlayerActionStore& m_PlayerActionStore;
        const BotStore& m_BotStore;
    };
}