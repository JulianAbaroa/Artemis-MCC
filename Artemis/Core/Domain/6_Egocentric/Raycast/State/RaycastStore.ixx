export module Egocentric.Raycast.State;

import Common.State.System;
import Egocentric.Raycast.Type;

export namespace Egocentric::Raycast::State
{
	using RaycastStore = Common::State::System::Snapshot<Egocentric::Raycast::Type::Raycasts>;
}