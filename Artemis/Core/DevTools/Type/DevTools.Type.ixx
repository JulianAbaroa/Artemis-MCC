export module DevTools.Type;

import Platform.Lifecycle.State;
import Platform.Lifecycle.System;
import Platform.Match.State;
import Platform.Match.System;
import Platform.Host.State;
import Platform.Input.State;
import Platform.Window.System;
import Platform.Simulation.State;
import Platform.Memory.System;
import Platform.Capture.State;
import Platform.Bridge.System;
import Platform.Render.State;
import Platform.Render.System;
import std;

namespace
{
    using Platform::Lifecycle::State::LifecycleStore;
    using Platform::Lifecycle::System::LifecycleService;
    using Platform::Match::State::MatchConfigStore;
    using Platform::Match::State::MatchCycleStore;
    using Platform::Match::System::MatchConfigService;
    using Platform::Host::State::LocalUserStore;
    using Platform::Input::State::VirtualInputStore;
    using Platform::Input::State::PlayerActionStore;
    using Platform::Window::System::InputService;
    using Platform::Simulation::State::BotStore;
    using Platform::Memory::System::MemoryReaderService;
    using Platform::Capture::State::CaptureStore;
    using Platform::Bridge::System::BridgeService;
    using Platform::Render::State::RenderStore;
    using Platform::Render::System::RenderService;
}

export namespace DevTools::Type
{
    // What the development tools use of the engine: one reference per member of the Platform layer they read or call (D226).
    // note: The composition root (Core.ixx, future Mod) builds it from Platform::Layer, so DevTools does not import Platform.Layer.
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