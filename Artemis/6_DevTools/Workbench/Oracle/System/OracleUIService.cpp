module;

#include "External/imgui/imgui.h"

module DevTools.Workbench.Oracle.System;

import Common.Engine.Type;
import Common.Math.Type;
import Common.Time.System;
import Blam.MapModel.World.Type;
import Views.Oracle.System;
import Bridge.Codec.Tick.Type;
import DevTools.Gui.Widget.System;

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Common::Math::Type::Vec3;
    using Common::Time::System::LocalNow;
    using Engine::Capture::Oracle::Type::OracleOutcome;
    using Engine::Capture::Oracle::Type::OracleRay;
    using Engine::Capture::Oracle::Type::OracleStatus;
    using Views::Oracle::System::Compare;
    using Views::Oracle::System::FormatReport;
    using Views::Oracle::System::FormatSummary;
    using Views::Oracle::System::RandomDirection;
    using Views::Oracle::System::Record;
    using Views::Oracle::System::RecordLag;
    using Views::Oracle::System::ResolveRootObject;
    using Views::Oracle::Type::Constant::k_RandomRayRange;
    using Views::Oracle::Type::Stats::OracleStats;
    using DevTools::Gui::Widget::System::StatusGuiService;
    using DevTools::Workbench::Oracle::Type::k_MaxPendingBatches;
    using DevTools::Workbench::Oracle::Type::k_ReportFolder;
    using DevTools::Workbench::Oracle::Type::k_SummaryInterval;
    using std::filesystem::path;

    constexpr float k_MinDirectionLength{ 0.0001f };
    constexpr ImVec4 k_ErrorColor{ 1.0f, 0.4f, 0.4f, 1.0f };

    // return: The text a status gets in the window.
    auto DescribeStatus(OracleStatus status) -> const char*
    {
        switch (status)
        {
        case OracleStatus::Off: return "Off.";
        case OracleStatus::Waiting: return "Waiting for the game thread to trace the first batch.";
        case OracleStatus::Running: return "Running.";
        case OracleStatus::Unavailable: return "Unavailable: collision_test_vector was not found or faulted, see the logs.";
        }

        return "";
    }
}

namespace DevTools::Workbench::Oracle::System
{
    auto OracleUIService::Draw() -> void
    {
        ImGui::SeparatorText("Raycast oracle");

        bool isEnabled{ m_OracleUIStore.IsEnabled() };
        if (ImGui::Checkbox("Enabled##RaycastOracle", &isEnabled)) m_OracleUIStore.SetEnabled(isEnabled);
        StatusGuiService::ShowTooltip("Traces rays with the engine function the bullets use and compares them with the rays of Artemis."
            " Off by default. The report goes to RaycastOracle in the storage folder.");

        int rays{ m_OracleUIStore.GetRaysPerBatch() };
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
        if (ImGui::InputInt("Rays per batch##RaycastOracle", &rays)) m_OracleUIStore.SetRaysPerBatch(rays);
        StatusGuiService::ShowTooltip("Rays in each batch: the rays of the tick first, random ones for the rest.");

        int everyTicks{ m_OracleUIStore.GetEveryTicks() };
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
        if (ImGui::InputInt("Every ticks##RaycastOracle", &everyTicks)) m_OracleUIStore.SetEveryTicks(everyTicks);
        StatusGuiService::ShowTooltip("Ticks between two batches.");

        const OracleStatus status{ m_OracleStore.GetStatus() };
        if (status == OracleStatus::Unavailable) ImGui::TextColored(k_ErrorColor, "%s", DescribeStatus(status));
        else ImGui::TextUnformatted(DescribeStatus(status));

        ImGui::Text("Batches dropped: %llu  Outcomes dropped: %llu", static_cast<unsigned long long>(m_OracleStore.GetDroppedBatches()),
            static_cast<unsigned long long>(m_OracleStore.GetDroppedOutcomes()));

        const std::string summary{ m_OracleUIStore.GetSummary() };
        if (!summary.empty()) ImGui::TextWrapped("%s", summary.c_str());
    }

    auto OracleUIService::OnTick(const ObservedTick& observed) -> void
    {
        if (!m_OracleUIStore.IsEnabled())
        {
            if (m_OracleStore.IsEnabled()) this->Stop();
            return;
        }

        if (!m_OracleStore.IsEnabled()) this->Start();

        if (!observed.Tick) return;

        // The first tick after a map load: the batches that wait were made for the old map. (ObservedTick::Generation is the tick counter
        // of the AI thread, not a map generation: comparing it cleared the batches and the answers on every tick.)
        if (observed.IsFirstAfterLoad)
        {
            m_OracleUIStore.GetPending().clear();
            m_OracleStore.Cleanup();
        }

        this->Collect();

        const std::uint64_t ticks{ m_OracleUIStore.AdvanceTicks() };
        if (ticks % static_cast<std::uint64_t>(m_OracleUIStore.GetEveryTicks()) == 0) this->Submit(observed);

        this->Summarize(false);
    }

