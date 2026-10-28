module Viewer.Options.State;

import std;

namespace
{
    using Viewer::Options::Type::FlagInfo;
    using Viewer::Options::Type::ScalarInfo;
    using Viewer::Options::Type::k_Flags;
    using Viewer::Options::Type::k_Scalars;
}

namespace Viewer::Options::State
{
    OptionsStore::OptionsStore()
    {
        this->ResetToDefaults();
    }

    auto OptionsStore::IsEnabled(Flag flag) const -> bool
    {
        return m_Flags[static_cast<std::size_t>(flag)].load(std::memory_order_relaxed);
    }

    auto OptionsStore::SetEnabled(Flag flag, bool value) -> void
    {
        m_Flags[static_cast<std::size_t>(flag)].store(value, std::memory_order_relaxed);
    }

    auto OptionsStore::ToggleEnabled(Flag flag) -> void
    {
        this->SetEnabled(flag, !this->IsEnabled(flag));
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
        for (const FlagInfo& info : k_Flags) this->SetEnabled(info.Id, info.Default);
        for (const ScalarInfo& info : k_Scalars) this->SetScalar(info.Id, info.Default);
    }

    auto OptionsStore::Save(std::ostream& stream) const -> void
    {
        for (const FlagInfo& info : k_Flags)
        {
            stream << k_PreferencePrefix << info.Key << "="
                << (this->IsEnabled(info.Id) ? "1" : "0") << "\n";
        }

        for (const ScalarInfo& info : k_Scalars)
        {
            stream << k_PreferencePrefix << info.Key << "="
                << std::format("{:.3f}", this->GetScalar(info.Id)) << "\n";
        }
    }

    auto OptionsStore::Load(std::string_view key, std::string_view value) -> void
    {
        for (const FlagInfo& info : k_Flags)
        {
            if (key != info.Key) continue;

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