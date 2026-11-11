module DevTools.Workbench.Oracle.State;

import std;

namespace
{
    using DevTools::Workbench::Oracle::Type::k_MaxEveryTicks;
    using DevTools::Workbench::Oracle::Type::k_MaxRays;
    using DevTools::Workbench::Oracle::Type::k_RandomSeed;

    constexpr std::string_view k_RaysKey{ "Rays" };
    constexpr std::string_view k_EveryTicksKey{ "EveryTicks" };

    // return: The number in the text, or nullopt if it is not one.
    auto ParseInt(std::string_view text) -> std::optional<int>
    {
        int parsed{ 0 };
        const auto result{ std::from_chars(text.data(), text.data() + text.size(), parsed) };
        if (result.ec != std::errc{}) return std::nullopt;

        return parsed;
    }
}

namespace DevTools::Workbench::Oracle::State
{
    auto OracleUIStore::IsEnabled() const -> bool
    {
        return m_IsEnabled.load();
    }

    auto OracleUIStore::SetEnabled(bool value) -> void
    {
        m_IsEnabled.store(value);
    }

    auto OracleUIStore::GetRaysPerBatch() const -> int
    {
        return m_RaysPerBatch.load();
    }

    auto OracleUIStore::SetRaysPerBatch(int value) -> void
    {
        m_RaysPerBatch.store(std::clamp(value, 1, k_MaxRays));
    }

    auto OracleUIStore::GetEveryTicks() const -> int
    {
        return m_EveryTicks.load();
    }

    auto OracleUIStore::SetEveryTicks(int value) -> void
    {
        m_EveryTicks.store(std::clamp(value, 1, k_MaxEveryTicks));
    }

    auto OracleUIStore::Save(std::ostream& stream) const -> void
    {
        stream << k_OraclePreferencePrefix << k_RaysKey << '=' << this->GetRaysPerBatch() << '\n';
        stream << k_OraclePreferencePrefix << k_EveryTicksKey << '=' << this->GetEveryTicks() << '\n';
    }

    auto OracleUIStore::Load(std::string_view key, std::string_view value) -> void
    {
        const std::optional<int> parsed{ ParseInt(value) };
        if (!parsed.has_value()) return;

        if (key == k_RaysKey) this->SetRaysPerBatch(*parsed);
        else if (key == k_EveryTicksKey) this->SetEveryTicks(*parsed);
    }

    auto OracleUIStore::Reset(path reportPath, std::string header) -> void
    {
        m_Stats = OracleStats{};
        m_Pending.clear();
        m_Random.seed(k_RandomSeed);
        m_ReportPath = std::move(reportPath);
        m_Header = std::move(header);
        m_Ticks = 0;
        m_LoggedGates = 0;
        m_LastSummary = std::chrono::steady_clock::now();

        this->SetSummary({});
    }

    auto OracleUIStore::GetStats() -> OracleStats&
    {
        return m_Stats;
    }

    auto OracleUIStore::GetPending() -> std::deque<PendingBatch>&
    {
        return m_Pending;
    }

    auto OracleUIStore::GetRandom() -> std::mt19937&
    {
        return m_Random;
    }

    auto OracleUIStore::GetReportPath() const -> const path&
    {
        return m_ReportPath;
    }

    auto OracleUIStore::GetHeader() const -> const std::string&
    {
        return m_Header;
    }

    auto OracleUIStore::AdvanceTicks() -> std::uint64_t
    {
        return ++m_Ticks;
    }

    auto OracleUIStore::MarkLogged(std::uint32_t gate) -> bool
    {
        if ((m_LoggedGates & gate) != 0) return false;

        m_LoggedGates |= gate;
        return true;
    }

    auto OracleUIStore::GetLastSummary() const -> time_point
    {
        return m_LastSummary;
    }

    auto OracleUIStore::SetLastSummary(time_point value) -> void
    {
        m_LastSummary = value;
    }

    auto OracleUIStore::GetSummary() const -> std::string
    {
        std::lock_guard lock{ m_Mutex };
        return m_Summary;
    }

    auto OracleUIStore::SetSummary(std::string value) -> void
    {
        std::lock_guard lock{ m_Mutex };
        m_Summary = std::move(value);
    }
}