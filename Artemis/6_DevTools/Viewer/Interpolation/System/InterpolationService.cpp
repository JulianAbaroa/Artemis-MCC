module DevTools.Viewer.Interpolation.System;

import Common.Math.Type;
import Common.Math.System;
import Common.Semantic.Type;
import Perception.Collidable.Type;
import Perception.Entities.Type;
import Bridge.Codec.Tick.Type;
import std;

namespace
{
    using Common::Math::Type::Pose;
    using Common::Math::Type::PoseSource;
    using Common::Math::Type::RenderPose;
    using Common::Semantic::Type::Physics;
    using Perception::Collidable::Type::Collidable;
    using DevTools::Viewer::Interpolation::Type::ObjectDelta;

    using DevTools::Viewer::Interpolation::Type::k_InterpolationSnapDistance;

    // Hash constants (Knuth and murmur finalizer multipliers) that fold the fraction and the tick into the revision.
    constexpr std::uint32_t k_FractionHash{ 0x9E3779B1u };
    constexpr std::uint32_t k_TickHash{ 0x85EBCA6Bu };

    auto PoseOf(const Collidable& object) -> Pose
    {
        return Pose{ .Position = object.Position, .Forward = object.Forward, .Up = object.Up };
    }

    // return: Whether the game never moves the object (Forge Fixed and Phased).
    auto IsStaticIn(const Bridge::Codec::Tick::Type::Tick& tick, std::uint32_t handle) -> bool
    {
        if (!tick.Entities) return false;

        const auto* entity{ tick.Entities->Find(handle) };
        return entity && (entity->Physics == Physics::Fixed || entity->Physics == Physics::Phased);
    }

    auto IsSamePose(const Pose& a, const Pose& b) -> bool
    {
        return std::memcmp(&a, &b, sizeof(Pose)) == 0;
    }
}

namespace DevTools::Viewer::Interpolation::System
{
    auto InterpolationService::Build(const CollidableScene& shown, const std::shared_ptr<const CollidableScene>& newer,
        const std::shared_ptr<const CollidableScene>& older, float fraction, FramePoses& out) -> void
    {
        out.Objects.clear();
        out.Revision = 0;
        out.Source = PoseSource::None;

        const bool isExact{ newer != nullptr };
        const std::shared_ptr<const CollidableScene>& neighbor{ isExact ? newer : older };
        if (!neighbor || !shown.Tick || !std::isfinite(fraction) || fraction < 0.0f || fraction > 1.0f) return;

        out.Source = isExact ? PoseSource::Exact : PoseSource::Extrapolated;

        // note: The two scenes are small (hundreds of objects) and the table is sorted for the lookups, so no hash map is built per frame.
        // note: Collidable is qualified from the root: inside DevTools::Viewer the bare name is the Collidable namespace of the Viewer.
        std::vector<const ::Perception::Collidable::Type::Collidable*> others{};
        others.reserve(neighbor->Objects.size());
        for (const ::Perception::Collidable::Type::Collidable& object : neighbor->Objects) others.push_back(&object);
        std::ranges::sort(others, {}, &::Perception::Collidable::Type::Collidable::Handle);

        for (const ::Perception::Collidable::Type::Collidable& object : shown.Objects)
        {
            if (IsStaticIn(*shown.Tick, object.Handle)) continue;

            const auto it{ std::ranges::lower_bound(others, object.Handle, {}, &::Perception::Collidable::Type::Collidable::Handle) };
            if (it == others.end() || (*it)->Handle != object.Handle) continue;

            const Pose base{ PoseOf(object) };
            const Pose other{ PoseOf(**it) };
            if (IsSamePose(base, other)) continue;

            const RenderPose render{ isExact
                ? Common::Math::System::ResolveRenderPose(base, other, std::nullopt, fraction, k_InterpolationSnapDistance)
                : Common::Math::System::ResolveRenderPose(base, std::nullopt, other, fraction, k_InterpolationSnapDistance) };
            if (render.Source == PoseSource::None) continue;

            out.Objects.push_back(ObjectDelta{ .Handle = object.Handle,
                .Offset = Common::Math::System::Subtract(render.Value.Position, base.Position),
                .Transform = Common::Math::System::MovePose3x4(base, render.Value) });
        }

        std::ranges::sort(out.Objects, {}, &ObjectDelta::Handle);

        const std::uint32_t tick{ shown.Tick->Match ? shown.Tick->Match->GameTick : 0u };
        out.Revision = (std::bit_cast<std::uint32_t>(fraction) * k_FractionHash ^ tick * k_TickHash ^ static_cast<std::uint32_t>(out.Source)) | 1u;
    }
}