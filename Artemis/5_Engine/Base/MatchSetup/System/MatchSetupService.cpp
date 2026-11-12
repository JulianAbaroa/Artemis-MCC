module Engine.Base.MatchSetup.System;
import :MatchSetup;

import Engine.Base.Hook.System;
import Engine.Base.Lifecycle.Type;
import Engine.Base.MapVariant.Type;
import Engine.Base.Preset.Type;
import Blam.Layout.Game.Type;
import Blam.Layout.Object.Type;
import std;

namespace
{
    using Engine::Base::Lifecycle::Type::Status;
    using Engine::Base::Hook::System::ReadRoundIndex;
    using Engine::Base::Hook::System::ReadRoundState;
    using Engine::Base::MapVariant::Type::ActiveMapVariant;
    using Engine::Base::MapVariant::Type::MapVariantLivePlan;
    using Engine::Base::MatchSetup::Type::GetMatchSetupStep;
    using Engine::Base::MatchSetup::Type::MatchSetup;
    using Engine::Base::MatchSetup::Type::MatchSetupLoad;
    using Engine::Base::MatchSetup::Type::MatchSetupOrigin;
    using Engine::Base::MatchSetup::Type::MatchSetupOutcome;
    using Engine::Base::MatchSetup::Type::MatchSetupRequest;
    using Engine::Base::MatchSetup::Type::MatchSetupResult;
    using Engine::Base::MatchSetup::Type::MatchSetupStep;
    using Engine::Base::MatchSetup::Type::Roster;
    using Engine::Base::MatchSetup::Type::RosterApplication;
    using Engine::Base::MatchSetup::Type::CheckRosterSize;
    using Engine::Base::MatchSetup::Type::ToText;
    using Engine::Base::Preset::Type::PresetLivePlan;
    using Engine::Base::Preset::Type::PresetPick;

    using Blam::Layout::Game::Type::Offset::k_GameGlobalsGameVariant;
    using Blam::Layout::Game::Type::Offset::k_VariantModeFirst;
    using Blam::Layout::Game::Type::Offset::k_VariantModeLast;
    using Blam::Layout::Object::Type::Constant::k_GameGlobalsMapVariant;
    using Blam::Layout::Object::Type::Constant::k_GameGlobalsSize;
    using Blam::Layout::Object::Type::Constant::k_GameModeMultiplayer;
    using Blam::Layout::Object::Type::Constant::k_GameModeTypeOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantAdjust;
    using Blam::Layout::Object::Type::Constant::k_MapVariantAlignMask;
    using Blam::Layout::Object::Type::Constant::k_MapVariantSize;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextGameModeOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextMapVariantOffset;

    // The memory of the match a setup is written into: the live game variant, the runtime map variant the engine builds the round from and
    // its copy in the game options. Where the engine computes them (FUN_18006bcec for the runtime one).
    struct LiveMemory
    {
        std::span<std::uint8_t> Variant{};
        // Empty while the engine has no runtime map variant.
        std::span<std::byte> Runtime{};
        std::span<std::byte> Options{};
        std::uint8_t GameMode{ 0 };
    };

    // return: The live memory of the match, or nothing while there are no game globals.
    // note: Game thread only.
    auto ReadLiveMemory(std::uintptr_t threadContext) -> std::optional<LiveMemory>
    {
        const std::uintptr_t gameGlobals{ *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ThreadContextGameModeOffset) };
        if (gameGlobals == 0) return std::nullopt;

