export module DevTools.Viewer.Inspector.System:Action;

import Engine.Control.Input.State;
import Engine.Control.Simulation.State;
import Perception.Players.Type;
import std;

export namespace DevTools::Viewer::Inspector::System
{
    // Draws the action section: the action the simulation used for a player in the latest tick, and who wrote it.
    class ActionSection
    {
    private:
        using PlayerActionStore = Engine::Control::Input::State::PlayerActionStore;
        using BotStore = Engine::Control::Simulation::State::BotStore;
        using Player = Perception::Players::Type::Player;

    public:
        ActionSection() = default;
        ~ActionSection() = default;

        // Draws the buttons, the facing and the throttle of the action, and whether it comes from Python (an override), a bot or the engine.
        // note: Read only. It copies the stores, so it is called only while the section is open.
        static auto Draw(const Player& player, const PlayerActionStore& actionStore, const BotStore& botStore) -> void;
    };
}