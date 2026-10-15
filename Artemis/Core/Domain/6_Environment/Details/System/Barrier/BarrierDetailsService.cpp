module Environment.Details.System;
import :BarrierDetails;

import Common.Math.Type;
import Common.Semantic.Type;
import Relations.Classifier.Type;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::Subtype::Barrier;
    using Common::Semantic::Type::Catalog::TagLabel;
    using Common::Semantic::Type::Catalog::FindTagLabel;
    using Relations::Classifier::Type::Classified;
    using Environment::Details::Type::BarrierDetail;
    using Environment::Details::Type::BarrierDetails;
}

namespace Environment::Details::System
{
    auto BarrierDetailsService::Update() -> void
    {
        auto classifiedsPtr = m_ClassifierStore.Acquire();
        auto objectTablePtr = m_ObjectStore.Acquire();
        if (!classifiedsPtr || !objectTablePtr) return;

        BarrierDetails barriers{};

        for (const Classified& classified : *classifiedsPtr)
        {
            if (classified.Function != Function::Barrier) continue;

            const auto objectIt{ objectTablePtr->find(classified.Handle) };
            if (objectIt == objectTablePtr->end() || objectIt->second.Address == 0) continue;

            const auto& object{ objectIt->second };

            Vec3 passDirection{};

            if (classified.Subtype == static_cast<std::uint8_t>(Barrier::OneWay))
            {
                // note: Some models face the blocked side (D91). The catalog marks them, so the rule needs no tag names.
                const TagLabel* label{ FindTagLabel(object.FourCC, object.TagName) };
                const bool isReversed{ label != nullptr && label->IsFacingReversed };

                passDirection = isReversed ? Vec3{ -object.Forward.X, -object.Forward.Y, -object.Forward.Z } : object.Forward;
            }

            barriers.emplace(classified.Handle, BarrierDetail{ .Handle = classified.Handle, .PassDirection = passDirection });
        }

        m_BarrierDetailsStore.Publish(std::move(barriers));
    }

    auto BarrierDetailsService::Cleanup() -> void
    {
        m_BarrierDetailsStore.Cleanup();

        m_LogsService.Message("[BarrierDetailsService] INFO: Cleanup completed.");
    }
}