        const std::uintptr_t pointer{ *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ThreadContextMapVariantOffset) };

        LiveMemory live{ .Variant = { reinterpret_cast<std::uint8_t*>(gameGlobals + k_GameGlobalsGameVariant), k_GameGlobalsSize - k_GameGlobalsGameVariant },
            .Options = { reinterpret_cast<std::byte*>(gameGlobals + k_GameGlobalsMapVariant), k_MapVariantSize },
            .GameMode = *reinterpret_cast<const std::uint8_t*>(gameGlobals + k_GameModeTypeOffset) };

        if (pointer != 0)
        {
            const std::uintptr_t runtimeAddress{ (pointer + k_MapVariantAdjust) & ~k_MapVariantAlignMask };
            live.Runtime = { reinterpret_cast<std::byte*>(runtimeAddress), k_MapVariantSize };
        }

        return live;
    }

    // return: Whether the live memory has what the fields of the setup are written into, and the bots service is there for the roster. A
    // request that cannot be served yet waits.
    auto CanServe(const MatchSetup& setup, const std::optional<LiveMemory>& live, bool hasPort) -> bool
    {
        if (setup.Preset && (!live || live->GameMode < k_VariantModeFirst || live->GameMode > k_VariantModeLast)) return false;
        if (setup.Map && (!live || live->Runtime.empty() || live->GameMode != k_GameModeMultiplayer)) return false;
        if (setup.Bots && !hasPort) return false;

        return true;
    }

    // return: The name of a map variant for the logs, "of the load" for none.
    auto MapName(const std::shared_ptr<const ActiveMapVariant>& mapVariant) -> std::string
    {
        return mapVariant != nullptr ? mapVariant->Name : std::string{ "of the load" };
    }

    // return: Why a roster that failed was only partly applied, for the reason of the result.
    auto PartialReason(const RosterApplication& application) -> std::string
    {
        if (!application.Reason.empty()) return "partial: roster: " + application.Reason;

        return std::format("partial: roster {} of {} operations done, see the log", application.Done, application.Total);
    }
}

namespace Engine::Base::MatchSetup::System
{
    auto MatchSetupService::Request(MatchSetup setup, MatchSetupOrigin origin, bool isMerge) -> std::uint64_t
    {
        try
        {
            // The match is where the ticks run, and it starts to be one while it loads.
            const Status status{ m_LifecycleStore.GetStatus() };
            const bool isInMatch{ status == Status::Initialized || status == Status::Running };

            std::optional<std::string> reason{};

            if (setup.Bots) reason = CheckRosterSize(*setup.Bots).transform([](const std::string& text) { return "roster: " + text; });

            if (!reason && isInMatch && setup.Preset) reason = m_PresetService.Check(*setup.Preset).transform([](const std::string& text) { return "preset: " + text; });

            if (!reason && isInMatch && setup.Map && *setup.Map != nullptr)
            {
                reason = m_MapVariantService.Check(**setup.Map).transform([](const std::string& text) { return "map: " + text; });
            }

            if (reason) return this->Reject(*reason);

            const std::uint64_t sequence{ m_MatchSetupStore.Request(std::move(setup), origin, isMerge) };
            m_Log.Message("[MatchSetupService] INFO: Setup {} requested, it waits for the next start or round break.", sequence);
            return sequence;
        }
        catch (const std::exception& exception)
        {
            return this->Reject(exception.what());
        }
    }

    auto MatchSetupService::Reject(std::string reason) -> std::uint64_t
    {
        m_Log.Message("[MatchSetupService] WARNING: Setup refused: {}.", reason);
        return m_MatchSetupStore.Reject(std::move(reason));
    }

    auto MatchSetupService::Cancel(MatchSetupOrigin origin) -> void
    {
        m_MatchSetupStore.DropPending(origin);
    }

    auto MatchSetupService::SetRosterPort(RosterPort* port) -> void
    {
        m_RosterPort.store(port);
    }

    auto MatchSetupService::GetActive() const -> ActiveMatchSetup
    {
        return ActiveMatchSetup{ .Preset = m_PresetService.GetActiveName(), .Map = m_MapVariantService.GetActiveName(),
            .Bots = m_MatchSetupStore.GetActiveBots() };
    }

    auto MatchSetupService::GetResult() const -> MatchSetupResult
    {
        return m_MatchSetupStore.GetResult();
    }

    auto MatchSetupService::HasLoadWork() const -> bool
    {
        return m_MatchSetupStore.HasPending() || !m_PresetService.GetActiveName().empty() || m_MapVariantService.GetActive() != nullptr ||
            m_MatchSetupStore.GetActiveBots().has_value();
    }

