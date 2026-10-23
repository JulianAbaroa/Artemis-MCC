module Environment.Details.System;
import :LiftDetails;

import Common.Math.Type;
import Common.Semantic.Type;
import Relations.Classifier.Type;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::Subtype::Lift;
    using Relations::Classifier::Type::Classified;
    using Environment::Details::Type::LiftDetail;
    using Environment::Details::Type::LiftDetails;

    // return: The launch direction of a lift of the subtype, or zero when the subtype is None.
    // note: Provisional rule until A12 (impulse of each lift).
    auto LaunchDirectionOf(std::uint8_t subtype, const Vec3& forward, const Vec3& up) -> Vec3
    {
        if (subtype == static_cast<std::uint8_t>(Lift::Vertical)) return Vec3{ 0.0f, 0.0f, 1.0f };
        if (subtype == static_cast<std::uint8_t>(Lift::Curved)) return forward;

        if (subtype == static_cast<std::uint8_t>(Lift::Redirected))
        {
            return Vec3{ forward.Y * up.Z - forward.Z * up.Y, forward.Z * up.X - forward.X * up.Z, forward.X * up.Y - forward.Y * up.X };
        }

        return Vec3{};
    }
}

namespace Environment::Details::System
{
    auto LiftDetailsService::Update() -> void
    {
        auto classifiedsPtr = m_ClassifierStore.Acquire();
        auto objectTablePtr = m_ObjectStore.Acquire();
        if (!classifiedsPtr || !objectTablePtr) return;

        LiftDetails lifts{};

        for (const Classified& classified : *classifiedsPtr)
        {
            if (classified.Function != Function::Lift) continue;

            const auto objectIt{ objectTablePtr->find(classified.Handle) };
            if (objectIt == objectTablePtr->end() || objectIt->second.Address == 0) continue;

            const auto& object{ objectIt->second };

            lifts.emplace(classified.Handle, LiftDetail{ .Handle = classified.Handle,
                .LaunchDirection = LaunchDirectionOf(classified.Subtype, object.Forward, object.Up) });
        }

        m_LiftDetailsStore.Publish(std::move(lifts));
    }

    auto LiftDetailsService::Cleanup() -> void
    {
        m_LiftDetailsStore.Cleanup();

        m_Log.Message("[LiftDetailsService] INFO: Cleanup completed.");
    }
}