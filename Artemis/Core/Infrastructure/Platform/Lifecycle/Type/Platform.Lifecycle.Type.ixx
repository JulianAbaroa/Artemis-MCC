export module Platform.Lifecycle.Type;

import std;

export namespace Platform::Lifecycle::Type
{
    using LifecycleCallback = std::function<void()>;

    // Logic ticks per second of the engine at x1 (fixed step of 1/60 s, D105).
    inline constexpr float k_TicksPerSecond{ 60.0f };

    // Signature of the engine initialization function.
    using EngineInitializeFunction = auto(__fastcall*)() -> void;

    // Signature of the engine destruction function.
    using DestroySubsystemsFunction = auto(__fastcall*)() -> void;

    // State of the game engine as seen by Artemis.
    enum class Status
    {
        // No engine instance yet. Initial state and the state after a reboot.
        Waiting,
        // Engine initialized. Waiting for simulation ticks.
        Initialized,
        // Simulation ticks are being processed.
        Running,
        // Engine is being destroyed. Artemis work must stop.
        TearingDown,
        // Engine destroyed, or hooks found corrupted. Waiting for a reboot.
        Destroyed,
    };
}