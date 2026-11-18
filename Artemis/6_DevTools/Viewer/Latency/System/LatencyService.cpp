module DevTools.Viewer.Latency.System;

import Common.Math.Type;
import Common.Math.System;
import Common.State.Type;
import Engine.Base.Memory.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Math::Type::PoseSource;
    using Common::State::Type::PickKind;
    using Engine::Base::Memory::Type::FrameClock::ClockSource;
    using Engine::Base::Memory::Type::FrameClock::FrameClock;
    using DevTools::Viewer::Latency::Type::EyeAtTick;
    using DevTools::Viewer::Latency::Type::LatencyMemory;
    using DevTools::Viewer::Latency::Type::LatencyProbe;
    using DevTools::Viewer::Latency::Type::Percentiles;

    using DevTools::Viewer::Latency::Type::k_CameraMovedDistance;
    using DevTools::Viewer::Latency::Type::k_EyeMovingDistance;
    using DevTools::Viewer::Latency::Type::k_LagBins;

    constexpr double k_P50Fraction{ 0.5 };
    constexpr double k_P99Fraction{ 0.99 };

    // return: The point a fraction of the way from a to b.
    auto Lerp(const Vec3& a, const Vec3& b, float fraction) -> Vec3
    {
        return Vec3{ a.X + (b.X - a.X) * fraction, a.Y + (b.Y - a.Y) * fraction, a.Z + (b.Z - a.Z) * fraction };
    }

    // return: The value at the nearest rank of the fraction, 0 without values. Sorts the values.
    auto PercentileOf(std::vector<float>& values, double fraction) -> float
    {
        if (values.empty()) return 0.0f;

        std::sort(values.begin(), values.end());

        const auto rank{ static_cast<std::size_t>(std::ceil(fraction * static_cast<double>(values.size()))) };
        return values[std::clamp<std::size_t>(rank, 1, values.size()) - 1];
    }

    auto PercentilesOf(std::vector<float>& values) -> Percentiles
    {
        return Percentiles{ .P50 = PercentileOf(values, k_P50Fraction), .P99 = PercentileOf(values, k_P99Fraction) };
    }

    // Keeps the eye of the scene of this tick and, when the tick before it was the last one seen, the eye of that one.
    auto TrackEye(LatencyMemory& memory, std::uint32_t sceneTick, const Vec3& eye) -> void
    {
        if (memory.Current.IsValid && memory.Current.Tick == sceneTick) return;

        memory.Previous = (memory.Current.IsValid && memory.Current.Tick + 1 == sceneTick) ? memory.Current : EyeAtTick{};
        memory.Current = EyeAtTick{ .IsValid = true, .Tick = sceneTick, .Eye = eye };
    }
}

namespace DevTools::Viewer::Latency::System
{
    auto LatencyService::ReadClock() -> FrameClock
    {
        return m_ClockReader.Read(m_CaptureStore.GetThreadContext());
    }

    auto LatencyService::Record(const LatencyInput& input) -> void
    {
        LatencyMemory memory{ m_Store.GetMemory() };
        const LatencySample sample{ this->Observe(memory, input) };

        m_Store.Push(sample, memory);
    }

    auto LatencyService::GetReport() const -> LatencyReport
    {
        const std::vector<LatencySample> samples{ m_Store.GetSamples() };

        return this->Summarize(samples);
    }

