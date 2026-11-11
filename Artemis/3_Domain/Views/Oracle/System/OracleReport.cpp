module Views.Oracle.System;

import Blam.Layout.Collision.Type;
import Views.Raycast.Type;

namespace
{
    using Blam::Layout::Collision::Type::Result::CollisionResult;
    using Blam::Layout::Collision::Type::Result::ResultKind;
    using Views::Oracle::Type::Sample::RayDifference;
    using Views::Oracle::Type::Sample::RaySource;
    using Views::Oracle::Type::Stats::OracleStats;
    using Views::Oracle::Type::Stats::OracleSummary;
    using Views::Oracle::Type::Stats::SideCounters;
    using Views::Oracle::Type::Stats::WorstDifference;
    using Views::Raycast::Type::HitKind;

    auto KindName(ResultKind kind) -> std::string_view
    {
        switch (kind)
        {
        case ResultKind::None: return "none";
        case ResultKind::Structure: return "structure";
        case ResultKind::Special: return "special";
        case ResultKind::InstancedGeometry: return "instance";
        case ResultKind::Object: return "object";
        }

        return "unknown";
    }

    auto HitKindName(HitKind kind) -> std::string_view
    {
        switch (kind)
        {
        case HitKind::None: return "none";
        case HitKind::Static: return "static";
        case HitKind::Dynamic: return "dynamic";
        }

        return "unknown";
    }

    auto FormatSide(std::string_view name, const SideCounters& side) -> std::string
    {
        return std::format("{}: rays={} engine_hits={} (structure={} instance={} object={}) ours_hits={} hit_diff={} dist_diff={} (max {:.3f} m, mean {:.4f} m over {})"
            " kind_diff={} object_diff={} material_diff={} flags_diff={}", name, side.Rays, side.EngineHits, side.EngineStructureHits,
            side.EngineInstanceHits, side.EngineObjectHits, side.OursHits, side.HitMismatches, side.DistanceMismatches, side.MaxDistanceDelta,
            side.BothHit == 0 ? 0.0 : side.SumDistanceDelta / static_cast<double>(side.BothHit), side.BothHit, side.KindMismatches,
            side.ObjectMismatches, side.MaterialMismatches, side.FlagsMismatches);
    }

    auto FormatDifferences(const RayDifference& difference) -> std::string
    {
        std::string text{};

        if (difference.IsHitDifferent) text += " hit";
        if (difference.IsDistanceDifferent) text += std::format(" distance({:.3f} m)", difference.DistanceDelta);
        if (difference.IsKindDifferent) text += " kind";
        if (difference.IsObjectDifferent) text += " object";
        if (difference.IsMaterialDifferent) text += " material";
        if (difference.IsFlagsDifferent) text += " flags";

        return text;
    }

    auto FormatWorst(std::size_t rank, const WorstDifference& worst) -> std::string
    {
        const auto& sample{ worst.Sample };
        const CollisionResult& engine{ worst.IsObjectsSide ? sample.EngineObjects : sample.EngineStatic };

        std::string text{ std::format("#{} side={} score={:.1f} source={}\n", rank, worst.IsObjectsSide ? "objects" : "static", worst.Score,
            sample.Source == RaySource::Tick ? "tick" : "random") };

        text += std::format("  ray: origin=({:.3f}, {:.3f}, {:.3f}) direction=({:.4f}, {:.4f}, {:.4f}) max_distance={:.3f}\n", sample.Origin.X,
            sample.Origin.Y, sample.Origin.Z, sample.Direction.X, sample.Direction.Y, sample.Direction.Z, sample.MaxDistance);

        if (worst.IsObjectsSide && sample.OursTick.has_value())
        {
            const auto& ours{ *sample.OursTick };
            text += std::format("  ours (tick): hit={} kind={} distance={:.3f} object={:08X} material={} flags={:02X}\n", ours.Hit,
                HitKindName(ours.Kind), ours.Distance, ours.ObjectHandle, ours.Material, ours.SurfaceFlags);
        }
        else
        {
            text += std::format("  ours (static): hit={} distance={:.3f} material={} flags={:02X}\n", sample.OursStatic.IsHit,
                sample.OursStatic.Distance, sample.OursStatic.Material, sample.OursStatic.SurfaceFlags);
        }

        text += std::format("  engine: hit={} kind={} fraction={:.5f} distance={:.3f} point=({:.3f}, {:.3f}, {:.3f}) material={} flags={:02X}"
            " byte_a={:02X} byte_b={:02X} back_face={} object={:08X}\n", engine.IsHit, KindName(engine.Kind), engine.Fraction,
            engine.Fraction * sample.MaxDistance, engine.Point[0], engine.Point[1], engine.Point[2], engine.Material, engine.SurfaceFlags,
            engine.SurfaceByteA, engine.SurfaceByteB, engine.IsBackFace, engine.ObjectHandle);
        text += std::format("  differences:{}\n", FormatDifferences(worst.Difference));

        return text;
    }
}

namespace Views::Oracle::System
{
    auto FormatSummary(const OracleStats& stats) -> std::string
    {
        const OracleSummary summary{ Summarize(stats) };

        std::string text{ std::format("rays={} (tick={} random={}) | {} | {} | random_object_hits={} | engine us static mean={:.2f} p50={:.2f}"
            " p99={:.2f} objects mean={:.2f} p50={:.2f} p99={:.2f} | lag ticks mean={:.2f} max={} | outcomes={} stale={} skipped_rays={}",
            stats.TickRays + stats.RandomRays, stats.TickRays, stats.RandomRays, FormatSide("static", stats.Static),
            FormatSide("objects", stats.Objects), stats.RandomObjectHits, summary.StaticMean, summary.StaticP50, summary.StaticP99,
            summary.ObjectsMean, summary.ObjectsP50, summary.ObjectsP99, summary.LagMean, stats.LagMax, stats.Outcomes, stats.StaleOutcomes, stats.SkippedRays) };

        return text;
    }

    auto FormatReport(const OracleStats& stats, std::string_view header) -> std::string
    {
        const OracleSummary summary{ Summarize(stats) };

        std::string text{ header };
        text += "\n\nCounters\n";
        text += std::format("  rays: {} (tick {}, random {}), outcomes {}, stale outcomes {}, random rays where the engine hit an object {}, rays skipped (not valid) {}\n",
            stats.TickRays + stats.RandomRays, stats.TickRays, stats.RandomRays, stats.Outcomes, stats.StaleOutcomes, stats.RandomObjectHits,
            stats.SkippedRays);
        text += std::format("  {}\n", FormatSide("static (engine without objects against Artemis, same ray)", stats.Static));
        text += std::format("  {}\n", FormatSide("objects (engine with objects against the tick, one or two ticks apart)", stats.Objects));
        text += std::format("  engine cost in microseconds per ray: static mean {:.2f} p50 {:.2f} p99 {:.2f} max {:.2f}, objects mean {:.2f} p50 {:.2f}"
            " p99 {:.2f} max {:.2f}\n", summary.StaticMean, summary.StaticP50, summary.StaticP99, stats.StaticCost.MaxMicroseconds,
            summary.ObjectsMean, summary.ObjectsP50, summary.ObjectsP99, stats.ObjectsCost.MaxMicroseconds);
        text += std::format("  lag between handing the rays over and the engine tracing them, in ticks: mean {:.2f} max {}\n", summary.LagMean,
            stats.LagMax);

        text += std::format("\nWorst {} differences\n", stats.Worst.size());
        for (std::size_t index{ 0 }; index < stats.Worst.size(); ++index)
        {
            text += FormatWorst(index + 1, stats.Worst[index]);
        }

        return text;
    }
}