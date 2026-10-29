export module DevTools.Workbench.ObjectTable.Type;

import Blam.Runtime.Object.Type;
import Bridge.Codec.Tick.Type;
import std;

namespace
{
    using Blam::Runtime::Object::Type::Alive::AliveObject;
}

export namespace DevTools::Workbench::ObjectTable::Type
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