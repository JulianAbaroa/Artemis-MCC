export module Environment.Aim.State;

import Common.State.System;
import Environment.Aim.Type;
import std;

export namespace Environment::Aim::State
{
	using Aims = std::unordered_map<std::uint32_t, Environment::Aim::Type::Aim>;
	using AimStore = Common::State::System::Snapshot<Aims>;
}