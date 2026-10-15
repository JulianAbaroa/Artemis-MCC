export module Environment.Details.State;

import Common.State.System;
import Environment.Details.Type;
import std;

export namespace Environment::Details::State
{
    using VitalityDetailsStore = Common::State::System::Snapshot<Environment::Details::Type::VitalityDetails>;
    using ZoneDetailsStore = Common::State::System::Snapshot<Environment::Details::Type::ZoneDetails>;
    using TeleporterDetailsStore = Common::State::System::Snapshot<Environment::Details::Type::TeleporterDetails>;
}