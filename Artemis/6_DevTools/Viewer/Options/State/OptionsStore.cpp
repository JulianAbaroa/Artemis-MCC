module DevTools.Viewer.Options.State;

import std;

namespace
{
    using DevTools::Viewer::Options::Type::FlagInfo;
    using DevTools::Viewer::Options::Type::Layer;
    using DevTools::Viewer::Options::Type::LayerInfo;
    using DevTools::Viewer::Options::Type::Level;
    using DevTools::Viewer::Options::Type::ScalarInfo;
    using DevTools::Viewer::Options::Type::IsFlagOn;
    using DevTools::Viewer::Options::Type::k_Flags;
    using DevTools::Viewer::Options::Type::k_Layers;
    using DevTools::Viewer::Options::Type::k_Scalars;
}

namespace DevTools::Viewer::Options::State
{
    OptionsStore::OptionsStore()
    {
        this->ResetToDefaults();
    }

    auto OptionsStore::IsEnabled(Flag flag) const -> bool
    {
        const FlagInfo& info = k_Flags[static_cast<std::size_t>(flag)];
        if (info.Owner != Layer::Count) return IsFlagOn(info, this->GetLevel(info.Owner));

        return m_Flags[static_cast<std::size_t>(flag)].load(std::memory_order_relaxed);
    }

    auto OptionsStore::SetEnabled(Flag flag, bool value) -> void
    {
        if (k_Flags[static_cast<std::size_t>(flag)].Owner != Layer::Count) return;

        m_Flags[static_cast<std::size_t>(flag)].store(value, std::memory_order_relaxed);
    }

    auto OptionsStore::ToggleEnabled(Flag flag) -> void
    {
        this->SetEnabled(flag, !this->IsEnabled(flag));
    }

    auto OptionsStore::GetLevel(Layer layer) const -> Level
    {
        return m_Levels[static_cast<std::size_t>(layer)].load(std::memory_order_relaxed);
    }

    auto OptionsStore::SetLevel(Layer layer, Level level) -> void
    {
        m_Levels[static_cast<std::size_t>(layer)].store(level, std::memory_order_relaxed);
    }

    auto OptionsStore::GetScalar(Scalar scalar) const -> float
    {
        return m_Scalars[static_cast<std::size_t>(scalar)].load(std::memory_order_relaxed);
    }

    auto OptionsStore::SetScalar(Scalar scalar, float value) -> void
    {
        const ScalarInfo& info = k_Scalars[static_cast<std::size_t>(scalar)];

        m_Scalars[static_cast<std::size_t>(scalar)].store(
            std::clamp(value, info.Min, info.Max), std::memory_order_relaxed);
    }

    auto OptionsStore::ResetToDefaults() -> void
    {
        for (const LayerInfo& info : k_Layers) this->SetLevel(info.Id, info.Default);
        for (const FlagInfo& info : k_Flags) this->SetEnabled(info.Id, info.Default);
        for (const ScalarInfo& info : k_Scalars) this->SetScalar(info.Id, info.Default);
    }

    auto OptionsStore::Save(std::ostream& stream) const -> void
    {
        for (const LayerInfo& info : k_Layers)
        {
            stream << k_PreferencePrefix << info.Key << "=" << static_cast<int>(this->GetLevel(info.Id)) << "\n";
        }

        for (const FlagInfo& info : k_Flags)
        {
            if (info.Owner != Layer::Count) continue;

            stream << k_PreferencePrefix << info.Key << "="
                << (this->IsEnabled(info.Id) ? "1" : "0") << "\n";
        }

        for (const ScalarInfo& info : k_Scalars)
        {
            stream << k_PreferencePrefix << info.Key << "="
                << (info.IsLogarithmic ? std::format("{:.6g}", this->GetScalar(info.Id)) : std::format("{:.3f}", this->GetScalar(info.Id))) << "\n";
        }
    }

    auto OptionsStore::Load(std::string_view key, std::string_view value) -> void
    {
        for (const LayerInfo& info : k_Layers)
        {
            if (key != info.Key) continue;

            int parsed{ static_cast<int>(info.Default) };
            const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
            if (result.ec != std::errc{} || parsed < static_cast<int>(Level::Off) || parsed > static_cast<int>(Level::Full)) parsed = static_cast<int>(info.Default);

            this->SetLevel(info.Id, static_cast<Level>(parsed));
            return;
        }

        for (const FlagInfo& info : k_Flags)
        {
            if (info.Owner != Layer::Count || key != info.Key) continue;

            this->SetEnabled(info.Id, value == "1" || value == "true");
            return;
        }

        for (const ScalarInfo& info : k_Scalars)
        {
            if (key != info.Key) continue;

            float parsed{ info.Default };
            const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
            if (result.ec != std::errc{}) parsed = info.Default;

            this->SetScalar(info.Id, parsed);
            return;
        }
    }
}