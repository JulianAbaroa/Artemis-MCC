export module Resolved.Definitions.Type:Weap;

import :Object;

import std;

export namespace Resolved::Definitions::Type::Weap
{
    // Holds the weapon definition.
    struct Weap
    {
        Object::Object Base{};

        float AutoaimRange{};
    };
}