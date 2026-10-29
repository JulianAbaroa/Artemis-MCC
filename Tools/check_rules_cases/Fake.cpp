module DevTools.Viewer.Fake.System;
import DevTools.Viewer.Fake.System:Part;

namespace
{
    using DevTools::Viewer::Fake::Type::Thing;
}

namespace DevTools::Viewer::Fake::System
{
    void FakeService::Run()
    {
        Helper();
        auto x = DevTools::Viewer::Fake::Type::k_Limit; // one; two
	int tabbed{ 0 };
        if (x) {
            return;
        }
        logs.Message("[FakeService] Failed to run.");
    }
}
   