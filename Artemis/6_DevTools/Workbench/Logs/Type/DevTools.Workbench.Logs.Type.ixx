export module DevTools.Workbench.Logs.Type;

import std;

export namespace DevTools::Workbench::Logs::Type
{
    // Text filter of the logs window.
    struct FilterState
    {
        std::string LowerQuery{};
        bool IsFiltering{ false };
    };
}