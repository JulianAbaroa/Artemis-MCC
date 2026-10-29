export module Perception.Details.State;

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
    class ObjectiveDetailsStore : public Common::State::Type::Snapshot<Perception::Details::Type::ObjectiveDetails>
    {
    };
}