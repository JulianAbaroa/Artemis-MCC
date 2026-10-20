export module Environment.Details.State;

import Common.State.Type;
import Environment.Details.Type;
import std;

export namespace Environment::Details::State
{
    class VitalityDetailsStore : public Common::State::Type::Snapshot<Environment::Details::Type::VitalityDetails>
    {
    };
    class ZoneDetailsStore : public Common::State::Type::Snapshot<Environment::Details::Type::ZoneDetails>
    {
    };
    class TeleporterDetailsStore : public Common::State::Type::Snapshot<Environment::Details::Type::TeleporterDetails>
    {
    };
    class LiftDetailsStore : public Common::State::Type::Snapshot<Environment::Details::Type::LiftDetails>
    {
    };
    class BarrierDetailsStore : public Common::State::Type::Snapshot<Environment::Details::Type::BarrierDetails>
    {
    };
}