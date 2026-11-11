module;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

module Engine.Capture.Oracle.System;

import Common.Engine.Type;
import Engine.Base.Memory.Type;

namespace
{
    using Blam::Layout::Collision::Type::Result::Decode;
    using Engine::Capture::Oracle::Type::CollisionTestVectorFunction;
    using Engine::Capture::Oracle::Type::OracleBatch;
    using Engine::Capture::Oracle::Type::OracleOutcome;
    using Engine::Capture::Oracle::Type::OracleRay;
    using Engine::Capture::Oracle::Type::OracleStatus;
    using Engine::Capture::Oracle::Type::k_MaxVectorLength;

    using Common::Engine::Type::k_NoHandle;
    using Blam::Layout::Collision::Type::Constant::k_ResultSize;
    using Blam::Layout::Collision::Type::Flag::k_Static;
    using Blam::Layout::Collision::Type::Flag::k_StaticAndObjects;
    using Engine::Base::Memory::Type::Signature::k_CollisionTestVectorPrologue;
    using Engine::Base::Memory::Type::Signature::k_CollisionTestVectorRva;
    using Engine::Base::Memory::Type::Signature::k_CollisionTestVectorWindow;

    // The engine reads a point and a vector as 16 bytes, so each gets a fourth float.
    constexpr std::size_t k_PaddedVectorSize{ 4 };
    constexpr std::int64_t k_NanosecondsPerSecond{ 1'000'000'000 };

    // Bytes the engine could write past k_ResultSize if the layout of the result is not what the static analysis says, and the value
    // they are filled with to notice it.
    constexpr std::size_t k_GuardSize{ 0x90 };
    constexpr std::uint8_t k_Canary{ 0xA5 };

    // Exception filter: only an access violation is handled. The code is kept for the log. Anything else (a breakpoint, a stack overflow,
    // a C++ exception) goes on to its own handler.
    auto FilterFault(unsigned long exceptionCode, unsigned long& faultCode) -> int
    {
        faultCode = exceptionCode;
        return exceptionCode == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
    }

    // Calls the engine function, catching an access violation.
    // return: False if the call faulted (faultCode says which). isHit is only valid when it returns true.
    auto CallGuarded(CollisionTestVectorFunction function, std::uint64_t flags, const float* point,
        const float* vector, std::uint32_t ignoreHandle, void* result, bool& isHit, unsigned long& faultCode) -> bool
    {
        __try
        {
            isHit = function(flags, false, point, vector, ignoreHandle, k_NoHandle, k_NoHandle, result);
            return true;
        }
        __except (FilterFault(GetExceptionCode(), faultCode))
        {
            return false;
        }
    }

    // return: Whether the ray can go to the engine: every value finite and a length between 0 (excluded) and k_MaxVectorLength.
    auto IsRayValid(const OracleRay& ray) -> bool
    {
        float lengthSquared{ 0.0f };
        for (std::size_t axis{ 0 }; axis < ray.Origin.size(); ++axis)
        {
            if (!std::isfinite(ray.Origin[axis]) || !std::isfinite(ray.Vector[axis])) return false;
            lengthSquared += ray.Vector[axis] * ray.Vector[axis];
        }

        return std::isfinite(lengthSquared) && lengthSquared > 0.0f && lengthSquared <= k_MaxVectorLength * k_MaxVectorLength;
    }
}

namespace Engine::Capture::Oracle::System
{
    auto OracleService::Run() -> void
    {
        if (!m_OracleStore.IsEnabled()) return;

        const std::uint64_t sequence{ m_OracleStore.AdvanceSequence() };
        if (!m_LifecycleStore.CanPublish() || m_CaptureStore.GetThreadContext() == 0)
        {
            if (!std::exchange(m_IsSkipLogged, true))
            {
                m_Log.Message("[OracleService] INFO: Run skipped: the engine is not running or the thread context is not known yet.");
            }
            return;
        }

        std::optional<OracleBatch> batch{ m_OracleStore.TakeBatch() };
        if (!batch.has_value()) return;
        if (!this->ResolveFunction()) return;

        OracleOutcome outcome{ .BatchId = batch->Id, .SequenceAtSubmit = batch->SequenceAtSubmit, .SequenceAtRun = sequence };
        outcome.Probes.reserve(batch->Rays.size());

        for (const OracleRay& ray : batch->Rays)
        {
            std::optional<OracleProbe> probe{ this->Probe(ray) };
            if (!probe.has_value())
            {
                m_CollisionTestVector = nullptr;
                m_OracleStore.SetStatus(OracleStatus::Unavailable);
                m_Log.Message("[OracleService] ERROR: collision_test_vector failed. The state of the game is untrusted: restart the game."
                    " The raycast oracle is off until then.");
                return;
            }

            outcome.Probes.push_back(*probe);
        }

        if (!std::exchange(m_IsFirstOutcomeLogged, true))
        {
            m_Log.Message("[OracleService] INFO: First batch traced on the game thread: {} rays.", outcome.Probes.size());
        }

        if (m_OracleStore.IsEnabled()) m_OracleStore.TryTransition(OracleStatus::Waiting, OracleStatus::Running);
        m_OracleStore.Publish(std::move(outcome));
    }