    auto MatchSetupService::BeginStart(std::span<const std::byte> configuration) -> void
    {
        // The base values belong to the match that ended: the start holds the ones of this one.
        m_PresetService.DropBase();
        m_MapVariantService.BeginStart(configuration);
    }

    auto MatchSetupService::WriteToStart(std::span<std::byte> configuration, const PresetPick& preset,
        const std::shared_ptr<const ActiveMapVariant>& mapVariant, bool isPresetRequired) -> std::optional<std::string>
    {
        // Everything goes into the copy of the configuration, so a refusal puts it back whole.
        const std::vector<std::byte> backup(configuration.begin(), configuration.end());

        const auto fail = [&](std::string reason) {
            std::ranges::copy(backup, configuration.begin());
            m_PresetService.DropBase();
            return std::optional<std::string>{ std::move(reason) };
        };

        try
        {
            if (preset.Preset != nullptr)
            {
                const auto written{ m_PresetService.WriteToStart(configuration, preset) };
                if (!written) return fail("preset: " + written.error());
                if (!*written && isPresetRequired) return fail("preset: the game variant is not a Megalo variant");
            }

            if (mapVariant != nullptr)
            {
                const auto written{ m_MapVariantService.WriteToStart(configuration, *mapVariant) };
                if (!written) return fail("map: " + written.error());
            }

            return std::nullopt;
        }
        catch (const std::exception& exception)
        {
            return fail(exception.what());
        }
    }

    auto MatchSetupService::ApplyToStart(std::span<std::byte> configuration) -> void
    {
        try
        {
            const std::optional<MatchSetupRequest> request{ m_MatchSetupStore.TakePending() };
            const MatchSetup nothing{};
            const MatchSetup& setup{ request ? request->Value : nothing };
            const std::uint64_t sequence{ request ? request->Sequence : 0 };

            // What a field leaves out stays as the engine holds it: every start writes the active setup again.
            const PresetPick activePreset{ m_PresetService.GetActive() };
            const std::shared_ptr<const ActiveMapVariant> activeMap{ m_MapVariantService.GetActive() };
            const std::optional<Roster> activeBots{ m_MatchSetupStore.GetActiveBots() };

            const PresetPick preset{ setup.Preset ? *setup.Preset : activePreset };
            const std::shared_ptr<const ActiveMapVariant> mapVariant{ setup.Map ? *setup.Map : activeMap };
            const std::optional<Roster> bots{ setup.Bots ? setup.Bots : activeBots };

            if (!request && preset.Preset == nullptr && mapVariant == nullptr && !bots) return;

            std::optional<std::string> failure{};

            if (setup.Bots) failure = CheckRosterSize(*setup.Bots).transform([](const std::string& text) { return "roster: " + text; });
            if (!failure) failure = this->WriteToStart(configuration, preset, mapVariant, setup.Preset.has_value());

            if (failure)
            {
                m_Log.Message("[MatchSetupService] WARNING: Setup {} was not applied at load: {}.", sequence, *failure);
                if (request) m_MatchSetupStore.Finish(MatchSetupResult{ .Outcome = MatchSetupOutcome::Rejected, .Sequence = sequence, .Reason = *failure }, std::nullopt);

                // A refusal changes nothing: this start gets the setup the engine held before.
                const bool isKept{ (activePreset.Preset != nullptr || activeMap != nullptr) && !this->WriteToStart(configuration, activePreset, activeMap, false) };
                m_MatchSetupStore.SetLoad(MatchSetupLoad{ .Bots = activeBots, .IsWritten = isKept });
                return;
            }

            m_PresetService.Adopt(preset);
            m_MapVariantService.Adopt(mapVariant);
            m_Log.Message("[MatchSetupService] INFO: Setup {} written into the start configuration (preset {}, map {}).", sequence,
                preset.Name.empty() ? std::string{ "none" } : preset.Name, mapVariant != nullptr ? mapVariant->Name : std::string{ "none" });

            const bool isWritten{ preset.Preset != nullptr || mapVariant != nullptr };

            // The bots of a request need the match to run: the first tick adds them and gives the result.
            if (request && setup.Bots)
            {
                m_MatchSetupStore.SetLoad(MatchSetupLoad{ .Sequence = sequence, .Bots = bots, .IsWritten = isWritten });
                return;
            }

            if (request) m_MatchSetupStore.Finish(MatchSetupResult{ .Outcome = MatchSetupOutcome::AtLoad, .Sequence = sequence, .Round = 0 }, std::nullopt);

            // The active bots of a start with no request have no result to give.
            m_MatchSetupStore.SetLoad(MatchSetupLoad{ .Bots = bots, .IsWritten = isWritten });
        }
        catch (const std::exception& exception)
        {
            m_PresetService.DropBase();
            m_Log.Message("[MatchSetupService] ERROR: Failed to write the setup into the start ({}).", exception.what());
            m_MatchSetupStore.Reject(exception.what());
        }
    }

