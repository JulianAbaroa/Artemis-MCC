module Viewer.Fake.Hook;

namespace Viewer::Fake::Hook
{
    auto FakeDetour::HookedRun(int value) -> int
    {
        return value + 0x288;
    }
}