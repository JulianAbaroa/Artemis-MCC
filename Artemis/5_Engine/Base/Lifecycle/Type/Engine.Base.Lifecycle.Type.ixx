export module Engine.Base.Lifecycle.Type;

import std;

export namespace Engine::Base::Lifecycle::Type
{
    using LifecycleCallback = std::function<void()>;

    // Callback that receives the full path of a cache file the engine opened.
    using MapFileCallback = std::function<void(const std::string& filePath)>;

    // Signature of the engine initialization function.
    using EngineInitializeFunction = auto(__fastcall*)() -> void;

    // Signature of the engine function that opens a map file.
    // param mapRelativePath: Null terminated path of the map, relative to the game folder.
    using OpenMapFunction = auto(__fastcall*)(std::uint64_t param1,
        std::uint64_t param2, std::uint64_t mapRelativePath, std::uint32_t* param4) -> void;

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