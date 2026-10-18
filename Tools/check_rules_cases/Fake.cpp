module Viewer.Fake.System;
import Viewer.Fake.System:Part;

namespace
{
    using Viewer::Fake::Type::Thing;
}

namespace Viewer::Fake::System
{
    void FakeService::Run()
    {
        Helper();
        auto x = Viewer::Fake::Type::k_Limit; // one; two
	int tabbed{ 0 };
        if (x) {
            return;
        }
        logs.Message("[FakeService] Failed to run.");
    }
}
   