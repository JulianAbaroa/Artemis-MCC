module Common.Time.System;

import std;

namespace
{
    using std::chrono::system_clock;
    using std::chrono::seconds;
    using std::chrono::zoned_time;
}

namespace Common::Time::System
{
    auto LocalNow() -> zoned_time<seconds>
    {
        return zoned_time{ std::chrono::current_zone(), std::chrono::floor<seconds>(system_clock::now()) };
    }
}