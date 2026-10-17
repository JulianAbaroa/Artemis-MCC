export module UI.Simulation.System;

import Service.Telemetry.State;
import Platform.Lifecycle.State;
import Platform.Host.State;
import Platform.Input.Type;
import Platform.Input.State;
import UI.Simulation.Type;
import UI.Simulation.State;
import std;

export namespace UI::Simulation::System
{
    // Draws the simulation tab (D107): game speed, and local players (split-screen) with their input.
    class SimulationUIService
    {
    private:
        using TelemetryStore = Service::Telemetry::State::TelemetryStore;

        using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;
        using LocalUserStore = Platform::Host::State::LocalUserStore;

        using InputState = Platform::Input::Type::InputState;
        using VirtualInputStore = Platform::Input::State::VirtualInputStore;

        using LocalPlayerInput = UI::Simulation::Type::LocalPlayerInput;
        using SimulationUIStore = UI::Simulation::State::SimulationUIStore;

    public:
        SimulationUIService(TelemetryStore& telemetryStore, LifecycleStore& lifecycleStore, LocalUserStore& localUserStore,
            VirtualInputStore& virtualInputStore, SimulationUIStore& simulationUIStore) :
            m_TelemetryStore(telemetryStore), m_LifecycleStore(lifecycleStore), m_LocalUserStore(localUserStore),
            m_VirtualInputStore(virtualInputStore), m_SimulationUIStore(simulationUIStore) {}
        ~SimulationUIService() = default;

        SimulationUIService(const SimulationUIService&) = delete;
        auto operator=(const SimulationUIService&) -> SimulationUIService& = delete;

        // Draws the content of the simulation window.
        auto Draw() -> void;

    private:
        TelemetryStore& m_TelemetryStore;
        LifecycleStore& m_LifecycleStore;
        LocalUserStore& m_LocalUserStore;
        VirtualInputStore& m_VirtualInputStore;
        SimulationUIStore& m_SimulationUIStore;

        // Draws the requested and the real engine speed, the presets and the custom field.
        auto DrawGameSpeed() -> void;

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