    auto OracleService::ResolveFunction() -> bool
    {
        if (m_CollisionTestVector != nullptr) return true;
        if (m_IsResolveTried)
        {
            // A start after a failed resolve must not show Waiting forever.
            m_OracleStore.SetStatus(OracleStatus::Unavailable);
            return false;
        }

        m_IsResolveTried = true;

        const std::uintptr_t address{ m_AOBService.FindFunctionAtRva(k_CollisionTestVectorPrologue, k_CollisionTestVectorRva,
            k_CollisionTestVectorWindow) };
        LARGE_INTEGER frequency{};
        if (address == 0 || !QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0)
        {
            m_OracleStore.SetStatus(OracleStatus::Unavailable);
            m_Log.Message("[OracleService] ERROR: Failed to find collision_test_vector. The raycast oracle stays off.");
            return false;
        }

        m_TicksPerSecond = frequency.QuadPart;
        m_CollisionTestVector = reinterpret_cast<CollisionTestVectorFunction>(address);
        m_Log.Message("[OracleService] INFO: collision_test_vector found at rva {:X}.", k_CollisionTestVectorRva);
        return true;
    }

    auto OracleService::Probe(const OracleRay& ray) -> std::optional<OracleProbe>
    {
        OracleProbe probe{};

        if (!IsRayValid(ray))
        {
            probe.IsSkipped = true;
            return probe;
        }

        const std::optional<std::uint32_t> staticNanoseconds{ this->Trace(k_Static, ray, k_NoHandle, probe.Static) };
        if (!staticNanoseconds.has_value()) return std::nullopt;

        const std::optional<std::uint32_t> objectsNanoseconds{ this->Trace(k_StaticAndObjects, ray, ray.IgnoreHandle, probe.Objects) };
        if (!objectsNanoseconds.has_value()) return std::nullopt;

        probe.StaticNanoseconds = *staticNanoseconds;
        probe.ObjectsNanoseconds = *objectsNanoseconds;
        return probe;
    }

    auto OracleService::Trace(std::uint64_t flags, const OracleRay& ray, std::uint32_t ignoreHandle, CollisionResult& result)
        -> std::optional<std::uint32_t>
    {
        alignas(16) std::array<float, k_PaddedVectorSize> point{};
        alignas(16) std::array<float, k_PaddedVectorSize> vector{};
        std::ranges::copy(ray.Origin, point.begin());
        std::ranges::copy(ray.Vector, vector.begin());

        alignas(16) std::array<std::uint8_t, k_ResultSize + k_GuardSize> raw{};
        std::fill(raw.begin() + k_ResultSize, raw.end(), k_Canary);

        bool isHit{ false };
        unsigned long faultCode{ 0 };
        LARGE_INTEGER start{};
        LARGE_INTEGER end{};

        QueryPerformanceCounter(&start);
        const bool isCalled{ CallGuarded(m_CollisionTestVector, flags, point.data(), vector.data(), ignoreHandle, raw.data(), isHit,
            faultCode) };
        QueryPerformanceCounter(&end);

        if (!isCalled)
        {
            m_Log.Message("[OracleService] ERROR: collision_test_vector raised exception {:X}.", faultCode);
            return std::nullopt;
        }

        if (!std::ranges::all_of(std::span{ raw }.subspan(k_ResultSize), [](std::uint8_t value) { return value == k_Canary; }))
        {
            m_Log.Message("[OracleService] ERROR: collision_test_vector wrote past the {} bytes of its result.", k_ResultSize);
            return std::nullopt;
        }

        result = Decode(isHit, std::span<const std::uint8_t, k_ResultSize>{ raw.data(), k_ResultSize });

        const std::int64_t nanoseconds{ ((end.QuadPart - start.QuadPart) * k_NanosecondsPerSecond) / m_TicksPerSecond };
        return static_cast<std::uint32_t>(std::clamp<std::int64_t>(nanoseconds, 0, (std::numeric_limits<std::uint32_t>::max)()));
    }
}