module DevTools.Viewer.Fake.Hook;

namespace DevTools::Viewer::Fake::Hook
{
    auto FakeDetour::HookedRun(int value) -> int
    {
        return value + 0x288;
    }
}