    auto LatencyService::Observe(LatencyMemory& memory, const LatencyInput& input) -> LatencySample
    {
        const bool hasClock{ input.Clock.Source != ClockSource::None };
        const std::uint32_t tick{ input.Clock.Tick };

        LatencySample sample{ .Clock = input.Clock, .SceneTick = input.SceneTick, .IsBeforeHud = input.IsBeforeHud,
            .Pick = input.Pick, .WaitedUs = input.WaitedUs, .IsWaited = input.IsWaited, .IsTimedOut = input.IsTimedOut,
            .Interp = input.Interp };

        if (hasClock && input.SceneTick)
        {
            sample.HasLag = true;
            sample.Lag = static_cast<std::int32_t>(static_cast<std::int64_t>(tick) - static_cast<std::int64_t>(*input.SceneTick));
        }

        sample.IsSameTick = hasClock && memory.HasFrame && memory.Tick == tick;

        if (input.Camera && memory.Camera)
        {
            sample.HasCameraStep = true;
            sample.IsCameraMoved = Common::Math::System::Distance(*input.Camera, *memory.Camera) > k_CameraMovedDistance;
        }

        memory.HasFrame = hasClock;
        memory.Tick = tick;
        memory.Camera = input.Camera;

        if (input.SceneTick && input.Eye) TrackEye(memory, *input.SceneTick, *input.Eye);

        const bool isProbeReady{ input.IsGameMode && input.Camera && hasClock && input.SceneTick &&
            memory.Current.IsValid && memory.Previous.IsValid && memory.Current.Tick == *input.SceneTick &&
            Common::Math::System::Distance(memory.Current.Eye, memory.Previous.Eye) > k_EyeMovingDistance };
        if (!isProbeReady) return sample;

        if (*input.SceneTick == tick)
        {
            const Vec3 interpolated{ Lerp(memory.Previous.Eye, memory.Current.Eye, std::clamp(input.Clock.Fraction, 0.0f, 1.0f)) };

            sample.Probe = LatencyProbe{
                .ToCurrent = Common::Math::System::Distance(*input.Camera, memory.Current.Eye),
                .ToPrevious = Common::Math::System::Distance(*input.Camera, memory.Previous.Eye),
                .ToLerp = Common::Math::System::Distance(*input.Camera, interpolated),
            };
        }
        else if (*input.SceneTick < tick)
        {
            sample.IsProbeBehind = true;
        }

        return sample;
    }

    auto LatencyService::Summarize(std::span<const LatencySample> samples) -> LatencyReport
    {
        LatencyReport report{};
        report.Frames = samples.size();

        std::vector<float> toCurrent{};
        std::vector<float> toPrevious{};
        std::vector<float> toLerp{};
        std::vector<float> waits{};

        bool hasMaxLag{ false };

        for (const LatencySample& sample : samples)
        {
            if (sample.HasLag)
            {
                if (sample.Lag < 0) ++report.Negative;
                else ++report.Lag[(std::min)(static_cast<std::size_t>(sample.Lag), k_LagBins - 1)];

                report.MaxLag = hasMaxLag ? (std::max)(report.MaxLag, sample.Lag) : sample.Lag;
                hasMaxLag = true;
            }

            if (sample.IsSameTick && sample.HasCameraStep)
            {
                ++report.SameTickFrames;
                if (sample.IsCameraMoved) ++report.CameraMovedFrames;
            }

            if (sample.IsProbeBehind) ++report.BehindFrames;

            if (sample.Pick == PickKind::Exact) ++report.DrawnExact;
            else if (sample.Pick == PickKind::Older) ++report.DrawnOlder;
            else if (sample.Pick == PickKind::Newer) ++report.DrawnNewer;

            if (sample.Pick != PickKind::None)
            {
                if (sample.Interp == PoseSource::Exact) ++report.InterpExact;
                else if (sample.Interp == PoseSource::Extrapolated) ++report.InterpExtrapolated;
                else ++report.InterpNone;
            }

            if (sample.IsWaited)
            {
                ++report.Waits;
                waits.push_back(static_cast<float>(sample.WaitedUs));
                if (sample.IsTimedOut) ++report.Timeouts;
            }

            if (sample.Probe)
            {
                ++report.ProbeFrames;
                toCurrent.push_back(sample.Probe->ToCurrent);
                toPrevious.push_back(sample.Probe->ToPrevious);
                toLerp.push_back(sample.Probe->ToLerp);
            }
        }

        report.ToCurrent = PercentilesOf(toCurrent);
        report.ToPrevious = PercentilesOf(toPrevious);
        report.ToLerp = PercentilesOf(toLerp);
        report.WaitP99Us = static_cast<std::uint32_t>(PercentileOf(waits, k_P99Fraction));

        if (!samples.empty())
        {
            report.HasLast = true;
            report.Last = samples.back();
        }

        return report;
    }
}