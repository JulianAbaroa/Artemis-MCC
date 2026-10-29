export module DevTools.Workbench.DefinitionsInspector.Type;

import std;

export namespace DevTools::Workbench::DefinitionsInspector::Type
{
    // Tag opened from a link of the inspected definition.
    struct LinkedTag
    {
        std::string FourCC{};
        std::string TagName{};
    };
}