    auto OracleUIService::Start() -> void
    {
        const auto now{ LocalNow() };

        path reportPath{};
        const std::string storage{ m_SettingsStore.GetAppDataDirectory() };
        if (!storage.empty())
        {
            const path folder{ path(storage) / k_ReportFolder };
            std::error_code error{};
            std::filesystem::create_directories(folder, error);
            if (!error) reportPath = folder / std::format("{:%Y%m%d_%H%M%S}.txt", now);
        }

        if (reportPath.empty())
        {
            m_Log.Message("[RaycastOracle] WARNING: No storage folder, so no report file is written. The summary still goes to the log.");
        }

        const std::string header{ std::format("Raycast oracle report, started {:%Y-%m-%d %H:%M:%S}\nRays per batch {} (the rays of the tick first, then random rays"
            " of the eye of the player, range {} m), a batch every {} ticks.\nEngine tests: static = structure and instanced geometry with every"
            " surface and both faces, with objects = the same plus every object type ignoring the root object of the player.\nThe engine traces"
            " one or two ticks after the tick of the rays, so a moving object can differ.", now, m_OracleUIStore.GetRaysPerBatch(),
            k_RandomRayRange, m_OracleUIStore.GetEveryTicks()) };

        m_OracleUIStore.Reset(reportPath, header);
        m_OracleStore.Cleanup();
        m_OracleStore.TryTransition(OracleStatus::Off, OracleStatus::Waiting);
        m_OracleStore.SetEnabled(true);

        m_Log.Message("[RaycastOracle] INFO: Started, {} rays every {} tick(s). Report: {}.", m_OracleUIStore.GetRaysPerBatch(),
            m_OracleUIStore.GetEveryTicks(), reportPath.empty() ? std::string{ "none" } : reportPath.string());
    }

    auto OracleUIService::Stop() -> void
    {
        m_OracleStore.SetEnabled(false);
        m_OracleStore.Cleanup();

        // Unavailable stays: the engine function is not trusted any more until the game restarts.
        if (!m_OracleStore.TryTransition(OracleStatus::Waiting, OracleStatus::Off)) m_OracleStore.TryTransition(OracleStatus::Running, OracleStatus::Off);

        this->Collect();
        this->Summarize(true);

        m_Log.Message("[RaycastOracle] INFO: Stopped.");
    }

    auto OracleUIService::Collect() -> void
    {
        OracleStats& stats{ m_OracleUIStore.GetStats() };
        std::deque<PendingBatch>& pending{ m_OracleUIStore.GetPending() };

        for (const OracleOutcome& outcome : m_OracleStore.TakeOutcomes())
        {
            const auto found{ std::ranges::find_if(pending, [&outcome](const PendingBatch& batch) {
                return batch.Id == outcome.BatchId;
            }) };

            if (found == pending.end() || found->Rays.size() != outcome.Probes.size())
            {
                ++stats.StaleOutcomes;
                continue;
            }

            this->LogOnce(Gate::FirstOutcome, std::format("First outcome from the game thread: {} rays traced.", outcome.Probes.size()));
            RecordLag(stats, outcome.SequenceAtRun > outcome.SequenceAtSubmit ? outcome.SequenceAtRun - outcome.SequenceAtSubmit : 0);

            for (std::size_t index{ 0 }; index < found->Rays.size(); ++index)
            {
                const PendingRay& ray{ found->Rays[index] };
                const OracleProbe& probe{ outcome.Probes[index] };
                if (probe.IsSkipped)
                {
                    ++stats.SkippedRays;
                    continue;
                }

                const OracleSample sample{
                    .Source = ray.Source,
                    .Origin = ray.Origin,
                    .Direction = ray.Direction,
                    .MaxDistance = ray.MaxDistance,
                    .EngineStatic = probe.Static,
                    .EngineObjects = probe.Objects,
                    .EngineStaticNanoseconds = probe.StaticNanoseconds,
                    .EngineObjectsNanoseconds = probe.ObjectsNanoseconds,
                    .OursStatic = ray.OursStatic,
                    .OursTick = ray.OursTick };

                Record(stats, sample, Compare(sample));
            }

            // The batches older than the one answered will not be answered: the engine takes the newest one.
            pending.erase(pending.begin(), found + 1);
        }
    }

