export module Environment.Health.Type;

import std;

export namespace Environment::Health::Type
{
    struct Health
    {
        std::uint32_t Handle{};
        std::vector<float> SectionVitalities{};
        bool IsDead{ false };
    };
}