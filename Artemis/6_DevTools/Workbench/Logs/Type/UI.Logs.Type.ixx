export module UI.Logs.Type;

import std;

export namespace UI::Logs::Type
{
    // Text filter of the logs window.
    struct FilterState
    {
        std::string LowerQuery{};
        bool IsFiltering{ false };
    };
}