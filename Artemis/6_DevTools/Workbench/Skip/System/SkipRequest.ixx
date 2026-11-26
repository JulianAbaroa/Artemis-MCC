export module DevTools.Workbench.Skip.System;

import DevTools.Workbench.Skip.Type;
import std;

export namespace DevTools::Workbench::Skip::System
{
    // return: What to do with the text of a skip request. A request is one line "<scenario id> <match number>", the number 1-based like the
    // matchIndex of state.json, and it applies only when both name the running match.
    // param scenarioId: The id of the running scenario (the stem of its file in the inbox).
    // param matchNumber: The 1-based number of the running match.
    auto DecideSkip(std::string_view text, std::string_view scenarioId, std::size_t matchNumber) -> Skip::Type::SkipDecision;
}