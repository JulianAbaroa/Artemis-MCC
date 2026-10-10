export module Environment.Health.Type;

import Resolved.Vitality.Type;
import std;

export namespace Environment::Health::Type
{
    using VitalityLayout = Resolved::Vitality::Type::Vitality::Vitality;
    using VitalitySection = Resolved::Vitality::Type::Vitality::Section;
    using VitalityKind = Resolved::Vitality::Type::Vitality::Kind;

    struct Health
    {
        std::uint32_t Handle{};
        std::vector<float> SectionVitalities{};
        bool IsDead{ false };
        std::shared_ptr<const VitalityLayout> Layout{};
    };
}