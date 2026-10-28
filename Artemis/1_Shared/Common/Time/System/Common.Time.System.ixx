export module Common.Time.System;

import std;

export namespace Common::Time::System
{
    // return: The current local time, to the second, for file names and log lines.
    auto LocalNow() -> std::chrono::zoned_time<std::chrono::seconds>;
}