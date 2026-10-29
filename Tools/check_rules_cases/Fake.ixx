export module DevTools.Viewer.Fake.System;

import DevTools.Viewer.Other.Type;
import Common.Math.Type;
import std;
import Services.Logs.System;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using Renamed = Common::Math::Type::Vec3;
}

export namespace DevTools::Viewer::Fake::System
{
    struct Loose
    {
        int Value;
    };

    /* block comment */
    class FakeService
    {
    private:
        using LogsService = Services::Logs::System::LogsService;
        int m_Wrong{ 0 };

    public:
        bool GetReady() const;
        auto IsOk() -> bool;
        auto Run() -> void;

    private:
        int m_Count;
        int counter{ 0 };
        bool m_Ready{ false };
        static constexpr int Limit{ 3 };
        static inline FakeService* Instance{ nullptr };
        auto Helper() -> void;
    };
}