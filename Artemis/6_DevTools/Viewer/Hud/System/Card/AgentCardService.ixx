export module DevTools.Viewer.Hud.System:AgentCard;

import Engine.Control.Simulation.Type;
import Engine.Control.Simulation.State;
import DevTools.Viewer.Collidable.State;
import DevTools.Viewer.Scene.System;
import DevTools.Viewer.Options.State;
import DevTools.Viewer.Selection.State;
import std;

export namespace DevTools::Viewer::Hud::System
{
    // Draws the card of the selected player in a corner of the game frame (VW.4c, D338, D339): name, vitality, shield and weapon, and at the level Full
    // the rest of its loadout.
    class AgentCardService
    {
    private:
        using Bot = Engine::Control::Simulation::Type::Bot;
        using BotStore = Engine::Control::Simulation::State::BotStore;
        using CollidableStore = DevTools::Viewer::Collidable::State::CollidableStore;
        using SceneService = DevTools::Viewer::Scene::System::SceneService;
        using OptionsStore = DevTools::Viewer::Options::State::OptionsStore;
        using SelectionStore = DevTools::Viewer::Selection::State::SelectionStore;

    public:
        // param botStore: Only read, to mark the bots.
        AgentCardService(CollidableStore& collidableStore, SceneService& sceneService, OptionsStore& optionsStore,
            SelectionStore& selectionStore, const BotStore& botStore) :
            m_CollidableStore(collidableStore), m_SceneService(sceneService), m_OptionsStore(optionsStore),
            m_SelectionStore(selectionStore), m_BotStore(botStore) {}
        ~AgentCardService() = default;

        AgentCardService(const AgentCardService&) = delete;
        auto operator=(const AgentCardService&) -> AgentCardService& = delete;

        // Draws the card of the player whose biped is selected, with the Tick of the frame scene.
        // note: Does nothing at the level Off, without a selection or when the selection is not the biped of a player (never the local player by default).
        // The caller gives the same gate as the labels (active Viewer and a scene updated in this frame).
        auto Draw() -> void;

    private:
        CollidableStore& m_CollidableStore;
        SceneService& m_SceneService;
        OptionsStore& m_OptionsStore;
        SelectionStore& m_SelectionStore;
        const BotStore& m_BotStore;

        std::vector<Bot> m_Bots{};
    };
}