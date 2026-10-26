export module Egocentric.Raycast.State;

import Common.State.Type;
import Egocentric.Raycast.Type;

export namespace Egocentric::Raycast::State
{
    class RaycastStore : public Common::State::Type::Snapshot<Egocentric::Raycast::Type::Raycasts>
    {
    };
}