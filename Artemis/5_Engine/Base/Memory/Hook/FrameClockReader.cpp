module;

#include <windows.h>

module Engine.Base.Memory.Hook;
import :FrameClock;

import Engine.Base.Hook.System;
import Engine.Base.Host.Type;
import Blam.Layout.Object.Type;
import std;

namespace
{
    using Engine::Base::Memory::Type::FrameClock::ClockSource;
    using Engine::Base::Memory::Type::FrameClock::FrameClock;
    using Engine::Base::Memory::Type::FrameClock::InterpolationState;

    using Blam::Layout::Object::Type::Constant::k_ThreadContextTimeGlobalsOffset;
    using Blam::Layout::Object::Type::Constant::k_TimeGlobalsEngineTickOffset;
    using Blam::Layout::Object::Type::Constant::k_TimeGlobalsTickFractionOffset;
    using Engine::Base::Host::Type::k_GameModuleName;
    using Engine::Base::Memory::Type::FrameClock::k_RenderStateInterpolationOffOffset;
    using Engine::Base::Memory::Type::FrameClock::k_ThreadContextRenderStateOffset;
    using Engine::Base::Memory::Type::Signature::k_InterpolationBufferRva;
    using Engine::Base::Memory::Type::Signature::k_TelemetryIdModifier;

    // return: The TLS slot global the match of the signature points to, or nullptr.
    auto ResolveSlot(std::uintptr_t match) -> const std::uint32_t*
    {
        __try
        {
            return Engine::Base::Hook::System::ResolveTlsIndex(match);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return nullptr;
        }
    }

    // return: The thread context of the calling thread, or 0 when the slot is unknown, holds none or cannot be read.
    auto ContextOfThread(const std::uint32_t* slot) -> std::uintptr_t
    {
        if (slot == nullptr) return 0;

        __try
        {
            return Engine::Base::Hook::System::ReadThreadContext(*slot);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }
}

namespace Engine::Base::Memory::Hook
{
    auto FrameClockReader::Read(std::uintptr_t publishedContext) -> FrameClock
    {
        const auto module{ reinterpret_cast<std::uintptr_t>(GetModuleHandleW(k_GameModuleName)) };

        // note: The game module is loaded again for every match, so the slot is resolved again when the base changes.
        if (module != m_Module)
        {
            m_Module = module;
            m_TlsSlot = module != 0 ? ResolveSlot(m_AOBService.FindPattern(k_TelemetryIdModifier)) : nullptr;
        }

        const std::uintptr_t interpolationBuffer{ module != 0 ? module + k_InterpolationBufferRva : 0 };

        const std::uintptr_t own{ ContextOfThread(m_TlsSlot) };
        if (own != 0) return this->ReadContext(own, interpolationBuffer, ClockSource::ThreadContext);

        if (publishedContext != 0) return this->ReadContext(publishedContext, interpolationBuffer, ClockSource::Published);

        return FrameClock{};
    }

    auto FrameClockReader::ReadContext(std::uintptr_t context, std::uintptr_t interpolationBuffer, ClockSource source) -> FrameClock
    {
        std::uintptr_t timeGlobals{ 0 };
        if (!m_Reader.ReadRaw(context + k_ThreadContextTimeGlobalsOffset, &timeGlobals, sizeof(timeGlobals)) || timeGlobals == 0) return FrameClock{};

        FrameClock clock{ .Source = source };
        if (!m_Reader.ReadRaw(timeGlobals + k_TimeGlobalsEngineTickOffset, &clock.Tick, sizeof(clock.Tick)) ||
            !m_Reader.ReadRaw(timeGlobals + k_TimeGlobalsTickFractionOffset, &clock.Fraction, sizeof(clock.Fraction))) return FrameClock{};

        clock.Interpolation = this->ReadInterpolation(context, interpolationBuffer);

        return clock;
    }

    auto FrameClockReader::ReadInterpolation(std::uintptr_t context, std::uintptr_t interpolationBuffer) -> InterpolationState
    {
        if (interpolationBuffer == 0) return InterpolationState::Unknown;

        std::uintptr_t buffer{ 0 };
        std::uintptr_t renderState{ 0 };
        std::uint8_t isOff{ 0 };

        if (!m_Reader.ReadRaw(interpolationBuffer, &buffer, sizeof(buffer))) return InterpolationState::Unknown;
        if (!m_Reader.ReadRaw(context + k_ThreadContextRenderStateOffset, &renderState, sizeof(renderState)) || renderState == 0) return InterpolationState::Unknown;
        if (!m_Reader.ReadRaw(renderState + k_RenderStateInterpolationOffOffset, &isOff, sizeof(isOff))) return InterpolationState::Unknown;

        return buffer != 0 && isOff == 0 ? InterpolationState::On : InterpolationState::Off;
    }
}