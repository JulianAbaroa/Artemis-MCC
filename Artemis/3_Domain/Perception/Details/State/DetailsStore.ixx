export module Perception.Details.State;

export import :ChargeMemory;

import Common.State.Type;
import Perception.Details.Type;
import std;

export namespace Perception::Details::State
{
    class VitalityDetailsStore : public Common::State::Type::Snapshot<Perception::Details::Type::VitalityDetails>
    {
    };
    class ZoneDetailsStore : public Common::State::Type::Snapshot<Perception::Details::Type::ZoneDetails>
    {
    };
    class TeleporterDetailsStore : public Common::State::Type::Snapshot<Perception::Details::Type::TeleporterDetails>
    {
    };
    class LiftDetailsStore : public Common::State::Type::Snapshot<Perception::Details::Type::LiftDetails>
    {
    };
    class BarrierDetailsStore : public Common::State::Type::Snapshot<Perception::Details::Type::BarrierDetails>
    {
    };
    class ProjectileDetailsStore : public Common::State::Type::Snapshot<Perception::Details::Type::ProjectileDetails>
    {
    };
    class WeaponDetailsStore : public Common::State::Type::Snapshot<Perception::Details::Type::WeaponDetails>
    {
    };
    class ObjectiveDetailsStore : public Common::State::Type::Snapshot<Perception::Details::Type::ObjectiveDetails>
    {
    };
}