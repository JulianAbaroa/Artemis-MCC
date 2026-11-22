export module Engine.Control.Input.System:PossessionService;

import Common.Engine.Type;
import Common.Log.System;
import Control.Action.Type;
import Control.Client.Type;
import Control.Client.System;
import Engine.Control.Input.Type;
import Engine.Control.Input.State;
import Engine.Control.Simulation.System;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;

    using ::Control::Action::Type::UnitClientState;
    using ::Control::Client::System::StepClient;
    using ::Control::Client::Type::ClientContextSource;
    using ::Control::Client::Type::ClientSettings;
    using ::Control::Client::Type::ClientState;
    using ::Control::Client::Type::ClientStatus;
    using ::Control::Client::Type::ClientStep;
    using ::Control::Client::Type::InputFrame;
    using ::Control::Client::Type::UnitContext;

    using Engine::Control::Input::State::PossessionStore;
    using Engine::Control::Input::Type::ControlTagLocator;
    using Engine::Control::Input::Type::PossessedBot;
    using Engine::Control::Input::Type::k_InputTableSize;

    using Engine::Control::Simulation::System::BotService;
}

export namespace Engine::Control::Input::System
{
    // Runs the client of the user for the bot it possesses, once per FRAME (D360). The hook of the player control (0x5df80) calls it for input user 0 with the
    // table of that frame, so every frame counts once whatever the ticks do (zero or several per frame): the aim accumulates and the one-frame
    // requests wait in the possessed bot until a tick delivers them (pure AccumulateFrame and TakeTickAction).
    // note: Game thread only. It never writes game memory.
    class PossessionService
    {
    private:
        using Log = Common::Log::System::Log;

    public:
        PossessionService(Log& log, BotService& botService, PossessionStore& possessionStore) :
            m_Log(log), m_BotService(botService), m_PossessionStore(possessionStore) {}
        ~PossessionService() = default;

        PossessionService(const PossessionService&) = delete;
        auto operator=(const PossessionService&) -> PossessionService& = delete;

        // return: Whether a bot is possessed (the hook only calls StepFrame then).
        auto IsActive() const -> bool;

        // Runs one frame of the client for the possessed bot. The bot is shared with the ticks without a lock, so it only runs on the thread that
        // began the possession: on another one it logs one ERROR, turns the possession off and never touches the bot. Logs one INFO per possession with
        // the two thread ids. Does nothing when the unit is missing, the settings cannot be read or the table is not in
        // the keyboard and mouse state: the tick then gives the bot a neutral action.
        // param moduleBase: Base of the game module.
        // param controlTag: Where the control tag is (the settings read it).
        // param threadContext: Thread context of the engine.
        // param inputUser: The input user of the call (the one the possession listens to).
        // param localIndex: Its local index.
        // param deltaTime: Seconds of the frame, the same dt the engine client gets.
        // param table: The event table of the input user at the entry of the call.
        auto StepFrame(std::uintptr_t moduleBase, const ControlTagLocator& controlTag, std::uintptr_t threadContext, std::int32_t inputUser,
            std::uint32_t localIndex, float deltaTime, const std::array<std::uint8_t, k_InputTableSize>& table) -> void;

    private:
        Log& m_Log;
        BotService& m_BotService;
        PossessionStore& m_PossessionStore;
    };
}