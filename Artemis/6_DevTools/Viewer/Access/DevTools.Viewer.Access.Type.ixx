export module DevTools.Viewer.Access.Type;

import Engine.Base.Lifecycle.State;
import Engine.Base.Lifecycle.System;
import Engine.Match.State;
import Engine.Match.System;
import Engine.Base.Host.State;
import Engine.Control.Input.State;
import Engine.Render.Window.System;
import Engine.Control.Simulation.State;
import Engine.Base.Memory.System;
import Engine.Capture.State;
import Bridge.Transport.System;
import Engine.Render.State;
import Engine.Render.System;
import std;

namespace
{
    using Engine::Base::Lifecycle::State::LifecycleStore;
    using Engine::Base::Lifecycle::System::LifecycleService;
    using Engine::Match::State::MatchConfigStore;
    using Engine::Match::State::MatchCycleStore;
    using Engine::Match::System::MatchConfigService;
    using Engine::Base::Host::State::LocalUserStore;
    using Engine::Control::Input::State::VirtualInputStore;
    using Engine::Control::Input::State::PlayerActionStore;
    using Engine::Render::Window::System::InputService;
    using Engine::Control::Simulation::State::BotStore;
    using Engine::Base::Memory::System::MemoryReaderService;
    using Engine::Capture::State::CaptureStore;
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
        MemoryReaderService& m_MemoryReaderService;
        LifecycleService& m_LifecycleService;
        LifecycleStore& m_LifecycleStore;
        MatchConfigStore& m_MatchConfigStore;
        MatchConfigService& m_MatchConfigService;
        MatchCycleStore& m_MatchCycleStore;
        LocalUserStore& m_LocalUserStore;
        VirtualInputStore& m_VirtualInputStore;
        PlayerActionStore& m_PlayerActionStore;
        BotStore& m_BotStore;
        BridgeService& m_BridgeService;
    };
}