    auto MatchSetupService::UpdateLoad(std::uintptr_t threadContext) -> void
    {
        if (threadContext == 0 || m_MatchSetupStore.IsMatchStarted()) return;

        // The service does not throw, but nothing may leave the hook.
        try
        {
            this->ApplyLoad(threadContext);
        }
        catch (const std::exception& exception)
        {
            m_Log.Message("[MatchSetupService] ERROR: The setup at load failed with an exception ({}).", exception.what());
        }
        catch (...)
        {
            m_Log.Message("[MatchSetupService] ERROR: The setup at load failed with an exception.");
        }
    }

    auto MatchSetupService::UpdateBreak(std::uintptr_t threadContext) -> void
    {
        // The break waits for the first tick of the match, which gives the load its bots. A request that waited at that tick is part of
        // the load and is served in the first frame after it, whatever the round.
        if (threadContext == 0 || !m_MatchSetupStore.HasPending() || !m_MatchSetupStore.IsMatchStarted()) return;

        const bool isAtLoad{ m_MatchSetupStore.IsLoadWindow() };

        try
        {
            if (!isAtLoad && GetMatchSetupStep(true, true, ReadRoundState(threadContext)) != MatchSetupStep::Break) return;

            this->ApplyBreak(threadContext, isAtLoad);
        }
        catch (const std::exception& exception)
        {
            m_Log.Message("[MatchSetupService] ERROR: The setup failed with an exception ({}).", exception.what());
        }
        catch (...)
        {
            m_Log.Message("[MatchSetupService] ERROR: The setup failed with an exception.");
        }
    }

    auto MatchSetupService::ApplyLoad(std::uintptr_t threadContext) -> void
    {
        m_MatchSetupStore.SetMatchStarted();
        if (m_MatchSetupStore.HasPending()) m_MatchSetupStore.SetLoadWindow(true);

        const std::optional<MatchSetupLoad> load{ m_MatchSetupStore.TakeLoad() };
        if (!load || !load->Bots) return;

        // A load with no request has no result to give: the active roster is applied again and only the log says how it went.
        const bool hasResult{ load->Sequence != 0 };
        const std::int16_t round{ ReadRoundIndex(threadContext).value_or(-1) };
        RosterPort* const port{ m_RosterPort.load() };

        const auto finish = [&](MatchSetupOutcome outcome, const std::string& reason, const std::optional<Roster>& adopted) {
            if (!reason.empty()) m_Log.Message("[MatchSetupService] WARNING: Setup {} at load: {}.", load->Sequence, reason);
            if (hasResult) m_MatchSetupStore.Finish(MatchSetupResult{ .Outcome = outcome, .Sequence = load->Sequence, .Round = round, .Reason = reason }, adopted);
        };

        // Rejected means nothing was applied: after a preset or a map variant went into the start the result is AtLoad with the reason.
        const auto fail = [&](const std::string& reason) { finish(load->IsWritten ? MatchSetupOutcome::AtLoad : MatchSetupOutcome::Rejected, reason, std::nullopt); };

        if (port == nullptr) return fail("roster: no bots service");

        if (const auto reason{ port->CheckRoster(threadContext, *load->Bots) }) return fail((load->IsWritten ? "partial: roster: " : "roster: ") + *reason);

        const RosterApplication application{ port->ApplyRoster(threadContext, *load->Bots) };

        if (!application.Reason.empty() || application.Done < application.Total)
        {
            // Some operations may be done and there is no rollback: it is a partial result unless nothing was done at all.
            const bool isNothingApplied{ application.Done == 0 && !load->IsWritten };
            if (isNothingApplied) return finish(MatchSetupOutcome::Rejected, "roster: " + (application.Reason.empty() ? std::string{ "no operation done" } : application.Reason), std::nullopt);

            return fail(PartialReason(application));
        }

        finish(MatchSetupOutcome::AtLoad, std::string{}, load->Bots);
        m_Log.Message("[MatchSetupService] INFO: Setup {} applied at load (roster {}).", load->Sequence, ToText(*load->Bots));
    }

