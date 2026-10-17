export module UI.Simulation.System;

import Platform.Host.State;
import Platform.Input.Type;
import Platform.Input.State;
import UI.Simulation.Type;
import UI.Simulation.State;
import std;

export namespace UI::Simulation::System
{
    // Draws the simulation tab (D107): local players (split-screen) and their input. Game speed joins it when A8 resumes.
    class SimulationUIService
    {
    private:
        using LocalUserStore = Platform::Host::State::LocalUserStore;

        using InputState = Platform::Input::Type::InputState;
        using VirtualInputStore = Platform::Input::State::VirtualInputStore;

        using LocalPlayerInput = UI::Simulation::Type::LocalPlayerInput;
        using SimulationUIStore = UI::Simulation::State::SimulationUIStore;

    public:
        SimulationUIService(LocalUserStore& localUserStore, VirtualInputStore& virtualInputStore,
            SimulationUIStore& simulationUIStore) :
            m_LocalUserStore(localUserStore), m_VirtualInputStore(virtualInputStore),
            m_SimulationUIStore(simulationUIStore) {}
        ~SimulationUIService() = default;

        SimulationUIService(const SimulationUIService&) = delete;
        auto operator=(const SimulationUIService&) -> SimulationUIService& = delete;

        // Draws the content of the simulation window.
        auto Draw() -> void;

    private:
        LocalUserStore& m_LocalUserStore;
        VirtualInputStore& m_VirtualInputStore;
        SimulationUIStore& m_SimulationUIStore;

        // Draws the local player count and one row per player.
        auto DrawLocalPlayers() -> void;

        // Draws the name, the engine status and the input combo of a player.
        auto DrawLocalPlayerRow(std::size_t player, std::size_t requestedCount) -> void;

        // Applies an input choice to the virtual input of a player.
        auto ApplyLocalPlayerInput(std::size_t player, LocalPlayerInput input) -> void;

        // return: The virtual state of a test input. Empty for Idle.
        static auto BuildTestState(LocalPlayerInput input) -> InputState;
    };
}