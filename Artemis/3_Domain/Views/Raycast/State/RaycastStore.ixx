export module Views.Raycast.State;

import Common.State.Type;
import Views.Raycast.Type;

export namespace Views::Raycast::State
{
    class RaycastStore : public Common::State::Type::Snapshot<Views::Raycast::Type::Raycasts>
    {
    };
}