    auto OracleUIService::Submit(const ObservedTick& observed) -> void
    {
        const auto& tick{ *observed.Tick };
        if (!tick.Self || !tick.Self->IsAlive)
        {
            this->LogOnce(Gate::NoLocalPlayer, "No alive local player in the tick, so no batch is handed over.");
            return;
        }

        std::uint32_t ignoreHandle{ k_NoHandle };
        if (tick.ObjectTable && tick.Self->BipedHandle != k_NoHandle)
        {
            ignoreHandle = ResolveRootObject(*tick.ObjectTable, tick.Self->BipedHandle);
        }

        const std::size_t limit{ static_cast<std::size_t>(m_OracleUIStore.GetRaysPerBatch()) };

        OracleBatch batch{ .Id = m_OracleStore.AllocateBatchId(), .SequenceAtSubmit = m_OracleStore.GetSequence() };
        PendingBatch pending{ .Id = batch.Id };

        if (tick.Raycasts)
        {
            std::vector<RaycastHit> hits{ tick.Raycasts->AimHit };
            hits.insert(hits.end(), tick.Raycasts->PerceptionHits.begin(), tick.Raycasts->PerceptionHits.end());

            for (const RaycastHit& hit : hits)
            {
                if (batch.Rays.size() >= limit) break;
                if (hit.MaxDistance <= 0.0f) continue;

                const float length{ std::sqrt((hit.Direction.X * hit.Direction.X) + (hit.Direction.Y * hit.Direction.Y)
                    + (hit.Direction.Z * hit.Direction.Z)) };
                if (length < k_MinDirectionLength) continue;

                this->AddRay(batch, pending, RaySource::Tick, PendingRay{ .Origin = hit.Origin, .Direction = hit.Direction,
                    .MaxDistance = hit.MaxDistance, .OursTick = hit }, ignoreHandle);
            }
        }

        while (batch.Rays.size() < limit)
        {
            this->AddRay(batch, pending, RaySource::Random, PendingRay{ .Origin = tick.Self->Position,
                .Direction = RandomDirection(m_OracleUIStore.GetRandom()), .MaxDistance = k_RandomRayRange }, ignoreHandle);
        }

        if (batch.Rays.empty())
        {
            this->LogOnce(Gate::EmptyBatch, "The batch has no rays, so nothing is handed over.");
            return;
        }

        this->LogOnce(Gate::FirstBatch, std::format("First batch handed over: {} rays ({} of the tick).", batch.Rays.size(),
            tick.Raycasts ? tick.Raycasts->PerceptionHits.size() + 1 : 0));

        std::deque<PendingBatch>& waiting{ m_OracleUIStore.GetPending() };
        waiting.push_back(std::move(pending));
        while (waiting.size() > k_MaxPendingBatches) waiting.pop_front();

        m_OracleStore.Submit(std::move(batch));
    }

    auto OracleUIService::AddRay(OracleBatch& batch, PendingBatch& pending, RaySource source, const PendingRay& ray,
        std::uint32_t ignoreHandle) -> void
    {
        PendingRay stored{ ray };
        stored.Source = source;
        stored.OursStatic = m_Raycaster.Cast(ray.Origin, ray.Direction, ray.MaxDistance);

        const OracleRay engineRay{
            .Origin = { ray.Origin.X, ray.Origin.Y, ray.Origin.Z },
            .Vector = { ray.Direction.X * ray.MaxDistance, ray.Direction.Y * ray.MaxDistance, ray.Direction.Z * ray.MaxDistance },
            .IgnoreHandle = ignoreHandle };

        batch.Rays.push_back(engineRay);
        pending.Rays.push_back(std::move(stored));
    }

    auto OracleUIService::LogOnce(Gate gate, std::string_view text) -> void
    {
        if (!m_OracleUIStore.MarkLogged(static_cast<std::uint32_t>(gate))) return;

        m_Log.Message("[RaycastOracle] INFO: {}", text);
    }

    auto OracleUIService::Summarize(bool isForced) -> void
    {
        const auto now{ std::chrono::steady_clock::now() };
        if (!isForced && now - m_OracleUIStore.GetLastSummary() < k_SummaryInterval) return;

        m_OracleUIStore.SetLastSummary(now);

        const std::string summary{ FormatSummary(m_OracleUIStore.GetStats()) };
        m_OracleUIStore.SetSummary(summary);
        m_Log.Message("[RaycastOracle] INFO: {}", summary);

        this->WriteReport();
    }

    auto OracleUIService::WriteReport() -> void
    {
        const path& reportPath{ m_OracleUIStore.GetReportPath() };
        if (reportPath.empty()) return;

        std::ofstream file{ reportPath, std::ios::trunc };
        if (!file.is_open())
        {
            m_Log.Message("[RaycastOracle] ERROR: Failed to write the report file.");
            return;
        }

        file << FormatReport(m_OracleUIStore.GetStats(), m_OracleUIStore.GetHeader());
    }
}