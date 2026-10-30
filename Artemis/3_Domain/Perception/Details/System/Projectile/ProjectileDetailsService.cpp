module Perception.Details.System;
import :ProjectileDetails;

import Common.Engine.Type;
import Blam.Runtime.Object.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Blam::Runtime::Object::Type::Projectile::Projectile;
    using Perception::Details::Type::ProjectileDetail;
    using Perception::Details::Type::ProjectileDetails;
}

namespace Perception::Details::System
{
    auto ProjectileDetailsService::Update() -> void
    {
        auto objectTablePtr = m_ObjectStore.Acquire();
        if (!objectTablePtr) return;

        ProjectileDetails projectiles{};

        for (const auto& [handle, object] : *objectTablePtr)
        {
            const Projectile* projectile{ std::get_if<Projectile>(&object.Specific) };
            if (projectile == nullptr) continue;

            // note: A parent equal to the owner biped is the thrower's hand, not a victim (measured on armas_a).
            const bool hasParent{ object.ParentHandle != k_NoHandle };
            const bool isInHand{ hasParent && object.ParentHandle == projectile->OwnerBiped };
            const bool isStuck{ hasParent && !isInHand };

            projectiles.emplace(handle, ProjectileDetail{
                .Handle = handle,
                .OwnerPlayer = projectile->OwnerPlayer,
                .OwnerBiped = projectile->OwnerBiped,
                .ShooterRoot = projectile->ShooterRoot,
                .StuckTo = isStuck ? object.ParentHandle : k_NoHandle,
                .StuckNode = isStuck ? projectile->ParentNode : static_cast<std::int8_t>(-1),
                .InHand = isInHand });
        }

        m_ProjectileDetailsStore.Publish(std::move(projectiles));
    }

    auto ProjectileDetailsService::Cleanup() -> void
    {
        m_ProjectileDetailsStore.Cleanup();

        m_Log.Message("[ProjectileDetailsService] INFO: Cleanup completed.");
    }
}