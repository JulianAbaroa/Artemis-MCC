export module DevTools.Viewer.Access.Type;

import Engine.Base.Lifecycle.State;
import Engine.Base.Lifecycle.System;
import Engine.Base.MapVariant.System;
import Engine.Base.MatchSetup.System;
import Engine.Base.Preset.System;
import Engine.Match.State;
import Engine.Match.System;
import Engine.Base.Host.State;
import Engine.Control.Input.State;
import Engine.Render.Window.System;
import Engine.Control.Simulation.State;
import Engine.Base.Memory.System;
import Engine.Base.Memory.Hook;
import Engine.Base.Pov.State;
import Engine.Capture.State;
import Engine.Capture.Oracle.State;
import Bridge.Transport.System;
import Engine.Render.State;
import Engine.Render.System;
import std;

namespace
{
    using Engine::Base::Lifecycle::State::LifecycleStore;
    using Engine::Base::Lifecycle::System::LifecycleService;
    using Engine::Base::MatchSetup::System::MatchSetupService;
    using Engine::Base::Preset::System::PresetService;
    using Engine::Match::State::MapVariantDecodeStore;
    using Engine::Match::State::MatchConfigStore;
    using Engine::Match::State::MatchCycleStore;
    using Engine::Match::System::MapVariantCatalogService;
    using Engine::Match::System::MatchConfigService;
    using Engine::Base::MapVariant::System::MapVariantService;
    using Engine::Base::Host::State::LocalUserStore;
    using Engine::Control::Input::State::VirtualInputStore;
    using Engine::Control::Input::State::PlayerActionStore;
    using Engine::Control::Input::State::PossessRequestStore;
    using Engine::Control::Input::State::PossessionStore;
    using Engine::Control::Input::State::AgentClientStore;
    using Engine::Render::Window::System::InputService;
    using Engine::Control::Simulation::State::BotStore;
    using Engine::Base::Memory::System::GameCameraService;
    using Engine::Base::Memory::Hook::FrameClockReader;
    using Engine::Base::Memory::System::MemoryReaderService;
    using Engine::Base::Pov::State::PovRequestStore;
    using Engine::Base::Pov::State::PovStatusStore;
    using Engine::Capture::State::CaptureStore;
    using Engine::Capture::Oracle::State::OracleStore;
    using Bridge::Transport::System::BridgeService;
    using Engine::Render::State::RenderStore;
    using Engine::Render::System::RenderService;
}

export namespace DevTools::Viewer::Access::Type
{
    // What the development tools use of the engine: one reference per member of the engine Layers they read or call (D226).
    // note: Mod builds it from the Layers of Engine and of Bridge.Transport, so DevTools imports none of them.
    struct EngineAccess
    {
        RenderService& m_RenderService;
        RenderStore& m_RenderStore;
        InputService& m_InputService;
        CaptureStore& m_CaptureStore;
        OracleStore& m_OracleStore;
        MemoryReaderService& m_MemoryReaderService;
        GameCameraService& m_GameCameraService;
        FrameClockReader& m_FrameClockReader;
        LifecycleService& m_LifecycleService;
        LifecycleStore& m_LifecycleStore;
        MatchConfigStore& m_MatchConfigStore;
        MatchConfigService& m_MatchConfigService;
        MapVariantDecodeStore& m_MapVariantDecodeStore;
        MapVariantCatalogService& m_MapVariantCatalogService;
        MatchCycleStore& m_MatchCycleStore;
        MatchSetupService& m_MatchSetupService;
        PresetService& m_PresetService;
        MapVariantService& m_MapVariantService;
        LocalUserStore& m_LocalUserStore;
        VirtualInputStore& m_VirtualInputStore;
        PlayerActionStore& m_PlayerActionStore;
        BotStore& m_BotStore;
        BridgeService& m_BridgeService;
        PovRequestStore& m_PovRequestStore;
        PovStatusStore& m_PovStatusStore;

        // The bot the Viewer asks to possess and what the client keeps for it (VW.9c.2 B). The Viewer writes the request, the game thread owns the state.
        PossessRequestStore& m_PossessRequestStore;
        PossessionStore& m_PossessionStore;

        // The counters of the agents that play an AgentInput through the client of Artemis (VW.9c.3 C).
        AgentClientStore& m_AgentClientStore;
    };
}