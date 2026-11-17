export module Views.View.State;

import Common.State.Type;
import Views.View.Type;

export namespace Views::View::State
{
    class ViewsStore : public Common::State::Type::Snapshot<Views::View::Type::ViewTable>
    {
    };
}