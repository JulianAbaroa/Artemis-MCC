export module UI.DefinitionsInspector.Type;

import std;

export namespace UI::DefinitionsInspector::Type
{
    // Tag opened from a link of the inspected definition.
    struct LinkedTag
    {
        std::string FourCC{};
        std::string TagName{};
    };
}