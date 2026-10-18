export module Viewer.Fake.System;

import Viewer.Other.Type;
import Common.Math.Type;
import std;
import Service.Logs.System;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using Renamed = Common::Math::Type::Vec3;
}

export namespace Viewer::Fake::System
{
    struct Loose
    {
        int Value;
    };

    /* block comment */
    class FakeService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;
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