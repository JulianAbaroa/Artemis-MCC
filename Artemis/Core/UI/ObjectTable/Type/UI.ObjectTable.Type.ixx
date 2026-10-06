export module UI.ObjectTable.Type;

import Export.Tick.Type;
import std;

namespace
{
    using AliveObject = Export::Tick::Type::AliveObject;
}

export namespace UI::ObjectTable::Type
{
    // Alive objects of the same class.
    using Group = std::vector<const AliveObject*>;

    // Groups of alive objects by class name.
    using Groups = std::map<std::string, Group>;
}