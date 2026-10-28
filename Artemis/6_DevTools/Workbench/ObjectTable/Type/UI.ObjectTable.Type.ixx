export module UI.ObjectTable.Type;

import Tables.Object.Type;
import Export.Tick.Type;
import std;

namespace
{
    using Tables::Object::Type::Alive::AliveObject;
}

export namespace UI::ObjectTable::Type
{
    // Alive objects of the same class.
    struct Group : std::vector<const AliveObject*>
    {
        using vector::vector;
    };

    // Groups of alive objects by class name.
    struct Groups : std::map<std::string, Group>
    {
        using map::map;
    };
}