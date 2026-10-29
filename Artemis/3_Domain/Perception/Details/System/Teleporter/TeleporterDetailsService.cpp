module Perception.Details.System;
import :TeleporterDetails;

import Common.Semantic.Type;
import Blam.Runtime.Object.Type;
import Perception.Classifier.Type;

namespace
{
    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::Subtype::Teleporter;
    using Blam::Runtime::Object::Type::Crate::Crate;
    using Blam::Runtime::Object::Type::Crate::Teleport::Allowed;
    using Blam::Runtime::Object::Type::Crate::Teleport::HasFlag;
    using Perception::Classifier::Type::Classified;
    using Perception::Details::Type::TeleporterPass;
    using Perception::Details::Type::TeleporterDetail;
    using Perception::Details::Type::TeleporterDetails;

    // Teleporter seen in this tick, kept to link the exits of each channel.
    struct Endpoint
    {
        std::uint32_t Handle{};
        std::uint8_t Subtype{};
        std::uint8_t Channel{};
    };

    // return: The passes of the allowed flags. Players pass unless NoPlayers is set.
    auto PassesOf(Allowed allowed) -> TeleporterPass
    {
        TeleporterPass passes{ TeleporterPass::None };

        if (!HasFlag(allowed, Allowed::NoPlayers)) passes = passes | TeleporterPass::Players;
        if (HasFlag(allowed, Allowed::AllowGround)) passes = passes | TeleporterPass::GroundVehicles;
        if (HasFlag(allowed, Allowed::AllowHeavy)) passes = passes | TeleporterPass::HeavyVehicles;
        if (HasFlag(allowed, Allowed::AllowFlying)) passes = passes | TeleporterPass::FlyingVehicles;
        if (HasFlag(allowed, Allowed::AllowProjctile)) passes = passes | TeleporterPass::Projectiles;

        return passes;
    }

    // return: The subtype of the exits of a teleporter: Receiver for a Sender, TwoWay for a TwoWay, None for a Receiver.
    auto ExitOf(std::uint8_t subtype) -> std::uint8_t
    {
        if (subtype == static_cast<std::uint8_t>(Teleporter::Sender)) return static_cast<std::uint8_t>(Teleporter::Receiver);
        if (subtype == static_cast<std::uint8_t>(Teleporter::TwoWay)) return static_cast<std::uint8_t>(Teleporter::TwoWay);

        return static_cast<std::uint8_t>(Teleporter::None);
    }
}

namespace Perception::Details::System
{
    auto TeleporterDetailsService::Update() -> void
    {
        auto classifiedsPtr = m_ClassifierStore.Acquire();
        auto objectTablePtr = m_ObjectStore.Acquire();
        if (!classifiedsPtr || !objectTablePtr) return;

        TeleporterDetails teleporters{};
        std::vector<Endpoint> endpoints{};

        for (const Classified& classified : *classifiedsPtr)
        {
            if (classified.Function != Function::Teleporter) continue;

            const auto objectIt{ objectTablePtr->find(classified.Handle) };
            if (objectIt == objectTablePtr->end()) continue;

            const auto* crate{ std::get_if<Crate>(&objectIt->second.Specific) };
            if (!crate || !crate->Teleport) continue;

            const std::uint8_t channel{ crate->Teleport->Channel };

            teleporters.emplace(classified.Handle, TeleporterDetail{ .Handle = classified.Handle, .Channel = channel,
                .Passes = PassesOf(crate->Teleport->Allowed) });
            endpoints.push_back(Endpoint{ .Handle = classified.Handle, .Subtype = classified.Subtype, .Channel = channel });
        }

        std::ranges::sort(endpoints, {}, &Endpoint::Handle);

        for (const Endpoint& endpoint : endpoints)
        {
            const std::uint8_t exit{ ExitOf(endpoint.Subtype) };
            if (exit == static_cast<std::uint8_t>(Teleporter::None)) continue;

            auto& destinations{ teleporters.at(endpoint.Handle).Destinations };

            for (const Endpoint& other : endpoints)
            {
                if (other.Handle == endpoint.Handle || other.Channel != endpoint.Channel || other.Subtype != exit) continue;

                destinations.push_back(other.Handle);
            }
        }

        m_TeleporterDetailsStore.Publish(std::move(teleporters));
    }

    auto TeleporterDetailsService::Cleanup() -> void
    {
        m_TeleporterDetailsStore.Cleanup();

        m_Log.Message("[TeleporterDetailsService] INFO: Cleanup completed.");
    }
}