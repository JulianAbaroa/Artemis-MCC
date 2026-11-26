export module DevTools.Workbench.Skip.Type;

import std;

export namespace DevTools::Workbench::Skip::Type
{
    // What the bench does with a skip request (the file skip.txt of the exchange folder) while a match runs.
    enum class SkipDecision : std::uint8_t
    {
        // The request names the running match: end it as if it had reached its limit.
        Apply,
        // A well formed request for another scenario or another match: it is old, so it is dropped.
        Stale,
        // Empty or not "<scenario id> <match number>": it is dropped.
        Invalid
    };
}