    auto MatchSetupService::ApplyBreak(std::uintptr_t threadContext, bool isAtLoad) -> void
    {
        RosterPort* const port{ m_RosterPort.load() };
        const std::optional<LiveMemory> live{ ReadLiveMemory(threadContext) };
        const std::optional<MatchSetupRequest> request{ m_MatchSetupStore.TakePendingIf(
            [&live, port](const MatchSetup& setup) { return CanServe(setup, live, port != nullptr); }) };
        if (!request) return;

        if (isAtLoad) m_MatchSetupStore.SetLoadWindow(false);

        // The request is taken: whatever happens it ends with a result.
        try
        {
            this->Serve(threadContext, port, *request, isAtLoad);
        }
        catch (const std::exception& exception)
        {
            m_Log.Message("[MatchSetupService] ERROR: Setup {} failed with an exception: {}.", request->Sequence, exception.what());
            m_MatchSetupStore.Finish(MatchSetupResult{ .Outcome = MatchSetupOutcome::Rejected, .Sequence = request->Sequence,
                .Round = ReadRoundIndex(threadContext).value_or(-1), .Reason = std::string{ "exception: " } + exception.what() }, std::nullopt);
        }
    }

    auto MatchSetupService::Serve(std::uintptr_t threadContext, RosterPort* port, const MatchSetupRequest& request, bool isAtLoad) -> void
    {
        const MatchSetup& setup{ request.Value };
        const std::optional<LiveMemory> live{ ReadLiveMemory(threadContext) };
        const std::int16_t round{ ReadRoundIndex(threadContext).value_or(-1) };
        const MatchSetupOutcome success{ isAtLoad ? MatchSetupOutcome::AtLoad : MatchSetupOutcome::Applied };
        const char* const when{ isAtLoad ? "at load" : "between rounds" };

        // Rejected means nothing was applied.
        const auto reject = [&](const std::string& reason, bool isFailure) {
            if (isFailure) m_Log.Message("[MatchSetupService] ERROR: Setup {} was not applied {} {}: {}.", request.Sequence, when, round, reason);
            else           m_Log.Message("[MatchSetupService] WARNING: Setup {} was not applied {} {}: {}.", request.Sequence, when, round, reason);

            m_MatchSetupStore.Finish(MatchSetupResult{ .Outcome = MatchSetupOutcome::Rejected, .Sequence = request.Sequence, .Round = round,
                .Reason = reason }, std::nullopt);
        };

        if ((setup.Preset || setup.Map) && !live) return reject("the game memory is not available", false);

        // Checks first: nothing is written until every field passed.
        std::optional<PresetLivePlan> presetPlan{};
        std::optional<MapVariantLivePlan> mapPlan{};

        if (setup.Preset)
        {
            auto plan{ m_PresetService.PrepareLive(*setup.Preset, live->Variant) };
            if (!plan) return reject("preset: " + plan.error(), false);

            presetPlan = std::move(*plan);
        }

        if (setup.Map)
        {
            auto plan{ m_MapVariantService.PrepareLive(*setup.Map, live->Runtime) };
            if (!plan) return reject("map: " + plan.error(), false);

            mapPlan = std::move(*plan);
        }

        if (setup.Bots)
        {
            if (port == nullptr) return reject("roster: no bots service", false);
            if (const auto reason{ port->CheckRoster(threadContext, *setup.Bots) }) return reject("roster: " + *reason, false);
        }

        // Preset, then map variant. Each commit restores its own memory when it does not read back equal, a map variant that fails takes
        // the preset written before it back out, and nothing that throws leaves the memory modified without its base saved.
        const PresetPick previousPreset{ m_PresetService.GetActive() };
        const std::shared_ptr<const ActiveMapVariant> previousMap{ m_MapVariantService.GetActive() };
        std::vector<std::uint8_t> variantBackup{};
        std::vector<std::byte> runtimeBackup{};
        std::vector<std::byte> optionsBackup{};

        const auto restore = [&] {
            if (!variantBackup.empty()) std::ranges::copy(variantBackup, live->Variant.begin());
            if (!runtimeBackup.empty()) std::ranges::copy(runtimeBackup, live->Runtime.begin());
            if (!optionsBackup.empty()) std::ranges::copy(optionsBackup, live->Options.begin());
        };

        try
        {
            if (presetPlan) variantBackup.assign(live->Variant.begin(), live->Variant.end());

            if (mapPlan)
            {
                runtimeBackup.assign(live->Runtime.begin(), live->Runtime.end());
                optionsBackup.assign(live->Options.begin(), live->Options.end());
            }

            if (presetPlan)
            {
                if (const auto done{ m_PresetService.CommitLive(*presetPlan, live->Variant) }; !done) return reject("preset: " + done.error(), true);
            }

            if (mapPlan)
            {
                if (const auto done{ m_MapVariantService.CommitLive(*mapPlan, live->Runtime, live->Options) }; !done)
                {
                    restore();
                    return reject("map: " + done.error(), true);
                }
            }

            if (presetPlan) m_PresetService.AdoptLive(*presetPlan);
            if (mapPlan) m_MapVariantService.Adopt(mapPlan->Pick);
        }
        catch (const std::exception& exception)
        {
            restore();
            m_PresetService.Adopt(previousPreset);
            m_MapVariantService.Adopt(previousMap);
            return reject(std::string{ "exception: " } + exception.what(), true);
        }

        const bool isWritten{ presetPlan.has_value() || mapPlan.has_value() };
        std::optional<Roster> adopted{};
        std::string reason{};

        if (setup.Bots)
        {
            RosterApplication application{};

            try
            {
                application = port->ApplyRoster(threadContext, *setup.Bots);
            }
            catch (const std::exception& exception)
            {
                application.Reason = std::string{ "exception: " } + exception.what();
            }

            if (!application.Reason.empty() || application.Done < application.Total)
            {
                // The roster has no rollback: with a preset or a map variant already written, or some bots done, the setup is Applied
                // with the reason, never Rejected.
                if (!isWritten && application.Done == 0) return reject("roster: " + (application.Reason.empty() ? std::string{ "no operation done" } : application.Reason), true);

                reason = PartialReason(application);
                m_Log.Message("[MatchSetupService] ERROR: Setup {} {} {}: {}.", request.Sequence, when, round, reason);
            }
            else
            {
                adopted = setup.Bots;
            }
        }

        m_MatchSetupStore.Finish(MatchSetupResult{ .Outcome = success, .Sequence = request.Sequence, .Round = round, .Reason = reason }, adopted);

        m_Log.Message("[MatchSetupService] INFO: Setup {} applied {} {} (preset {}, map {}, roster {}).", request.Sequence, when, round,
            setup.Preset ? (setup.Preset->Name.empty() ? std::string{ "none" } : setup.Preset->Name) : std::string{ "unchanged" },
            setup.Map ? MapName(*setup.Map) : std::string{ "unchanged" }, setup.Bots ? ToText(*setup.Bots) : std::string{ "unchanged" });
    }
}