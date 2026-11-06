#include <gtest/gtest.h>

#include <stdlib.h>

import Common.Engine.Type;
import Common.Math.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Contract.Bridge.Type;
import Blam.Layout.Object.Type;
import Bridge.Codec.Dump.Type;
import Bridge.Codec.Dump.System;
import Bridge.Codec.Layer;
import Bridge.Codec.Tick.Type;
import Blam.Layout.Snapshot.Type;
import Blam.Layout.Snapshot.System;
import Blam.Map.Layer;
import Blam.MapModel.Layer;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Object.Type;
import Perception.Details.Type;
import Perception.Entities.Type;
import Perception.Events.Type;
import Perception.Match.Type;
import Perception.Players.Type;
import Tests.Support.Blam;
import Tests.Support.FilmPass;
import Tests.Support.Pipeline;
import Tests.Support.Replay;
import std;

// The first offline test of the core (M5c, D204): a .sdump replayed through the pipeline must give the records of the .tdump
// recorded in the same session, byte by byte. The synthetic recordings are written the way the DLL writes them. The real ones
// are the golden recordings of Ignore/Regresion.

namespace
{
    namespace SnapshotCodec = Blam::Layout::Snapshot::System;
    namespace TickCodec = Bridge::Codec::Dump::System;
    namespace ObjeOffset = Blam::Layout::Object::Type::Obje::Offset;

    using Common::Math::Type::Vec3;
    using Blam::Layout::Snapshot::Type::Snapshot;
    using Blam::Layout::Snapshot::Type::SnapshotDumpFooter;
    using Blam::Layout::Snapshot::Type::SnapshotLoadInfo;
    using Blam::Layout::Snapshot::Type::SnapshotWriterState;
    using Contract::Bridge::Type::BridgeTick;
    using Tests::Support::Blam::Put;
    using Tests::Support::FilmPass::FilmOutcome;
    using Tests::Support::FilmPass::PeakWorkingSetMegabytes;
    using Tests::Support::FilmPass::RecordingsFolder;
    using Tests::Support::FilmPass::RunFilmPass;
    using Tests::Support::Pipeline::Chain;
    using Tests::Support::Pipeline::FillSyntheticTags;
    using Tests::Support::Pipeline::WithBipedAndPlayer;
    using Tests::Support::Replay::Replay;
    using Tests::Support::Replay::ReplayResult;
    using Tests::Support::Replay::ReplayStatus;

    using Blam::Layout::Snapshot::Type::k_SnapshotDumpEndMarker;
    using Blam::Layout::Snapshot::Type::k_SnapshotFirstAfterLoad;
    using Common::Engine::Type::k_NoHandle;
    using Contract::Bridge::Type::k_BridgeLayoutHash;
    using Tests::Support::Pipeline::k_BipedHandle;

    constexpr std::string_view k_JobsVariable{ "ARTEMIS_REPLAY_JOBS" };
    constexpr std::string_view k_SyntheticMap{ "synthetic.map" };

    // A recording in memory: the two files of one session.
    struct Recording
    {
        std::stringstream Sdump{};
        std::stringstream Tdump{};
    };

    // Writes the snapshots the way the DLL records them: the pipeline over each one, the .tdump from its Tick and the .sdump from
    // what is stored. The stored snapshots are the same ones unless a test wants a corrupt .sdump.
    // param flags: SnapshotRecordHeader::Flags of each record.
    auto Record(Chain& chain, const std::vector<Snapshot>& snapshots, const std::vector<Snapshot>& stored,
        const std::vector<std::uint32_t>& flags) -> Recording
    {
        Recording recording{};
        TickCodec::WriteFileHeader(recording.Tdump, "synthetic", "2026-10-27 12:00:00", "test");
        SnapshotCodec::WriteFileHeader(recording.Sdump, k_BridgeLayoutHash, "2026-10-27 12:00:00", "test");

        Bridge::Codec::Dump::Type::TickDumpWriterState tickState{};
        TickCodec::Begin(tickState);
        SnapshotWriterState snapshotState{};
        auto flat{ std::make_unique<BridgeTick>() };

        SnapshotLoadInfo info{ .MapFileSize = 1234 };
        std::ranges::copy(k_SyntheticMap, info.MapFileName.begin());

        for (std::size_t i{ 0 }; i < snapshots.size(); ++i)
        {
            const std::uint64_t generation{ i + 1 };
            chain.m_Export.m_PipelineService.Run(snapshots[i], generation, {});
            const auto tick{ chain.m_Export.m_TickStore.Acquire() };

            TickCodec::ClearHeader(*flat);
            chain.m_Export.m_FlatService.Flatten(*tick, *flat);
            TickCodec::WriteRecord(recording.Tdump, *flat, tick->Match != nullptr, 0, std::nullopt, tickState);

            SnapshotCodec::WriteRecord(recording.Sdump, std::make_shared<const Snapshot>(stored[i]), generation, 0, flags[i], info,
                snapshotState);
        }

        TickCodec::WriteFooter(recording.Tdump, Bridge::Codec::Dump::Type::TickDumpCounters{ .RecordedTicks = snapshots.size() });
        SnapshotCodec::WriteFooter(recording.Sdump, SnapshotDumpFooter{ .Marker = k_SnapshotDumpEndMarker, .RecordedTicks = snapshots.size() });
        return recording;
    }

    // return: The biped of WithBipedAndPlayer moved to x = 11.
    auto WithBipedMoved() -> Snapshot
    {
        Snapshot snapshot{ WithBipedAndPlayer() };
        Put(snapshot.RawObjects.Edit(k_BipedHandle), ObjeOffset::k_Origin, Vec3{ 11.0f, 20.0f, 30.0f });
        return snapshot;
    }

    // What the Perception of the ctf_c recording gave at one engine tick.
    struct CtfMoment
    {
        std::int16_t RoundIndex{};
        std::array<Perception::Match::Type::MatchTeamScore, 8> TeamScores{};
        Perception::Details::Type::ObjectiveDetails Objectives{};
        std::uint32_t RoundClockTicks{};
        std::int32_t RoundTicksRemaining{};
        std::uint32_t RoundTimeLimitTicks{};
        std::uint8_t RoundLimit{};
        std::int32_t ScoreToWin{};
    };

    // One wait to respawn of a player: first GameTick seen with RespawnTicks above 0, its value, and the GameTick it reached 0.
    struct RespawnWait
    {
        std::uint32_t StartTick{};
        std::uint32_t StartValue{};
        std::uint32_t EndTick{};
    };

    // What the Perception gave over the whole ctf_c recording that is not tied to one tick.
    struct CtfRun
    {
        // Events Artemis generated (EngineSequence 0), in order.
        std::vector<Perception::Events::Type::GameEvent> Generated{};
        // Events of the engine ring with the kind of a flag that reached the log: none should.
        std::size_t EngineFlagEvents{};
        // Whether the Sequence of the log grew by one with each event, from 1.
        bool IsSequenceContinuous{ true };
        // MapId of the Ticks the pipeline assembled in the first records.
        std::set<std::int32_t> MapIds{};
        // Waits of each player (handle index) seen in the Players of the Ticks, in order.
        std::map<std::uint32_t, std::vector<RespawnWait>> Waits{};
        // Ticks in which a player waiting to respawn had a Body, or its wait did not go down by one per GameTick.
        std::size_t RespawnViolations{};
        // Death events of the engine ring.
        std::vector<Perception::Events::Type::GameEvent> Deaths{};
    };

    // The RespawnTicks of a player at the last GameTick it was seen.
    struct RespawnSample
    {
        std::uint32_t GameTick{};
        std::uint32_t Ticks{};
    };

    // Adds the RespawnTicks of the players of one Tick to the waits of the run and checks the invariants of the field.
    auto NoteRespawns(const Perception::Players::Type::Players& players, std::uint32_t gameTick, std::map<std::uint32_t, RespawnSample>& previous,
        CtfRun& run) -> void
    {
        for (const auto& player : players.Rows)
        {
            const std::uint32_t index{ player.Handle & 0xFFFF };
            if (player.RespawnTicks > 0 && player.Body) ++run.RespawnViolations;

            auto& waits{ run.Waits[index] };
            const auto it{ previous.find(index) };
            const std::uint32_t before{ it == previous.end() ? 0u : it->second.Ticks };
            const std::uint32_t beforeTick{ it == previous.end() ? 0u : it->second.GameTick };
            if (it != previous.end() && gameTick == beforeTick) continue;

            if (before > 0 && player.RespawnTicks > 0 && player.RespawnTicks != before - (gameTick - beforeTick)) ++run.RespawnViolations;
            if (player.RespawnTicks > 0 && before == 0) waits.push_back(RespawnWait{ .StartTick = gameTick, .StartValue = player.RespawnTicks });
            if (player.RespawnTicks == 0 && before > 0 && !waits.empty()) waits.back().EndTick = gameTick;
            previous[index] = RespawnSample{ .GameTick = gameTick, .Ticks = player.RespawnTicks };
        }
    }

    // What the Perception gave over a ctf film that the tests ask for: the moments of the engine ticks asked for, the last one and the
    // run. The state of the observer is at the end.
    struct CtfData
    {
        std::map<std::uint32_t, CtfMoment> Moments{};
        CtfMoment Last{};
        CtfRun Run{};

        std::map<std::uint32_t, RespawnSample> Previous{};
        std::uint32_t LastSequence{};
        std::shared_ptr<const Perception::Match::Type::MatchStatus> LastMatch{};
        std::shared_ptr<const Perception::Details::Type::ObjectiveDetails> LastObjectives{};
    };

    // What the Perception gave about the weapons, the projectiles and the entities at one engine tick of an armas recording.
    struct ArmasMoment
    {
        Perception::Details::Type::WeaponDetails Weapons{};
        Perception::Details::Type::ProjectileDetails Projectiles{};
        std::shared_ptr<const Perception::Entities::Type::Entities> Entities{};
        std::shared_ptr<const Perception::Players::Type::Players> Players{};
    };

    // The blasts of the explosives of the explosivos recording.
    struct ExplosiveData
    {
        // Per subtype: (RadiusMin, RadiusMax, Damage) of every tick.
        std::map<std::uint8_t, std::set<std::tuple<float, float, float>>> Blasts{};
        std::set<std::uint32_t> Handles{};
        // Live explosives without a detail, and details without an entity.
        std::size_t Missing{};
        std::size_t Orphans{};
    };

    // The ground and the crouch of the body of player 0 at one engine tick of the suelo recording, with the contact the engine gave.
    struct SueloMoment
    {
        Perception::Players::Type::GroundKind Ground{};
        bool IsGrounded{};
        std::uint32_t AirborneTicks{};
        std::uint32_t GroundObject{};
        float OwnZ{};
        float Crouch{};
        bool IsCrouching{};
        std::uint32_t EngineGroundObject{};
    };

    // A tick of the suelo recording in which the player 0 had another biped than the expected one.
    struct WrongBiped
    {
        std::uint32_t BipedHandle{};
        std::uint32_t GameTick{};
    };

    struct SueloData
    {
        std::map<std::uint32_t, SueloMoment> Moments{};
        std::vector<WrongBiped> WrongBipeds{};
    };

    // The checks of the ground of every body of a film.
    struct GroundData
    {
        std::size_t Bodies{};
        // The texts of the checks that failed.
        std::vector<std::string> Failures{};
    };

    // What the first exit of a lift looked like when the lift published it.
    struct LiftSeen
    {
        std::uint32_t ExitTick{};
        Common::Math::Type::Vec3 ExitVelocity{};
        std::uint32_t ExitBiped{};
        std::size_t StageCount{};
        Common::Math::Type::Vec3 LastDirection{};
        float Acceleration{};
        float MaximumVelocity{};
    };

    struct LiftExitData
    {
        std::map<std::uint32_t, LiftSeen> Seen{};
        std::set<std::uint32_t> ExitBeforeFirst{};
        std::size_t EmptyStages{};
    };

    // The checks of the lifts, the bipeds in a phantom and the powerups of every body of a film.
    struct LiftScanData
    {
        std::size_t EmptyStages{};
        std::size_t InPhantom{};
        std::size_t WithoutLift{};
        std::size_t WithPowerup{};
    };

    // The traits of the players of ctf_c: the gravity of all and the jump height of the player 0.
    struct GravityData
    {
        std::map<std::uint32_t, float> JumpAt{};
        std::vector<std::pair<std::uint32_t, Common::Semantic::Type::Event>> Carries{};
        std::size_t GravityViolations{};
        std::size_t Samples{};
        std::set<float> Jumps{};

        std::uint32_t LastTick{};
        std::uint32_t LastSequence{};
    };

    // The base traits of the players of the lifts film.
    struct BaseTraitsData
    {
        std::size_t Samples{};
        std::size_t Violations{};
    };

    // What the player 0 had at one engine tick of the powerups recording.
    struct PowerupSample
    {
        Common::Semantic::Type::Subtype::Powerup Powerup{};
        std::uint32_t Ticks{};
        float DamageResistance{};
        std::uint8_t ActiveCamo{};
        std::array<std::uint32_t, 3> Durations{};
    };

    auto Dot(const Common::Math::Type::Vec3& a, const Common::Math::Type::Vec3& b) -> float
    {
        return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
    }

    auto Speed(const Common::Math::Type::Vec3& v) -> float
    {
        return std::sqrt(Dot(v, v));
    }

    // Everything the tests gather from one film. A film is replayed once (D269): its observers fill this on the thread of the pass and
    // the tests assert on it afterwards.
    struct FilmResult
    {
        FilmOutcome Pass{};
        CtfData Ctf{};
        std::map<std::uint32_t, ArmasMoment> Armas{};
        ExplosiveData Explosives{};
        SueloData Suelo{};
        GroundData Ground{};
        LiftExitData LiftExit{};
        LiftScanData LiftScan{};
        GravityData Gravity{};
        BaseTraitsData BaseTraits{};
        // The last Match of the film.
        std::shared_ptr<const Perception::Match::Type::MatchStatus> LastMatch{};
        std::map<std::uint32_t, PowerupSample> Powerups{};
    };

    // Engine ticks that the tests ask for in each film.
    const std::vector<std::uint32_t> k_CtfCObjectiveTicks{ 1950, 1960, 4283, 5000, 7870, 15000, 17700, 21300, 28600, 30000 };
    const std::vector<std::uint32_t> k_CtfCClockTicks{ 3001, 11502, 11563, 15001 };
    const std::vector<std::uint32_t> k_CgbRoundTicks{ 100, 604 };
    const std::vector<std::uint32_t> k_CtfDTicks{ 1501, 7801, 8161, 9001, 12656 };
    const std::vector<std::uint32_t> k_ArmasATicks{ 2250, 2300, 2580, 2640, 3300, 4420, 4500, 4540, 5000, 5880, 5950, 6231, 6300, 6410,
        7300, 7400, 7480, 8000, 8500, 8550, 8560, 10765, 10780, 10800, 13790, 13810 };
    const std::vector<std::uint32_t> k_ArmasBTicks{ 1300, 1320, 1420, 1900, 2250, 2310, 2400, 3155, 7700 };

    constexpr std::size_t k_MapIdRecords{ 600 };
    constexpr std::uint32_t k_SueloBiped{ 0xE5B30344 };

    // What each observer of the pass has to keep in a film: the observers that read the same stores share one pass.
    struct FilmPlan
    {
        bool IsCtf{};
        bool IsArmas{};
        bool IsExplosives{};
        bool IsSuelo{};
        bool IsGround{};
        bool IsLiftExit{};
        bool IsLiftScan{};
        bool IsGravity{};
        bool IsBaseTraits{};
        bool IsLastMatch{};
        bool IsPowerups{};
        std::vector<std::uint32_t> CtfTicks{};
        std::vector<std::uint32_t> ArmasTicks{};
    };

    // return: The observers and the ticks of a film, the union of what the tests that use it ask for.
    auto PlanOf(std::string_view film) -> FilmPlan
    {
        const auto isOneOf{ [&](std::initializer_list<std::string_view> films) { return std::ranges::contains(films, film); } };

        FilmPlan plan{};
        plan.IsCtf = isOneOf({ "cgb_displace_1min", "ctf_c", "ctf_d", "armas_a", "armas_b" });
        plan.IsArmas = isOneOf({ "armas_a", "armas_b" });
        plan.IsExplosives = film == "explosivos";
        plan.IsSuelo = film == "suelo";
        plan.IsGround = isOneOf({ "cgb_displace_1min", "ctf_c", "ctf_d", "armas_a", "armas_b", "explosivos", "suelo" });
        plan.IsLiftExit = film == "lifts";
        plan.IsLiftScan = isOneOf({ "cgb_displace_1min", "ctf_c", "ctf_d", "armas_a", "armas_b", "explosivos", "suelo", "lifts" });
        plan.IsGravity = film == "ctf_c";
        plan.IsBaseTraits = film == "lifts";
        plan.IsLastMatch = isOneOf({ "ctf_c", "cgb_displace_1min" });
        plan.IsPowerups = film == "powerups";

        if (film == "ctf_c")
        {
            plan.CtfTicks = k_CtfCObjectiveTicks;
            plan.CtfTicks.insert(plan.CtfTicks.end(), k_CtfCClockTicks.begin(), k_CtfCClockTicks.end());
        }
        if (film == "cgb_displace_1min") plan.CtfTicks = k_CgbRoundTicks;
        if (film == "ctf_d") plan.CtfTicks = k_CtfDTicks;
        if (film == "armas_a") plan.ArmasTicks = k_ArmasATicks;
        if (film == "armas_b") plan.ArmasTicks = k_ArmasBTicks;
        return plan;
    }

    // return: What the Perception gave at one record, as a moment of a ctf film.
    auto MomentOf(const Perception::Match::Type::MatchStatus& match, const Perception::Details::Type::ObjectiveDetails& objectives) -> CtfMoment
    {
        return CtfMoment{ .RoundIndex = match.RoundIndex, .TeamScores = match.TeamScores, .Objectives = objectives,
            .RoundClockTicks = match.RoundClockTicks, .RoundTicksRemaining = match.RoundTicksRemaining,
            .RoundTimeLimitTicks = match.RoundTimeLimitTicks, .RoundLimit = match.RoundLimit, .ScoreToWin = match.ScoreToWin };
    }

    // Keeps what the Perception gave at the engine ticks asked for, the events of the run and the respawn waits.
    // param record: Number of the record, counted from 1.
    auto ObserveCtf(const Chain& chain, std::size_t record, const std::vector<std::uint32_t>& ticks, CtfData& data) -> void
    {
        CtfRun& run{ data.Run };
        const auto match{ chain.m_Environment.m_MatchStore.Acquire() };
        if (const auto tick{ chain.m_Export.m_TickStore.Acquire() }; tick)
        {
            if (record <= k_MapIdRecords) run.MapIds.insert(tick->MapId);
            if (match && tick->Players) NoteRespawns(*tick->Players, match->GameTick, data.Previous, run);
        }
        const auto objectives{ chain.m_Environment.m_ObjectiveDetailsStore.Acquire() };
        if (match && objectives)
        {
            if (std::ranges::contains(ticks, match->GameTick)) data.Moments[match->GameTick] = MomentOf(*match, *objectives);
            data.LastMatch = match;
            data.LastObjectives = objectives;
        }

        const auto log{ chain.m_Environment.m_EventsStore.Acquire() };
        if (!log) return;

        for (const auto& event : log->Rows)
        {
            if (event.Sequence <= data.LastSequence) continue;

            if (event.Sequence != data.LastSequence + 1) run.IsSequenceContinuous = false;
            data.LastSequence = event.Sequence;

            const bool isFlag{ event.Kind >= Common::Semantic::Type::Event::FlagTaken && event.Kind <= Common::Semantic::Type::Event::FlagCaptured };
            if (event.Kind == Common::Semantic::Type::Event::Death) run.Deaths.push_back(event);
            if (event.EngineSequence == 0) run.Generated.push_back(event);
            else if (isFlag) ++run.EngineFlagEvents;
        }
    }

    // Keeps the weapon and projectile details at the engine ticks asked for.
    auto ObserveArmas(const Chain& chain, const std::vector<std::uint32_t>& ticks, std::map<std::uint32_t, ArmasMoment>& moments) -> void
    {
        const auto match{ chain.m_Environment.m_MatchStore.Acquire() };
        const auto weapons{ chain.m_Environment.m_WeaponDetailsStore.Acquire() };
        const auto projectiles{ chain.m_Environment.m_ProjectileDetailsStore.Acquire() };
        const auto entities{ chain.m_Environment.m_EntitiesStore.Acquire() };
        const auto players{ chain.m_Environment.m_PlayersStore.Acquire() };
        if (match && weapons && projectiles && std::ranges::contains(ticks, match->GameTick))
        {
            moments[match->GameTick] = ArmasMoment{ .Weapons = *weapons, .Projectiles = *projectiles, .Entities = entities, .Players = players };
        }
    }

    auto ObserveExplosives(const Chain& chain, ExplosiveData& data) -> void
    {
        using Common::Semantic::Type::Function;

        const auto details{ chain.m_Environment.m_ExplosiveDetailsStore.Acquire() };
        const auto entities{ chain.m_Environment.m_EntitiesStore.Acquire() };
        if (!details || !entities) return;

        std::size_t live{ 0 };
        for (const auto& entity : entities->Rows)
        {
            if (entity.Function != Function::Explosive) continue;

            ++live;
            data.Handles.insert(entity.Handle);
            const auto it{ details->find(entity.Handle) };
            if (it == details->end()) { ++data.Missing; continue; }

            data.Blasts[entity.Subtype].insert({ it->second.RadiusMin, it->second.RadiusMax, it->second.Damage });
        }
        if (details->size() > live) data.Orphans += details->size() - live;
    }

    // Keeps the ground and the crouch of the player 0 at every engine tick.
    auto ObserveSuelo(const Chain& chain, SueloData& data) -> void
    {
        const auto match{ chain.m_Environment.m_MatchStore.Acquire() };
        const auto players{ chain.m_Environment.m_PlayersStore.Acquire() };
        const auto objects{ chain.m_Tables.m_ObjectStore.Acquire() };
        if (!match || !players || !objects || players->Rows.empty() || !players->Rows[0].Body) return;

        const auto& row{ players->Rows[0] };
        const auto& body{ *row.Body };
        const auto objectIt{ objects->find(row.BipedHandle) };
        const auto* biped{ objectIt == objects->end() ? nullptr : std::get_if<Blam::Runtime::Object::Type::Biped::Biped>(&objectIt->second.Specific) };
        data.Moments[match->GameTick] = SueloMoment{ .Ground = body.Ground, .IsGrounded = body.IsGrounded, .AirborneTicks = body.AirborneTicks,
            .GroundObject = body.GroundObject, .OwnZ = body.OwnVelocity.Z, .Crouch = body.CrouchFraction, .IsCrouching = body.IsCrouching,
            .EngineGroundObject = biped ? biped->GroundObjectHandle : 0 };
        if (row.BipedHandle != k_SueloBiped) data.WrongBipeds.push_back(WrongBiped{ .BipedHandle = row.BipedHandle, .GameTick = match->GameTick });
    }

    // Checks that the ground of every body is consistent with the contact of its biped.
    auto ObserveGround(const Chain& chain, std::string_view film, GroundData& data) -> void
    {
        const auto match{ chain.m_Environment.m_MatchStore.Acquire() };
        const auto players{ chain.m_Environment.m_PlayersStore.Acquire() };
        const auto objects{ chain.m_Tables.m_ObjectStore.Acquire() };
        if (!match || !players || !objects) return;

        for (const auto& row : players->Rows)
        {
            if (!row.Body) continue;

            const auto objectIt{ objects->find(row.BipedHandle) };
            if (objectIt == objects->end()) continue;
            const auto* biped{ std::get_if<Blam::Runtime::Object::Type::Biped::Biped>(&objectIt->second.Specific) };
            if (!biped) continue;

            ++data.Bodies;
            const auto& body{ *row.Body };
            const bool hasContact{ objectIt->second.ParentHandle == k_NoHandle && biped->ContactMaterial != Blam::Runtime::Object::Type::Biped::k_NoContact };
            const auto fail{ [&](std::string_view text) { data.Failures.push_back(std::format("{} {}: {}", film, match->GameTick, text)); } };
            if (body.AirborneTicks > 0 && hasContact) fail("airborne with contact");
            if (body.AirborneTicks >= 3 && body.IsGrounded) fail("grounded after 3 ticks");
            if (hasContact && !body.IsGrounded) fail("contact but not grounded");
            if (body.IsCrouching != (body.CrouchFraction >= 0.5f)) fail("crouch");
        }
    }

    // Keeps the first exit of every lift.
    auto ObserveLiftExit(const Chain& chain, LiftExitData& data) -> void
    {
        const auto match{ chain.m_Environment.m_MatchStore.Acquire() };
        const auto lifts{ chain.m_Environment.m_LiftDetailsStore.Acquire() };
        if (!match || !lifts) return;

        for (const auto& [handle, lift] : *lifts)
        {
            if (lift.Stages.empty()) ++data.EmptyStages;
            if (!lift.HasExit || data.Seen.contains(handle)) continue;

            // The exit is published from the tick it happens, so the first tick with it is the tick of the exit.
            if (lift.ExitTick != match->GameTick) data.ExitBeforeFirst.insert(handle);
            data.Seen[handle] = LiftSeen{ .ExitTick = lift.ExitTick, .ExitVelocity = lift.ExitVelocity, .ExitBiped = lift.ExitBiped,
                .StageCount = lift.Stages.size(),
                .LastDirection = lift.Stages.empty() ? Common::Math::Type::Vec3{} : lift.Stages.back().Direction,
                .Acceleration = lift.Stages.empty() ? 0.0f : lift.Stages.back().Acceleration,
                .MaximumVelocity = lift.Stages.empty() ? 0.0f : lift.Stages.back().MaximumVelocity };
        }
    }

    // Counts the lifts without stages, the bipeds in a phantom with no lift near and the players with a powerup.
    auto ObserveLiftScan(const Chain& chain, LiftScanData& data) -> void
    {
        constexpr float k_OwnerRadius{ 3.0f };

        // A18 (D263): only the powerups film has custom powerups. Checked here to replay each film once.
        if (const auto players{ chain.m_Environment.m_PlayersStore.Acquire() })
        {
            for (const auto& player : players->Rows)
            {
                if (player.Powerup != Common::Semantic::Type::Subtype::Powerup::None || player.PowerupTicks != 0) ++data.WithPowerup;
            }
        }

        const auto lifts{ chain.m_Environment.m_LiftDetailsStore.Acquire() };
        const auto objects{ chain.m_Tables.m_ObjectStore.Acquire() };
        if (!lifts || !objects) return;

        for (const auto& entry : *lifts)
        {
            if (entry.second.Stages.empty()) ++data.EmptyStages;
        }

        for (const auto& [handle, object] : *objects)
        {
            if (object.Profile.ObjectKind != Blam::MapModel::Definitions::Type::Object::ObjectKind::Biped || !object.IsInPhantom) continue;

            ++data.InPhantom;
            bool isNear{ false };
            for (const auto& entry : *lifts)
            {
                const auto liftIt{ objects->find(entry.first) };
                if (liftIt == objects->end()) continue;

                const Common::Math::Type::Vec3 d{ liftIt->second.WorldOrigin.X - object.WorldOrigin.X, liftIt->second.WorldOrigin.Y - object.WorldOrigin.Y,
                    liftIt->second.WorldOrigin.Z - object.WorldOrigin.Z };
                if (Speed(d) <= k_OwnerRadius) isNear = true;
            }
            if (!isNear) ++data.WithoutLift;
        }
    }

    // Keeps the flag events of the player 0 and the gravity and jump height of the players.
    auto ObserveGravity(const Chain& chain, GravityData& data) -> void
    {
        using Common::Semantic::Type::Event;

        const auto match{ chain.m_Environment.m_MatchStore.Acquire() };
        const auto players{ chain.m_Environment.m_PlayersStore.Acquire() };
        const auto log{ chain.m_Environment.m_EventsStore.Acquire() };
        if (log)
        {
            for (const auto& event : log->Rows)
            {
                if (event.Sequence <= data.LastSequence) continue;
                data.LastSequence = event.Sequence;
                if (event.EngineSequence == 0 && (event.Kind == Event::FlagTaken || event.Kind == Event::FlagDropped) && (event.CausePlayer & 0xFFFF) == 0)
                {
                    data.Carries.emplace_back(event.GameTick, event.Kind);
                }
            }
        }

        if (!match || !players || match->GameTick == data.LastTick) return;
        data.LastTick = match->GameTick;
        for (const auto& player : players->Rows)
        {
            ++data.Samples;
            if (player.Traits.Gravity != 0.75f) ++data.GravityViolations;
            if ((player.Handle & 0xFFFF) == 0)
            {
                data.JumpAt[match->GameTick] = player.Traits.JumpHeight;
                data.Jumps.insert(player.Traits.JumpHeight);
            }
        }
    }

    auto ObserveBaseTraits(const Chain& chain, BaseTraitsData& data) -> void
    {
        const auto players{ chain.m_Environment.m_PlayersStore.Acquire() };
        if (!players) return;

        for (const auto& player : players->Rows)
        {
            ++data.Samples;
            if (player.Traits.Gravity != 0.75f || player.Traits.MovementSpeed != 1.1f) ++data.Violations;
        }
    }

    // Keeps the player 0 at every engine tick.
    auto ObservePowerups(const Chain& chain, std::map<std::uint32_t, PowerupSample>& samples) -> void
    {
        const auto match{ chain.m_Environment.m_MatchStore.Acquire() };
        const auto players{ chain.m_Environment.m_PlayersStore.Acquire() };
        if (!match || !players || players->Rows.empty()) return;

        const auto& player{ players->Rows.front() };
        samples[match->GameTick] = PowerupSample{ .Powerup = player.Powerup, .Ticks = player.PowerupTicks,
            .DamageResistance = player.Traits.DamageResistance, .ActiveCamo = player.Traits.ActiveCamo, .Durations = match->PowerupDurationTicks };
    }

    // Replays a film once and gathers what the tests that use it ask for.
    auto GatherFilm(const std::string& film, bool isCompared) -> std::shared_ptr<const FilmResult>
    {
        const FilmPlan plan{ PlanOf(film) };
        auto result{ std::make_shared<FilmResult>() };

        result->Pass = RunFilmPass(film, isCompared, [&](const Chain& chain, std::size_t record)
        {
            if (plan.IsCtf) ObserveCtf(chain, record, plan.CtfTicks, result->Ctf);
            if (plan.IsArmas) ObserveArmas(chain, plan.ArmasTicks, result->Armas);
            if (plan.IsExplosives) ObserveExplosives(chain, result->Explosives);
            if (plan.IsSuelo) ObserveSuelo(chain, result->Suelo);
            if (plan.IsGround) ObserveGround(chain, film, result->Ground);
            if (plan.IsLiftExit) ObserveLiftExit(chain, result->LiftExit);
            if (plan.IsLiftScan) ObserveLiftScan(chain, result->LiftScan);
            if (plan.IsGravity) ObserveGravity(chain, result->Gravity);
            if (plan.IsBaseTraits) ObserveBaseTraits(chain, result->BaseTraits);
            if (plan.IsPowerups) ObservePowerups(chain, result->Powerups);
            if (plan.IsLastMatch)
            {
                if (const auto match{ chain.m_Environment.m_MatchStore.Acquire() }; match) result->LastMatch = match;
            }
        });

        CtfData& ctf{ result->Ctf };
        if (ctf.LastMatch && ctf.LastObjectives) ctf.Last = MomentOf(*ctf.LastMatch, *ctf.LastObjectives);
        ctf.LastMatch.reset();
        ctf.LastObjectives.reset();
        return result;
    }

    // The films of each test of ReplayTests. A test that is not here may use any film.
    struct TestFilms
    {
        std::string_view Test{};
        std::vector<std::string_view> Films{};
    };

    const std::vector<TestFilms> k_TestFilms{
        { "CtfObjectivesFollowTheRecordedMatch", { "ctf_c" } },
        { "CtfFlagEventsMatchTheExpectedList", { "ctf_c" } },
        { "CgbFlagsAreTakenAndDroppedSixTimesEachAndNothingElse", { "cgb_displace_1min" } },
        { "CtfCRoundClockIsAStopwatchWithoutLimit", { "ctf_c" } },
        { "MapIdIsTheSameNonZeroValueInEachFilm", { "cgb_displace_1min", "ctf_c", "ctf_d", "armas_a", "armas_b" } },
        { "RespawnTicksCountDownOnePerGameTickOnlyWithoutBody", { "cgb_displace_1min", "ctf_c", "ctf_d", "armas_a", "armas_b" } },
        { "CgbRoundHasOneRoundAndItsClockWaitsForTheEndOfThePreRound", { "cgb_displace_1min" } },
        { "CtfDRoundCountsDownFromTheTimeLimit", { "ctf_d" } },
        { "ArmasAWeaponsAndProjectilesFollowTheRecordedFilm", { "armas_a" } },
        { "ArmasBMagazinesReservesAndBatteryFollowTheRecordedFilm", { "armas_b" } },
        { "ExplosivesCarryTheBlastOfTheirTags", { "explosivos" } },
        { "SueloGroundAndCrouchFollowTheRecordedFilm", { "suelo" } },
        { "GroundIsConsistentWithTheContactInEveryFilm",
            { "cgb_displace_1min", "ctf_c", "ctf_d", "armas_a", "armas_b", "explosivos", "suelo" } },
        { "LiftsCarryTheStagesOfTheirTagsAndTheMeasuredExitOfTheLiftsFilm", { "lifts" } },
        { "EveryLiftOfEveryFilmHasStagesAndABipedInAPhantomIsNearALift",
            { "cgb_displace_1min", "ctf_c", "ctf_d", "armas_a", "armas_b", "explosivos", "suelo", "lifts" } },
        { "CtfCGravityIsConstantAndJumpHeightFollowsTheFlagOfThePlayer0", { "ctf_c" } },
        { "LiftsFilmPlayersHaveTheBaseGravityAndSpeed", { "lifts" } },
        { "VariantSettingsOfCtfCAndCgbMatchTheirGametypes", { "ctf_c", "cgb_displace_1min" } },
        { "PowerupsFilmGivesTheMeasuredPowerupAtEachMoment", { "powerups" } },
        { "PowerupsFilmCountsDownOnePerTickAndTheTraitsFollowOneTickLater", { "powerups" } },
        { "SyntheticRecordingReplaysEqual", {} },
        { "CorruptedObjectDiffers", {} },
        { "NotFromLoadIsSkipped", {} },
        { "SecondLoadStopsTheReplay", {} } };

    // return: The films of Ignore/Regresion that have golden.sdump and, if asked, golden.tdump, sorted by name.
    auto FilmsWithGolden(bool isTdumpNeeded) -> std::vector<std::string>
    {
        std::vector<std::string> films{};
        std::error_code error{};
        for (const auto& entry : std::filesystem::directory_iterator(RecordingsFolder(), error))
        {
            if (entry.is_directory() && std::filesystem::is_regular_file(entry.path() / "golden.sdump", error) &&
                (!isTdumpNeeded || std::filesystem::is_regular_file(entry.path() / "golden.tdump", error)))
            {
                films.push_back(entry.path().filename().string());
            }
        }

        std::ranges::sort(films);
        return films;
    }

    // return: How many passes may run at once: ARTEMIS_REPLAY_JOBS if it is set, else the hardware threads.
    auto ReplayJobs() -> std::size_t
    {
        char* value{ nullptr };
        std::size_t length{};
        std::size_t jobs{ 0 };

        if (_dupenv_s(&value, &length, k_JobsVariable.data()) == 0 && value != nullptr)
        {
            jobs = static_cast<std::size_t>(std::strtoul(value, nullptr, 10));
            std::free(value);
        }

        if (jobs == 0) jobs = std::thread::hardware_concurrency();
        return (std::max)(jobs, std::size_t{ 1 });
    }

    // The passes of the process: each film is replayed once, on a small pool of threads that takes the biggest .sdump first, and the
    // tests wait for the film they need (D269).
    class FilmPasses
    {
    public:
        // Starts the passes of the films that were not started yet.
        // param films: Film and whether its records are compared with golden.tdump.
        auto Launch(const std::map<std::string, bool>& films) -> void
        {
            auto batch{ std::make_shared<Batch>() };
            {
                const std::scoped_lock lock{ m_Mutex };
                for (const auto& [film, isCompared] : films)
                {
                    if (m_Results.contains(film)) continue;

                    Job& job{ batch->Jobs.emplace_back() };
                    job.Film = film;
                    job.IsCompared = isCompared;
                    m_Results.emplace(film, job.Promise.get_future().share());
                }
            }
            if (batch->Jobs.empty()) return;

            // The biggest first: the suite takes as long as the longest film if it starts at once.
            std::error_code error{};
            const auto sizeOf{ [&](const Job& job) { return std::filesystem::file_size(RecordingsFolder() / job.Film / "golden.sdump", error); } };
            std::ranges::stable_sort(batch->Jobs, [&](const Job& a, const Job& b) { return sizeOf(a) > sizeOf(b); });

            m_Jobs = (std::min)(ReplayJobs(), batch->Jobs.size());
            batch->Active = m_Jobs;
            for (std::size_t i{ 0 }; i < m_Jobs; ++i) m_Workers.emplace_back([this, batch] { this->Work(*batch); });
        }

        // return: The result of the film, waiting for its pass to end. The pass runs on this thread if nobody started it.
        auto Get(const std::string& film) -> const FilmResult&
        {
            std::shared_future<std::shared_ptr<const FilmResult>> future{};
            std::promise<std::shared_ptr<const FilmResult>> promise{};
            bool isOwner{ false };
            {
                const std::scoped_lock lock{ m_Mutex };
                if (const auto it{ m_Results.find(film) }; it != m_Results.end())
                {
                    future = it->second;
                }
                else
                {
                    future = promise.get_future().share();
                    m_Results.emplace(film, future);
                    isOwner = true;
                }
            }

            if (isOwner) promise.set_value(this->Run(film, false));
            return *future.get();
        }

        // Waits for the passes that are still running.
        auto Join() -> void
        {
            m_Workers.clear();
        }

    private:
        struct Job
        {
            std::string Film{};
            bool IsCompared{};
            std::promise<std::shared_ptr<const FilmResult>> Promise{};
        };

        struct Batch
        {
            std::vector<Job> Jobs{};
            std::atomic<std::size_t> Next{ 0 };
            std::atomic<std::size_t> Active{ 0 };
        };

        auto Work(Batch& batch) -> void
        {
            for (;;)
            {
                const std::size_t index{ batch.Next.fetch_add(1) };
                if (index >= batch.Jobs.size()) break;

                Job& job{ batch.Jobs[index] };
                job.Promise.set_value(this->Run(job.Film, job.IsCompared));
            }

            if (batch.Active.fetch_sub(1) != 1) return;

            const std::scoped_lock lock{ m_Output };
            std::cout << std::format("[ REPLAY MEM ] peak working set {} MB, {} jobs, hardware_concurrency {}\n", PeakWorkingSetMegabytes(), m_Jobs,
                std::thread::hardware_concurrency()) << std::flush;
        }

        // return: The result of the pass. An exception that escapes the pass is the error of the film.
        auto Run(const std::string& film, bool isCompared) -> std::shared_ptr<const FilmResult>
        {
            std::shared_ptr<const FilmResult> result{};
            try
            {
                result = GatherFilm(film, isCompared);
            }
            catch (const std::exception& exception)
            {
                auto failed{ std::make_shared<FilmResult>() };
                failed->Pass.Error = std::format("exception in the pass of {}: {}", film, exception.what());
                result = failed;
            }

            if (result->Pass.Skip.empty())
            {
                const std::scoped_lock lock{ m_Output };
                std::cout << std::format("[  REPLAY  ] {}: {} records in {:.1f} s\n", film, result->Pass.Records, result->Pass.Seconds) << std::flush;
            }
            return result;
        }

        std::mutex m_Mutex{};
        std::mutex m_Output{};
        std::map<std::string, std::shared_future<std::shared_ptr<const FilmResult>>> m_Results{};
        std::vector<std::jthread> m_Workers{};
        std::size_t m_Jobs{ 1 };
    };

    class ReplayTests : public ::testing::Test
    {
    protected:
        // Starts the passes of the films that the tests selected by the filter need, so they run at once while the first tests wait.
        static auto SetUpTestSuite() -> void
        {
            std::map<std::string, bool> films{};
            const auto want{ [&](const std::string& film, bool isCompared) { films[film] = films[film] || isCompared; } };

            const ::testing::UnitTest& unit{ *::testing::UnitTest::GetInstance() };
            for (int suiteIndex{ 0 }; suiteIndex < unit.total_test_suite_count(); ++suiteIndex)
            {
                const ::testing::TestSuite& suite{ *unit.GetTestSuite(suiteIndex) };
                if (std::string_view{ suite.name() } != "ReplayTests") continue;

                for (int testIndex{ 0 }; testIndex < suite.total_test_count(); ++testIndex)
                {
                    const ::testing::TestInfo& info{ *suite.GetTestInfo(testIndex) };
                    if (!info.should_run()) continue;

                    const std::string_view name{ info.name() };
                    if (name == "Recordings")
                    {
                        for (const std::string& film : FilmsWithGolden(true)) want(film, true);
                    }
                    else if (const auto it{ std::ranges::find(k_TestFilms, name, &TestFilms::Test) }; it != k_TestFilms.end())
                    {
                        for (const std::string_view film : it->Films) want(std::string{ film }, false);
                    }
                    else
                    {
                        for (const std::string& film : FilmsWithGolden(false)) want(film, false);
                    }
                }
            }

            s_Passes.Launch(films);
        }

        static auto TearDownTestSuite() -> void
        {
            s_Passes.Join();
        }

        // return: What the pass over the film gathered, waiting for the pass to end.
        static auto Film(const std::string& film) -> const FilmResult&
        {
            return s_Passes.Get(film);
        }

        auto SetUp() -> void override
        {
            FillSyntheticTags(m_Recorder.m_Map);
            FillSyntheticTags(m_Replayer.m_Map);
        }

        // return: The three snapshots of the synthetic recording: nothing, a biped and a player, the biped moved.
        auto Snapshots() const -> std::vector<Snapshot>
        {
            return { Snapshot{}, WithBipedAndPlayer(), WithBipedMoved() };
        }

        Chain m_Recorder{};
        Chain m_Replayer{};

    private:
        static inline FilmPasses s_Passes{};
    };
}

TEST_F(ReplayTests, SyntheticRecordingReplaysEqual)
{
    const std::vector<Snapshot> snapshots{ this->Snapshots() };
    Recording recording{ Record(m_Recorder, snapshots, snapshots, { k_SnapshotFirstAfterLoad, 0, 0 }) };

    const ReplayResult result{ Replay(m_Replayer.m_Export, recording.Sdump, recording.Tdump) };

    EXPECT_EQ(result.Status, ReplayStatus::Equal) << result.Reason;
    EXPECT_EQ(result.Records, 3u);
}

TEST_F(ReplayTests, CorruptedObjectDiffers)
{
    const std::vector<Snapshot> snapshots{ this->Snapshots() };

    // The .sdump keeps the second snapshot with the sign of the x of the Origin of the biped flipped (one byte).
    std::vector<Snapshot> stored{ snapshots };
    stored[1].RawObjects.Edit(k_BipedHandle)[ObjeOffset::k_Origin + 3] ^= 0x80;

    Recording recording{ Record(m_Recorder, snapshots, stored, { k_SnapshotFirstAfterLoad, 0, 0 }) };

    const ReplayResult result{ Replay(m_Replayer.m_Export, recording.Sdump, recording.Tdump) };

    EXPECT_EQ(result.Status, ReplayStatus::Differs) << result.Reason;
    EXPECT_EQ(result.FirstDifference, 2u);
}

TEST_F(ReplayTests, NotFromLoadIsSkipped)
{
    const std::vector<Snapshot> snapshots{ this->Snapshots() };
    Recording recording{ Record(m_Recorder, snapshots, snapshots, { 0, 0, 0 }) };

    const ReplayResult result{ Replay(m_Replayer.m_Export, recording.Sdump, recording.Tdump) };

    EXPECT_EQ(result.Status, ReplayStatus::Skipped) << result.Reason;
    EXPECT_EQ(result.Records, 0u);
}

TEST_F(ReplayTests, SecondLoadStopsTheReplay)
{
    const std::vector<Snapshot> snapshots{ this->Snapshots() };
    Recording recording{ Record(m_Recorder, snapshots, snapshots, { k_SnapshotFirstAfterLoad, 0, k_SnapshotFirstAfterLoad }) };

    const ReplayResult result{ Replay(m_Replayer.m_Export, recording.Sdump, recording.Tdump) };

    EXPECT_EQ(result.Status, ReplayStatus::Equal) << result.Reason;
    EXPECT_EQ(result.Records, 2u);
}

// --- B4h H.2: the objectives of the ctf_c recording (3 rounds, a user gametype) ---

namespace
{
    using Common::Semantic::Type::FlagState;
    using Perception::Details::Type::ObjectiveDetail;

    // return: The flags of the moment that satisfy the condition.
    template <typename TCondition>
    auto FlagsWhere(const CtfMoment& moment, TCondition condition) -> std::vector<ObjectiveDetail>
    {
        std::vector<ObjectiveDetail> flags{};
        for (const auto& [handle, flag] : moment.Objectives.Flags)
        {
            if (condition(flag)) flags.push_back(flag);
        }

        return flags;
    }

    // return: The index of a player in the player table, from its handle.
    auto PlayerIndexOf(std::uint32_t handle) -> std::uint32_t
    {
        return handle & 0xFFFF;
    }
}

TEST_F(ReplayTests, CtfObjectivesFollowTheRecordedMatch)
{
    const std::vector<std::uint32_t>& ticks{ k_CtfCObjectiveTicks };
    const FilmResult& film{ Film("ctf_c") };
    if (!film.Pass.Error.empty()) FAIL() << film.Pass.Error;
    if (!film.Pass.Skip.empty()) GTEST_SKIP() << film.Pass.Skip;
    const auto& moments{ film.Ctf.Moments };
    const auto& last{ film.Ctf.Last };

    for (const std::uint32_t tick : ticks) ASSERT_TRUE(moments.contains(tick)) << "no record at engine tick " << tick;

    // The flag of the blue base is carried by player 0 at 1950 and dropped at 1960.
    const auto blueAt1950{ FlagsWhere(moments.at(1950), [](const ObjectiveDetail& flag) { return flag.BaseTeam == Common::Team::Type::Team::Blue; }) };
    ASSERT_EQ(blueAt1950.size(), 1u);
    EXPECT_EQ(blueAt1950[0].State, FlagState::Carried);
    EXPECT_EQ(PlayerIndexOf(blueAt1950[0].CarrierPlayer), 0u);

    const auto blueAt1960{ FlagsWhere(moments.at(1960), [](const ObjectiveDetail& flag) { return flag.BaseTeam == Common::Team::Type::Team::Blue; }) };
    ASSERT_EQ(blueAt1960.size(), 1u);
    EXPECT_EQ(blueAt1960[0].State, FlagState::Dropped);

    // A dropped flag whose timer runs.
    EXPECT_FALSE(FlagsWhere(moments.at(4283), [](const ObjectiveDetail& flag) { return flag.State == FlagState::Dropped && flag.IsResetRunning; }).empty());

    // 12 ticks left of the reset timer (one tick of margin).
    EXPECT_FALSE(FlagsWhere(moments.at(7870), [](const ObjectiveDetail& flag) {
        return flag.State == FlagState::Dropped && flag.ResetTicks >= 11 && flag.ResetTicks <= 13; }).empty());

    // Round 2 return with one ally in the zone, and with two allies later.
    EXPECT_FALSE(FlagsWhere(moments.at(17700), [](const ObjectiveDetail& flag) { return flag.State == FlagState::Dropped && flag.AlliesInZone == 1; }).empty());
    EXPECT_FALSE(FlagsWhere(moments.at(21300), [](const ObjectiveDetail& flag) { return flag.State == FlagState::Dropped && flag.AlliesInZone == 2; }).empty());

    // A bot carries a flag (not player 0).
    EXPECT_FALSE(FlagsWhere(moments.at(28600), [](const ObjectiveDetail& flag) {
        return flag.State == FlagState::Carried && flag.CarrierPlayer != 0xFFFFFFFF && PlayerIndexOf(flag.CarrierPlayer) != 0; }).empty());

    EXPECT_EQ(moments.at(5000).RoundIndex, 0);
    EXPECT_EQ(moments.at(15000).RoundIndex, 1);
    EXPECT_EQ(moments.at(30000).RoundIndex, 2);

    // Three captures of one point: the totals of the match add up to 3.
    std::int32_t total{ 0 };
    for (const auto& score : last.TeamScores) total += score.Total;
    EXPECT_EQ(total, 3);
}

// --- B4h H.3: the flag events Artemis generates in the ctf_c recording ---

namespace
{
    using Common::Semantic::Type::Event;

    // A flag event of the expected list: the tick and the kind.
    struct ExpectedEvent
    {
        std::uint32_t Tick{};
        Event Kind{};
    };

    // return: The events of Ignore/Regresion/ctf_c/esperado.txt (lines "tick Kind"), empty if the file is not there.
    auto ReadExpectedEvents() -> std::vector<ExpectedEvent>
    {
        std::ifstream file{ RecordingsFolder() / "ctf_c" / "esperado.txt" };
        const std::map<std::string, Event> kinds{ { "Taken", Event::FlagTaken }, { "Dropped", Event::FlagDropped }, { "Returned", Event::FlagReturned },
            { "Reset", Event::FlagReset }, { "Captured", Event::FlagCaptured } };

        std::vector<ExpectedEvent> expected{};
        std::string line{};
        while (std::getline(file, line))
        {
            std::istringstream words{ line };
            std::uint32_t tick{};
            std::string kind{};
            if ((words >> tick >> kind) && kinds.contains(kind)) expected.push_back(ExpectedEvent{ tick, kinds.at(kind) });
        }

        return expected;
    }
}

TEST_F(ReplayTests, CtfFlagEventsMatchTheExpectedList)
{
    const std::vector<ExpectedEvent> expected{ ReadExpectedEvents() };
    if (expected.empty()) GTEST_SKIP() << "No Ignore/Regresion/ctf_c/esperado.txt";

    const FilmResult& film{ Film("ctf_c") };
    if (!film.Pass.Error.empty()) FAIL() << film.Pass.Error;
    if (!film.Pass.Skip.empty()) GTEST_SKIP() << film.Pass.Skip;
    const auto& run{ film.Ctf.Run };

    EXPECT_EQ(run.EngineFlagEvents, 0u);
    EXPECT_TRUE(run.IsSequenceContinuous);

    // Taken and Dropped must be in the exact tick. Reset, Returned and Captured within 3 ticks. Compared kind by kind, in order.
    std::vector<std::string> differences{};
    for (const Event kind : { Event::FlagTaken, Event::FlagDropped, Event::FlagReturned, Event::FlagReset, Event::FlagCaptured })
    {
        std::vector<std::uint32_t> want{};
        for (const ExpectedEvent& event : expected) if (event.Kind == kind) want.push_back(event.Tick);

        std::vector<std::uint32_t> got{};
        for (const auto& event : run.Generated) if (event.Kind == kind) got.push_back(event.GameTick);

        const std::uint32_t tolerance{ kind == Event::FlagTaken || kind == Event::FlagDropped ? 0u : 3u };
        const std::string name{ std::to_string(static_cast<int>(kind)) };
        if (want.size() != got.size()) differences.push_back(std::format("kind {}: expected {} events, generated {}", name, want.size(), got.size()));

        for (std::size_t i{ 0 }; i < std::min(want.size(), got.size()); ++i)
        {
            const std::uint32_t distance{ want[i] > got[i] ? want[i] - got[i] : got[i] - want[i] };
            if (distance > tolerance) differences.push_back(std::format("kind {} number {}: expected tick {}, generated tick {}", name, i, want[i], got[i]));
        }
    }

    std::string text{};
    for (std::size_t i{ 0 }; i < std::min<std::size_t>(differences.size(), 20); ++i) text += differences[i] + "\n";
    EXPECT_TRUE(differences.empty()) << differences.size() << " differences, the first 20:\n" << text;
    EXPECT_EQ(run.Generated.size(), expected.size());
}

// --- B4h H.4: the flags of the cgb_displace_1min film. Two players run with the enemy flags: the red flag is taken, dropped and taken
// again six times (the user watched this one), the blue flag too. The engine recreates the flag every time it is dropped, with no
// Megalo element for about ten ticks, and neither that nor the lying on the ground is a reset or a return. After the last drop both flags
// lie on the ground until the end of the film. ---

TEST_F(ReplayTests, CgbFlagsAreTakenAndDroppedSixTimesEachAndNothingElse)
{
    const FilmResult& film{ Film("cgb_displace_1min") };
    if (!film.Pass.Error.empty()) FAIL() << film.Pass.Error;
    if (!film.Pass.Skip.empty()) GTEST_SKIP() << film.Pass.Skip;
    const auto& last{ film.Ctf.Last };
    const auto& run{ film.Ctf.Run };

    EXPECT_EQ(run.EngineFlagEvents, 0u);
    EXPECT_TRUE(run.IsSequenceContinuous);

    // Ticks of each kind by the team that plays the side of the flag (the mapping of the film is red 0, blue 1).
    std::map<std::pair<std::uint8_t, Event>, std::vector<std::uint32_t>> ticks{};
    for (const auto& event : run.Generated) ticks[{ event.Modifier, event.Kind }].push_back(event.GameTick);

    using Ticks = std::vector<std::uint32_t>;
    EXPECT_EQ((ticks[{ 0, Event::FlagTaken }]), (Ticks{ 1970, 2082, 2195, 2311, 2434, 2611 }));
    EXPECT_EQ((ticks[{ 0, Event::FlagDropped }]), (Ticks{ 1996, 2136, 2218, 2339, 2460, 2630 }));
    EXPECT_EQ((ticks[{ 1, Event::FlagTaken }]), (Ticks{ 1911, 2023, 2148, 2274, 2329, 2440 }));
    EXPECT_EQ((ticks[{ 1, Event::FlagDropped }]), (Ticks{ 1959, 2082, 2196, 2298, 2395, 2492 }));
    EXPECT_EQ(run.Generated.size(), 24u);

    // They end lying on the ground: no reset, no return and no capture came.
    ASSERT_EQ(last.Objectives.Flags.size(), 2u);
    for (const auto& [handle, flag] : last.Objectives.Flags) EXPECT_EQ(flag.State, FlagState::Dropped) << handle;
}

// --- R5 Q89: the round time and the round limit of the game variant ---

TEST_F(ReplayTests, CtfCRoundClockIsAStopwatchWithoutLimit)
{
    const std::vector<std::uint32_t>& ticks{ k_CtfCClockTicks };
    const FilmResult& film{ Film("ctf_c") };
    if (!film.Pass.Error.empty()) FAIL() << film.Pass.Error;
    if (!film.Pass.Skip.empty()) GTEST_SKIP() << film.Pass.Skip;
    const auto& moments{ film.Ctf.Moments };

    for (const std::uint32_t tick : ticks) ASSERT_TRUE(moments.contains(tick)) << "no record at engine tick " << tick;

    EXPECT_EQ(moments.at(3001).RoundClockTicks, 2280u);
    // Frozen when the round ends and back to 0 when the map is recreated.
    EXPECT_EQ(moments.at(11502).RoundClockTicks, 10071u);
    EXPECT_EQ(moments.at(11563).RoundClockTicks, 0u);
    EXPECT_EQ(moments.at(15001).RoundClockTicks, 2695u);

    for (const std::uint32_t tick : ticks)
    {
        EXPECT_EQ(moments.at(tick).RoundLimit, 3) << tick;
        EXPECT_EQ(moments.at(tick).RoundTimeLimitTicks, 0u) << tick;
        EXPECT_EQ(moments.at(tick).RoundTicksRemaining, -1) << tick;
        EXPECT_EQ(moments.at(tick).ScoreToWin, 1) << tick;
    }
}

// --- H.1: the MapId of the scnr in the Tick ---

TEST_F(ReplayTests, MapIdIsTheSameNonZeroValueInEachFilm)
{
    // The golden.tdump of each film holds the MapId of every tick. Here the first records are enough.
    constexpr std::int32_t k_MapId{ 3006 };

    for (const std::string_view film : { "cgb_displace_1min", "ctf_c", "ctf_d", "armas_a", "armas_b" })
    {
        SCOPED_TRACE(film);
        const FilmResult& result{ Film(std::string{ film }) };
        if (!result.Pass.Error.empty()) { ADD_FAILURE() << result.Pass.Error; continue; }
        if (!result.Pass.Skip.empty()) { std::cout << "[  SKIPPED ] " << film << ": " << result.Pass.Skip << "\n"; continue; }
        const auto& run{ result.Ctf.Run };

        // The five films are on the same base map.
        EXPECT_EQ(run.MapIds, std::set<std::int32_t>{ k_MapId });
    }
}

// --- F2.P: the RespawnTicks of each player ---

TEST_F(ReplayTests, RespawnTicksCountDownOnePerGameTickOnlyWithoutBody)
{
    for (const std::string_view film : { "cgb_displace_1min", "ctf_c", "ctf_d", "armas_a", "armas_b" })
    {
        SCOPED_TRACE(film);
        const FilmResult& result{ Film(std::string{ film }) };
        if (!result.Pass.Error.empty()) { ADD_FAILURE() << result.Pass.Error; continue; }
        if (!result.Pass.Skip.empty()) { std::cout << "[  SKIPPED ] " << film << ": " << result.Pass.Skip << "\n"; continue; }
        const auto& run{ result.Ctf.Run };

        // A player that waits has no Body, and the wait goes down exactly one per GameTick.
        EXPECT_EQ(run.RespawnViolations, 0u);

        if (film != "ctf_c") continue;

        // Measured on ctf_c: the waits (start, value, end) of the players of the handle index 0, 14 and 15.
        const auto waitsOf = [&](std::uint32_t index)
        {
            const auto it{ run.Waits.find(index) };
            return it == run.Waits.end() ? std::vector<RespawnWait>{} : it->second;
        };
        const auto expectWait = [](const RespawnWait& wait, std::uint32_t start, std::uint32_t value, std::uint32_t end)
        {
            EXPECT_EQ(wait.StartTick, start);
            EXPECT_EQ(wait.StartValue, value);
            EXPECT_EQ(wait.EndTick, end);
        };

        const auto waits0{ waitsOf(0) };
        ASSERT_EQ(waits0.size(), 4u);
        expectWait(waits0[0], 2, 599, 601);
        expectWait(waits0[1], 4281, 420, 4701);
        expectWait(waits0[2], 11564, 599, 12163);
        expectWait(waits0[3], 23520, 599, 24119);

        const auto waits14{ waitsOf(14) };
        ASSERT_EQ(waits14.size(), 1u);
        expectWait(waits14[0], 23520, 599, 24119);

        const auto waits15{ waitsOf(15) };
        ASSERT_EQ(waits15.size(), 2u);
        expectWait(waits15[0], 11564, 599, 12163);
        expectWait(waits15[1], 23520, 599, 24119);

        // The wait of 420 ticks follows the death of the player 0.
        EXPECT_TRUE(std::ranges::any_of(run.Deaths, [](const auto& death)
        {
            return death.GameTick == 4280 && (death.EffectPlayer & 0xFFFF) == 0;
        }));
    }
}

TEST_F(ReplayTests, CgbRoundHasOneRoundAndItsClockWaitsForTheEndOfThePreRound)
{
    const std::vector<std::uint32_t>& ticks{ k_CgbRoundTicks };
    const FilmResult& film{ Film("cgb_displace_1min") };
    if (!film.Pass.Error.empty()) FAIL() << film.Pass.Error;
    if (!film.Pass.Skip.empty()) GTEST_SKIP() << film.Pass.Skip;
    const auto& moments{ film.Ctf.Moments };

    for (const std::uint32_t tick : ticks)
    {
        ASSERT_TRUE(moments.contains(tick)) << "no record at engine tick " << tick;
        EXPECT_EQ(moments.at(tick).RoundLimit, 1) << tick;
        EXPECT_EQ(moments.at(tick).ScoreToWin, 5) << tick;
        EXPECT_EQ(moments.at(tick).RoundClockTicks, 0u) << tick;
    }
}

TEST_F(ReplayTests, CtfDRoundCountsDownFromTheTimeLimit)
{
    const std::vector<std::uint32_t>& ticks{ k_CtfDTicks };
    const FilmResult& film{ Film("ctf_d") };
    if (!film.Pass.Error.empty()) FAIL() << film.Pass.Error;
    if (!film.Pass.Skip.empty()) GTEST_SKIP() << film.Pass.Skip;
    const auto& moments{ film.Ctf.Moments };

    for (const std::uint32_t tick : ticks)
    {
        ASSERT_TRUE(moments.contains(tick)) << "no record at engine tick " << tick;
        EXPECT_EQ(moments.at(tick).RoundTimeLimitTicks, 7200u) << tick;
        EXPECT_EQ(moments.at(tick).RoundLimit, 3) << tick;
        EXPECT_EQ(moments.at(tick).ScoreToWin, 1) << tick;
    }

    EXPECT_EQ(moments.at(1501).RoundTicksRemaining, 6300);
    EXPECT_EQ(moments.at(1501).RoundClockTicks, 900u);
    // Round 1 ran out of time.
    EXPECT_EQ(moments.at(7801).RoundTicksRemaining, 0);
    EXPECT_EQ(moments.at(7801).RoundClockTicks, 7200u);
    // The map was recreated: the clock is loaded with the whole limit.
    EXPECT_EQ(moments.at(8161).RoundTicksRemaining, 7200);
    EXPECT_EQ(moments.at(9001).RoundTicksRemaining, 6961);
    // Round 2 ended by a capture: frozen.
    EXPECT_EQ(moments.at(12656).RoundTicksRemaining, 3606);
}

TEST_F(ReplayTests, Recordings)
{
    const std::filesystem::path folder{ RecordingsFolder() };
    const std::vector<std::string> recordings{ FilmsWithGolden(true) };
    if (recordings.empty()) GTEST_SKIP() << "No recording (golden.sdump and golden.tdump) in " << folder.string();

    std::size_t replayed{ 0 };
    std::string skipped{};

    for (const std::string& name : recordings)
    {
        const std::filesystem::path directory{ folder / name };
        SCOPED_TRACE(directory.string());

        const FilmOutcome& pass{ Film(name).Pass };
        if (!pass.Error.empty()) { ADD_FAILURE() << pass.Error; continue; }
        if (!pass.RecordingsFailure.empty()) { ADD_FAILURE() << pass.RecordingsFailure; continue; }
        if (!pass.Warning.empty()) std::cout << "[  WARNING ] " << pass.Warning << "\n";
        if (!pass.Skip.empty() || !pass.IsCompared)
        {
            skipped += pass.RecordingsSkip;
            continue;
        }

        const ReplayResult& result{ pass.Comparison };
        if (result.Status == ReplayStatus::Skipped)
        {
            skipped += directory.string() + ": " + result.Reason + ". ";
            continue;
        }

        ++replayed;
        if (result.Status == ReplayStatus::LayoutChanged)
        {
            ADD_FAILURE() << result.Reason << ": python Tools/tickdump.py diff golden.tdump replay.tdump";
        }
        else if (result.Status == ReplayStatus::Differs)
        {
            ADD_FAILURE() << std::format("record {} (GameTick {}): python Tools/tickdump.py diff golden.tdump replay.tdump",
                result.FirstDifference, result.GameTick);
        }
        else if (result.Status != ReplayStatus::Equal)
        {
            ADD_FAILURE() << result.Reason;
        }
    }

    if (replayed == 0 && !::testing::Test::HasFailure()) GTEST_SKIP() << skipped;
}

// --- W.3, W.4: the weapons and projectiles of the armas_a and armas_b recordings ---

namespace
{
    constexpr std::uint32_t k_Player0{ 0xEC700000 };
    constexpr std::uint32_t k_Biped0{ 0xE43301C4 };
    constexpr std::uint32_t k_Dmr{ 0xE43701C8 };
    constexpr std::uint32_t k_PlasmaRifle{ 0xE42201B3 };
    constexpr std::uint32_t k_PlasmaPistol{ 0xE42301B4 };
    constexpr std::uint32_t k_Rocket{ 0xE42401B5 };
    constexpr std::uint32_t k_RocketTarget{ 0xE42801B9 };
    constexpr std::uint32_t k_PlasmaGrenade{ 0xE4E301CF };
    constexpr std::uint32_t k_GrenadeVictim{ 0xE4D501BA };
    constexpr std::uint32_t k_Needle{ 0xE4FD01B6 };
    constexpr std::uint32_t k_NeedleVictim{ 0xE4F001CF };
    constexpr std::uint32_t k_RocketProjectile{ 0xE4C501C7 };
}

TEST_F(ReplayTests, ArmasAWeaponsAndProjectilesFollowTheRecordedFilm)
{
    using Common::Semantic::Type::ChargeState;
    using Common::Engine::Type::k_NoHandle;

    const std::vector<std::uint32_t>& ticks{ k_ArmasATicks };
    const FilmResult& film{ Film("armas_a") };
    if (!film.Pass.Error.empty()) FAIL() << film.Pass.Error;
    if (!film.Pass.Skip.empty()) GTEST_SKIP() << film.Pass.Skip;
    const auto& moments{ film.Armas };

    for (const std::uint32_t tick : ticks) ASSERT_TRUE(moments.contains(tick)) << "no record at engine tick " << tick;

    const auto weapon{ [&](std::uint32_t tick, std::uint32_t handle) -> const Perception::Details::Type::WeaponDetail&
    {
        const auto& weapons{ moments.at(tick).Weapons };
        EXPECT_TRUE(weapons.contains(handle)) << std::format("no weapon {:08X} at {}", handle, tick);
        static const Perception::Details::Type::WeaponDetail missing{};
        return weapons.contains(handle) ? weapons.at(handle) : missing;
    } };
    const auto projectile{ [&](std::uint32_t tick, std::uint32_t handle) -> const Perception::Details::Type::ProjectileDetail&
    {
        const auto& projectiles{ moments.at(tick).Projectiles };
        EXPECT_TRUE(projectiles.contains(handle)) << std::format("no projectile {:08X} at {}", handle, tick);
        static const Perception::Details::Type::ProjectileDetail missing{};
        return projectiles.contains(handle) ? projectiles.at(handle) : missing;
    } };

    // DMR: drawn, ready, reloading (the reload state outlasts the reload), ready again, empty and reloading.
    EXPECT_EQ(weapon(2250, k_Dmr).MagazineCount, 1);
    EXPECT_FALSE(weapon(2250, k_Dmr).IsEnergyWeapon);
    EXPECT_FALSE(weapon(2250, k_Dmr).CanFireNow);
    EXPECT_FALSE(weapon(2250, k_Dmr).IsReloading);
    EXPECT_EQ(weapon(2300, k_Dmr).Magazines[0].Loaded, 15);
    EXPECT_TRUE(weapon(2300, k_Dmr).CanFireNow);
    EXPECT_TRUE(weapon(2580, k_Dmr).IsReloading);
    EXPECT_FALSE(weapon(2580, k_Dmr).CanFireNow);
    EXPECT_EQ(weapon(2580, k_Dmr).Magazines[0].Loaded, 10);
    EXPECT_FALSE(weapon(2640, k_Dmr).IsReloading);
    EXPECT_TRUE(weapon(2640, k_Dmr).CanFireNow);
    EXPECT_EQ(weapon(2640, k_Dmr).Magazines[0].Loaded, 15);
    EXPECT_TRUE(weapon(3300, k_Dmr).IsReloading);
    EXPECT_EQ(weapon(3300, k_Dmr).Magazines[0].Loaded, 0);

    // Plasma rifle: overheated in its two phases, ready when the second ends.
    EXPECT_EQ(weapon(4420, k_PlasmaRifle).MagazineCount, 0);
    EXPECT_TRUE(weapon(4420, k_PlasmaRifle).IsEnergyWeapon);
    EXPECT_FLOAT_EQ(weapon(4420, k_PlasmaRifle).Energy, 1.0f);
    EXPECT_TRUE(weapon(4420, k_PlasmaRifle).IsOverheated);
    EXPECT_FALSE(weapon(4420, k_PlasmaRifle).CanFireNow);
    EXPECT_TRUE(weapon(4500, k_PlasmaRifle).IsOverheated);
    EXPECT_LT(weapon(4500, k_PlasmaRifle).Heat, 0.4f);
    EXPECT_FALSE(weapon(4540, k_PlasmaRifle).IsOverheated);
    EXPECT_TRUE(weapon(4540, k_PlasmaRifle).CanFireNow);
    EXPECT_TRUE(weapon(5000, k_PlasmaRifle).IsOverheated);

    // Plasma pistol: charge, released early, charge to the end, overheated after the charged shot.
    EXPECT_EQ(weapon(5880, k_PlasmaPistol).ChargeState, ChargeState::Charging);
    EXPECT_NEAR(weapon(5880, k_PlasmaPistol).ChargeFraction, 6.0f / 48.0f, 1e-5f);
    EXPECT_EQ(weapon(5950, k_PlasmaPistol).ChargeState, ChargeState::None);
    EXPECT_FLOAT_EQ(weapon(5950, k_PlasmaPistol).ChargeFraction, 0.0f);
    EXPECT_EQ(weapon(6231, k_PlasmaPistol).ChargeState, ChargeState::Charging);
    EXPECT_NEAR(weapon(6231, k_PlasmaPistol).ChargeFraction, 24.0f / 48.0f, 1e-5f);
    EXPECT_EQ(weapon(6300, k_PlasmaPistol).ChargeState, ChargeState::Charged);
    EXPECT_FLOAT_EQ(weapon(6300, k_PlasmaPistol).ChargeFraction, 1.0f);
    EXPECT_TRUE(weapon(6410, k_PlasmaPistol).IsOverheated);
    EXPECT_FALSE(weapon(6410, k_PlasmaPistol).CanFireNow);

    // Rocket launcher: aims at a Falcon, locks, loses the lock (state 3) keeping the target, then no target, locks again.
    EXPECT_EQ(weapon(7300, k_Rocket).TrackedObject, k_RocketTarget);
    EXPECT_FALSE(weapon(7300, k_Rocket).IsLocked);
    EXPECT_TRUE(weapon(7400, k_Rocket).IsLocked);
    EXPECT_FALSE(weapon(7480, k_Rocket).IsLocked);
    EXPECT_EQ(weapon(7480, k_Rocket).TrackedObject, k_RocketTarget);
    EXPECT_EQ(weapon(8000, k_Rocket).TrackedObject, k_NoHandle);
    EXPECT_TRUE(weapon(8500, k_Rocket).IsLocked);
    EXPECT_EQ(weapon(8560, k_Rocket).Magazines[0].Loaded, 1);

    // Projectiles: a plasma grenade in the hand of its thrower, thrown, stuck to a victim.
    EXPECT_TRUE(projectile(10765, k_PlasmaGrenade).InHand);
    EXPECT_EQ(projectile(10765, k_PlasmaGrenade).StuckTo, k_NoHandle);
    EXPECT_EQ(projectile(10765, k_PlasmaGrenade).OwnerBiped, k_Biped0);
    EXPECT_EQ(projectile(10765, k_PlasmaGrenade).OwnerPlayer, k_Player0);
    EXPECT_FALSE(projectile(10780, k_PlasmaGrenade).InHand);
    EXPECT_EQ(projectile(10780, k_PlasmaGrenade).StuckTo, k_NoHandle);
    EXPECT_EQ(projectile(10800, k_PlasmaGrenade).StuckTo, k_GrenadeVictim);
    EXPECT_GE(projectile(10800, k_PlasmaGrenade).StuckNode, 0);
    EXPECT_EQ(projectile(13790, k_Needle).StuckTo, k_NoHandle);
    EXPECT_EQ(projectile(13810, k_Needle).StuckTo, k_NeedleVictim);
    EXPECT_EQ(projectile(13810, k_Needle).OwnerPlayer, k_Player0);
    EXPECT_EQ(projectile(8550, k_RocketProjectile).OwnerBiped, k_Biped0);
    EXPECT_EQ(projectile(8550, k_RocketProjectile).ShooterRoot, k_Biped0);

    // Entity.Team of a projectile is the team of its OwnerPlayer in the player table (W.4).
    const auto& at8550{ moments.at(8550) };
    ASSERT_NE(at8550.Entities, nullptr);
    ASSERT_NE(at8550.Players, nullptr);
    const auto* rocket{ at8550.Entities->Find(k_RocketProjectile) };
    const auto* owner{ at8550.Players->Find(k_Player0) };
    ASSERT_NE(rocket, nullptr);
    ASSERT_NE(owner, nullptr);
    EXPECT_EQ(rocket->Team, owner->Team);
}

// --- W.4: armas_b, finite ammunition: the reserve and the battery run out ---

TEST_F(ReplayTests, ArmasBMagazinesReservesAndBatteryFollowTheRecordedFilm)
{
    const std::vector<std::uint32_t>& ticks{ k_ArmasBTicks };
    const FilmResult& film{ Film("armas_b") };
    if (!film.Pass.Error.empty()) FAIL() << film.Pass.Error;
    if (!film.Pass.Skip.empty()) GTEST_SKIP() << film.Pass.Skip;
    const auto& moments{ film.Armas };

    for (const std::uint32_t tick : ticks) ASSERT_TRUE(moments.contains(tick)) << "no record at engine tick " << tick;

    const auto weapon{ [&](std::uint32_t tick, std::uint32_t handle) -> const Perception::Details::Type::WeaponDetail&
    {
        const auto& weapons{ moments.at(tick).Weapons };
        EXPECT_TRUE(weapons.contains(handle)) << std::format("no weapon {:08X} at {}", handle, tick);
        static const Perception::Details::Type::WeaponDetail missing{};
        return weapons.contains(handle) ? weapons.at(handle) : missing;
    } };

    constexpr std::uint32_t k_DmrB{ 0xE43501C6 };

    // DMR: last round, empty with a reserve, reloaded, reserve spent, empty and dry, ammunition picked up.
    EXPECT_EQ(weapon(1300, k_DmrB).Magazines[0].Loaded, 1);
    EXPECT_EQ(weapon(1300, k_DmrB).Magazines[0].Reserve, 30);
    EXPECT_EQ(weapon(1320, k_DmrB).Magazines[0].Loaded, 0);
    EXPECT_FALSE(weapon(1320, k_DmrB).CanFireNow);
    EXPECT_EQ(weapon(1420, k_DmrB).Magazines[0].Loaded, 15);
    EXPECT_EQ(weapon(1420, k_DmrB).Magazines[0].Reserve, 15);
    EXPECT_EQ(weapon(1900, k_DmrB).Magazines[0].Loaded, 15);
    EXPECT_EQ(weapon(1900, k_DmrB).Magazines[0].Reserve, 0);
    EXPECT_EQ(weapon(2250, k_DmrB).Magazines[0].Loaded, 0);
    EXPECT_EQ(weapon(2250, k_DmrB).Magazines[0].Reserve, 0);
    EXPECT_FALSE(weapon(2250, k_DmrB).CanFireNow);
    EXPECT_EQ(weapon(2310, k_DmrB).Magazines[0].Reserve, 45);

    // Plasma rifle: the battery drains shot by shot until the weapon cannot fire.
    EXPECT_TRUE(weapon(2400, k_PlasmaRifle).IsEnergyWeapon);
    EXPECT_FLOAT_EQ(weapon(2400, k_PlasmaRifle).Energy, 1.0f);
    EXPECT_NEAR(weapon(3155, k_PlasmaRifle).Energy, 0.97f, 1e-4f);
    EXPECT_FLOAT_EQ(weapon(7700, k_PlasmaRifle).Energy, 0.0f);
    EXPECT_FALSE(weapon(7700, k_PlasmaRifle).CanFireNow);
}

// --- B4x (D259): the explosives of the explosivos recording ---

TEST_F(ReplayTests, ExplosivesCarryTheBlastOfTheirTags)
{
    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::Subtype::Explosive;

    const FilmResult& film{ Film("explosivos") };
    if (!film.Pass.Error.empty()) FAIL() << film.Pass.Error;
    if (!film.Pass.Skip.empty()) GTEST_SKIP() << film.Pass.Skip;
    auto blasts{ film.Explosives.Blasts };
    const auto& handles{ film.Explosives.Handles };
    const std::size_t missing{ film.Explosives.Missing };
    const std::size_t orphans{ film.Explosives.Orphans };

    EXPECT_EQ(missing, 0u);
    EXPECT_EQ(orphans, 0u);
    // The film has the four kinds, and each one has the same blast all the time.
    EXPECT_GE(handles.size(), 4u);
    for (const Explosive kind : { Explosive::FusionCoil, Explosive::Landmine, Explosive::PlasmaBattery, Explosive::PropaneTank })
    {
        const auto& seen{ blasts[static_cast<std::uint8_t>(kind)] };
        ASSERT_EQ(seen.size(), 1u) << "subtype " << static_cast<int>(kind);
        const auto [radiusMin, radiusMax, damage]{ *seen.begin() };
        std::cout << std::format("[ Explosive ] subtype {}: radius {}..{}, damage {}\n", static_cast<int>(kind), radiusMin, radiusMax, damage);
        EXPECT_GT(radiusMax, radiusMin);
        EXPECT_GT(damage, 0.0f);
    }

    EXPECT_EQ(*blasts[static_cast<std::uint8_t>(Explosive::FusionCoil)].begin(), std::make_tuple(0.8f, 2.0f, 200.0f));
    EXPECT_EQ(*blasts[static_cast<std::uint8_t>(Explosive::Landmine)].begin(), std::make_tuple(0.6f, 2.0f, 600.0f));
    EXPECT_EQ(*blasts[static_cast<std::uint8_t>(Explosive::PlasmaBattery)].begin(), std::make_tuple(0.8f, 2.0f, 200.0f));
    EXPECT_EQ(*blasts[static_cast<std::uint8_t>(Explosive::PropaneTank)].begin(), std::make_tuple(0.666f, 3.0f, 300.0f));
}

// --- A3c (D260): the ground and the crouch of the suelo recording ---

namespace
{
    constexpr std::uint32_t k_SueloBlock{ 0xE4030194 };
    constexpr std::uint32_t k_SueloSecondBlock{ 0xE33B00CC };
    constexpr std::uint32_t k_SueloWarthog{ 0xE34000D1 };
    constexpr std::uint32_t k_SueloFlickerBlock{ 0xE4770208 };
}

TEST_F(ReplayTests, SueloGroundAndCrouchFollowTheRecordedFilm)
{
    using Perception::Players::Type::GroundKind;

    const FilmResult& film{ Film("suelo") };
    if (!film.Pass.Error.empty()) FAIL() << film.Pass.Error;
    if (!film.Pass.Skip.empty()) GTEST_SKIP() << film.Pass.Skip;
    const auto& moments{ film.Suelo.Moments };
    for (const WrongBiped& row : film.Suelo.WrongBipeds) EXPECT_EQ(row.BipedHandle, k_SueloBiped) << "GameTick " << row.GameTick;

    const auto at{ [&](std::uint32_t tick) -> const SueloMoment&
    {
        EXPECT_TRUE(moments.contains(tick)) << "no record at engine tick " << tick;
        static const SueloMoment missing{};
        return moments.contains(tick) ? moments.at(tick) : missing;
    } };

    // The jump off the block (1166): not grounded from the first tick, with the vertical speed of the jump. It lands on the BSP (1254).
    EXPECT_EQ(at(1165).AirborneTicks, 0u);
    EXPECT_EQ(at(1165).Ground, GroundKind::Object);
    EXPECT_FALSE(at(1166).IsGrounded);
    EXPECT_EQ(at(1166).Ground, GroundKind::Air);
    EXPECT_EQ(at(1166).AirborneTicks, 1u);
    EXPECT_NEAR(at(1166).OwnZ, 2.3f, 1e-3f);
    EXPECT_EQ(at(1253).Ground, GroundKind::Air);
    EXPECT_EQ(at(1254).Ground, GroundKind::Static);
    EXPECT_TRUE(at(1254).IsGrounded);
    EXPECT_EQ(at(1254).AirborneTicks, 0u);
    EXPECT_EQ(at(1254).GroundObject, k_NoHandle);

    // Three ticks without contact between blocks (890-892, speed -0.33): the first two keep the block, the third is the air.
    EXPECT_EQ(at(889).AirborneTicks, 0u);
    for (const std::uint32_t tick : { 890u, 891u })
    {
        EXPECT_TRUE(at(tick).IsGrounded) << tick;
        EXPECT_EQ(at(tick).Ground, GroundKind::Object) << tick;
        EXPECT_EQ(at(tick).GroundObject, k_SueloFlickerBlock) << tick;
        EXPECT_EQ(at(tick).AirborneTicks, tick - 889u) << tick;
    }
    EXPECT_FALSE(at(892).IsGrounded);
    EXPECT_EQ(at(892).Ground, GroundKind::Air);
    EXPECT_EQ(at(892).AirborneTicks, 3u);
    EXPECT_EQ(at(893).AirborneTicks, 0u);
    EXPECT_TRUE(at(893).IsGrounded);

    // Standing still on an object: the engine gives no handle at some ticks and the last one is kept.
    const auto standing{ [&](std::uint32_t from, std::uint32_t to, std::uint32_t object)
    {
        std::size_t withoutEngineHandle{ 0 };
        for (std::uint32_t tick{ from }; tick <= to; ++tick)
        {
            EXPECT_EQ(at(tick).Ground, GroundKind::Object) << tick;
            EXPECT_TRUE(at(tick).IsGrounded) << tick;
            EXPECT_EQ(at(tick).GroundObject, object) << tick;
            if (at(tick).EngineGroundObject == k_NoHandle) ++withoutEngineHandle;
        }
        EXPECT_GT(withoutEngineHandle, 0u) << "the engine never gave 0xFFFFFFFF in " << from << "-" << to;
    } };
    standing(7201, 7494, k_SueloBlock);
    standing(8401, 8591, k_SueloSecondBlock);

    // On the Warthog, and then seated in it.
    for (std::uint32_t tick{ 7772 }; tick <= 8031; ++tick)
    {
        ASSERT_EQ(at(tick).Ground, GroundKind::Object) << tick;
        ASSERT_EQ(at(tick).GroundObject, k_SueloWarthog) << tick;
    }
    for (std::uint32_t tick{ 8032 }; tick <= 8247; ++tick)
    {
        ASSERT_EQ(at(tick).Ground, GroundKind::Attached) << tick;
        ASSERT_FALSE(at(tick).IsGrounded) << tick;
        ASSERT_EQ(at(tick).AirborneTicks, 0u) << tick;
        ASSERT_EQ(at(tick).GroundObject, k_NoHandle) << tick;
    }

    // The long fall (9030-9416): 387 ticks in the air, and the landing resets the count.
    for (std::uint32_t tick{ 9030 }; tick <= 9416; ++tick)
    {
        ASSERT_EQ(at(tick).Ground, GroundKind::Air) << tick;
        ASSERT_FALSE(at(tick).IsGrounded) << tick;
        ASSERT_EQ(at(tick).AirborneTicks, tick - 9029u) << tick;
    }
    EXPECT_EQ(at(9417).AirborneTicks, 0u);
    EXPECT_TRUE(at(9417).IsGrounded);

    // The crouch moves at most 1/18 per tick, IsCrouching is the half of it, and the three taps cross it.
    for (const auto& [tick, moment] : moments)
    {
        EXPECT_EQ(moment.IsCrouching, moment.Crouch >= 0.5f) << tick;
        if (const auto previous{ moments.find(tick - 1) }; previous != moments.end())
        {
            EXPECT_LE(std::abs(moment.Crouch - previous->second.Crouch), (1.0f / 18.0f) + 1e-4f) << tick;
        }
    }
    for (const std::uint32_t tap : { 1586u, 1633u, 1682u })
    {
        bool isCrouching{ false };
        for (std::uint32_t tick{ tap }; tick < tap + 12; ++tick) isCrouching = isCrouching || at(tick).IsCrouching;
        EXPECT_TRUE(isCrouching) << "tap " << tap;
    }
}

TEST_F(ReplayTests, GroundIsConsistentWithTheContactInEveryFilm)
{
    using Perception::Players::Type::GroundKind;

    std::size_t replayed{ 0 };
    std::string skipped{};
    for (const std::string film : { "cgb_displace_1min", "ctf_c", "ctf_d", "armas_a", "armas_b", "explosivos", "suelo" })
    {
        const FilmResult& result{ Film(film) };
        if (!result.Pass.Error.empty()) { ADD_FAILURE() << result.Pass.Error; continue; }
        if (!result.Pass.Skip.empty())
        {
            skipped += result.Pass.Skip + ". ";
            continue;
        }

        ++replayed;
        for (const std::string& failure : result.Ground.Failures) ADD_FAILURE() << failure;
        EXPECT_GT(result.Ground.Bodies, 0u) << film;
    }

    if (replayed == 0) GTEST_SKIP() << skipped;
}

// --- A12 (D98, D261): the lifts of the lifts recording ---

namespace
{
    // What a lift of the lifts recording must give: the exit of its pass and the last stage of its tag. The values are the ones measured in
    // game (A12 sec. 7) with the engine tick of the Match.
    struct LiftExpectation
    {
        std::uint32_t Handle{};
        std::uint32_t ExitTick{};
        float ExitSpeed{};
        std::size_t StageCount{};
        float Acceleration{};
        float MaximumVelocity{};

        // Dot of the direction of the last stage with the normalized exit velocity that the pass must beat. 0 where the lift does not
        // saturate (the exit is not along the stage).
        float MinimumDot{};
    };
}

TEST_F(ReplayTests, LiftsCarryTheStagesOfTheirTagsAndTheMeasuredExitOfTheLiftsFilm)
{
    // Handle, tick of the exit, |ExitVelocity|, stages, Acceleration and MaximumVelocity of the last one, minimum dot of the direction.
    const std::vector<LiftExpectation> expected{
        { 0xE42201B3, 1227, 7.414f, 2, 11.0f, 7.5f, 0.99f },
        { 0xE42301B4, 1806, 10.916f, 2, 16.0f, 11.0f, 0.99f },
        { 0xE42401B5, 2271, 6.853f, 2, 10.0f, 8.0f, 0.0f },
        { 0xE42501B6, 2912, 10.292f, 1, 30.0f, 25.0f, 0.0f },
        { 0xE42101B2, 4380, 6.109f, 2, 10.0f, 7.0f, 0.0f },
        { 0xE42601B7, 7180, 7.722f, 3, 16.0f, 7.8f, 0.99f },
        { 0xE42701B8, 7590, 8.921f, 3, 16.0f, 8.0f, 0.99f },
        { 0xE42801B9, 8134, 6.426f, 3, 16.0f, 6.5f, 0.99f },
        { 0xE42901BA, 8510, 3.225f, 2, 10.0f, 3.0f, 0.85f },
        { 0xE42A01BB, 8822, 3.408f, 2, 20.0f, 3.5f, 0.99f },
        { 0xE42B01BC, 9362, 7.915f, 2, 15.0f, 8.0f, 0.99f } };

    const FilmResult& film{ Film("lifts") };
    if (!film.Pass.Error.empty()) FAIL() << film.Pass.Error;
    if (!film.Pass.Skip.empty()) GTEST_SKIP() << film.Pass.Skip;
    const auto& seen{ film.LiftExit.Seen };
    const auto& exitBeforeFirst{ film.LiftExit.ExitBeforeFirst };
    const std::size_t emptyStages{ film.LiftExit.EmptyStages };

    EXPECT_EQ(emptyStages, 0u);
    EXPECT_TRUE(exitBeforeFirst.empty());

    for (const LiftExpectation& lift : expected)
    {
        SCOPED_TRACE(std::format("{:08X}", lift.Handle));
        const auto it{ seen.find(lift.Handle) };
        ASSERT_NE(it, seen.end());
        const LiftSeen& found{ it->second };

        std::cout << std::format("[ Lift ] {:08X}: exit {} |v| {:.3f} stages {} last {}/{} dot {:.3f}\n", lift.Handle, found.ExitTick, Speed(found.ExitVelocity),
            found.StageCount, found.Acceleration, found.MaximumVelocity,
            Dot(found.LastDirection, found.ExitVelocity) / Speed(found.ExitVelocity));

        EXPECT_EQ(found.ExitTick, lift.ExitTick);
        EXPECT_NEAR(Speed(found.ExitVelocity), lift.ExitSpeed, 1e-3f);
        EXPECT_EQ(found.StageCount, lift.StageCount);
        EXPECT_FLOAT_EQ(found.Acceleration, lift.Acceleration);
        EXPECT_FLOAT_EQ(found.MaximumVelocity, lift.MaximumVelocity);

        // E42901BA (tunnels) leaves halfway through the turn toward its second stage: 0.903 measured.
        if (lift.MinimumDot > 0.0f) EXPECT_GT(Dot(found.LastDirection, found.ExitVelocity) / Speed(found.ExitVelocity), lift.MinimumDot);
    }

    // The exact velocity of two exits.
    EXPECT_NEAR(seen.at(0xE42201B3).ExitVelocity.X, 0.252f, 1e-3f);
    EXPECT_NEAR(seen.at(0xE42201B3).ExitVelocity.Y, -6.162f, 1e-3f);
    EXPECT_NEAR(seen.at(0xE42201B3).ExitVelocity.Z, 4.114f, 1e-3f);
    EXPECT_NEAR(seen.at(0xE42101B2).ExitVelocity.X, 0.041f, 1e-3f);
    EXPECT_NEAR(seen.at(0xE42101B2).ExitVelocity.Y, 0.155f, 1e-3f);
    EXPECT_NEAR(seen.at(0xE42101B2).ExitVelocity.Z, 6.107f, 1e-3f);
}

TEST_F(ReplayTests, EveryLiftOfEveryFilmHasStagesAndABipedInAPhantomIsNearALift)
{
    constexpr float k_OwnerRadius{ 3.0f };

    std::size_t replayed{ 0 };
    std::string skipped{};
    for (const std::string film : { "cgb_displace_1min", "ctf_c", "ctf_d", "armas_a", "armas_b", "explosivos", "suelo", "lifts" })
    {
        const FilmResult& result{ Film(film) };
        if (!result.Pass.Error.empty()) { ADD_FAILURE() << result.Pass.Error; continue; }
        if (!result.Pass.Skip.empty())
        {
            skipped += result.Pass.Skip + ". ";
            continue;
        }

        ++replayed;
        const LiftScanData& scan{ result.LiftScan };
        std::cout << std::format("[ Lift ] {}: {} biped ticks in a phantom, {} without a lift within {} wu\n", film, scan.InPhantom, scan.WithoutLift, k_OwnerRadius);
        EXPECT_EQ(scan.EmptyStages, 0u) << film;
        EXPECT_EQ(scan.WithPowerup, 0u) << film;
    }

    if (replayed == 0) GTEST_SKIP() << skipped;
}

// --- A19: the traits of the players and the settings of the game variant ---

TEST_F(ReplayTests, CtfCGravityIsConstantAndJumpHeightFollowsTheFlagOfThePlayer0)
{
    using Common::Semantic::Type::Event;

    const FilmResult& film{ Film("ctf_c") };
    if (!film.Pass.Error.empty()) FAIL() << film.Pass.Error;
    if (!film.Pass.Skip.empty()) GTEST_SKIP() << film.Pass.Skip;
    const auto& jumpAt{ film.Gravity.JumpAt };
    const auto& carries{ film.Gravity.Carries };
    const std::size_t gravityViolations{ film.Gravity.GravityViolations };
    const std::size_t samples{ film.Gravity.Samples };
    const auto& jumps{ film.Gravity.Jumps };

    EXPECT_GT(samples, 0u);
    EXPECT_EQ(gravityViolations, 0u);
    EXPECT_EQ(jumps, (std::set<float>{ 1.0f, 1.5f }));
    ASSERT_GT(carries.size(), 10u);

    // The jump height of the player 0 changes one tick after each Taken (to 1.5) and Dropped (to 1.0) of a flag it carries. Measured exceptions:
    // the drop by suicide at 4281 changes two ticks later (the player dies at 4280), and the deaths that start the waits at 11564 and 23520
    // reset the jump to 1.0 without a Dropped event of the player 0.
    constexpr std::uint32_t k_SuicideDrop{ 4281 };
    constexpr std::uint32_t k_SuicideChange{ 4283 };

    std::map<std::uint32_t, float> changes{};
    float previousJump{ 0.0f };
    for (const auto& [tick, jump] : jumpAt)
    {
        if (jump != previousJump) changes.emplace(tick, jump);
        previousJump = jump;
    }

    EXPECT_EQ(carries.size(), 138u);
    std::set<std::uint32_t> explained{};
    for (const auto& [tick, kind] : carries)
    {
        const std::uint32_t change{ tick == k_SuicideDrop ? k_SuicideChange : tick + 1 };
        explained.insert(change);
        ASSERT_TRUE(changes.contains(change)) << tick;
        EXPECT_FLOAT_EQ(changes.at(change), kind == Event::FlagTaken ? 1.5f : 1.0f) << tick;
    }

    std::map<std::uint32_t, float> unexplained{};
    for (const auto& [tick, jump] : changes)
    {
        if (tick != 1 && !explained.contains(tick)) unexplained.emplace(tick, jump);
    }
    EXPECT_EQ(unexplained, (std::map<std::uint32_t, float>{ { 11564, 1.0f }, { 23520, 1.0f } }));
}

TEST_F(ReplayTests, LiftsFilmPlayersHaveTheBaseGravityAndSpeed)
{
    const FilmResult& film{ Film("lifts") };
    if (!film.Pass.Error.empty()) FAIL() << film.Pass.Error;
    if (!film.Pass.Skip.empty()) GTEST_SKIP() << film.Pass.Skip;
    const std::size_t samples{ film.BaseTraits.Samples };
    const std::size_t violations{ film.BaseTraits.Violations };

    EXPECT_GT(samples, 0u);
    EXPECT_EQ(violations, 0u);
}

TEST_F(ReplayTests, VariantSettingsOfCtfCAndCgbMatchTheirGametypes)
{
    struct Expected
    {
        std::string_view Film{};
        std::uint32_t Respawn{};
        std::uint8_t Lives{};
        std::uint8_t RoundsToWin{};
        std::int32_t SuddenDeath{};
        std::uint32_t Grace{};
        std::array<std::uint32_t, 3> Powerups{};
    };

    for (const Expected& want : { Expected{ "ctf_c", 300, 0, 0, 0, 0, { 600, 600, 600 } }, Expected{ "cgb_displace_1min", 300, 0, 0, 0, 300, { 360, 900, 5400 } } })
    {
        SCOPED_TRACE(want.Film);
        const FilmResult& result{ Film(std::string{ want.Film }) };
        if (!result.Pass.Error.empty()) { ADD_FAILURE() << result.Pass.Error; continue; }
        if (!result.Pass.Skip.empty()) { std::cout << "[  SKIPPED ] " << want.Film << ": " << result.Pass.Skip << "\n"; continue; }
        const auto& last{ result.LastMatch };

        ASSERT_NE(last, nullptr);
        EXPECT_EQ(last->RespawnTimeTicks, want.Respawn);
        EXPECT_EQ(last->LivesPerRound, want.Lives);
        EXPECT_EQ(last->TeamLivesPerRound, 0);
        EXPECT_EQ(last->RoundsToWin, want.RoundsToWin);
        EXPECT_EQ(last->SuddenDeathTicks, want.SuddenDeath);
        EXPECT_EQ(last->GracePeriodTicks, want.Grace);
        EXPECT_EQ(last->PowerupDurationTicks, want.Powerups);
    }
}

// --- A18 (D263): the custom powerup of the player in the powerups recording ---

TEST_F(ReplayTests, PowerupsFilmGivesTheMeasuredPowerupAtEachMoment)
{
    using Common::Semantic::Type::Subtype::Powerup;

    // GameTick, powerup and ticks left, measured on the .sdump (red 10 s, blue 15 s, yellow 20 s).
    struct Expected
    {
        std::uint32_t Tick{};
        Powerup Kind{};
        std::uint32_t Ticks{};
    };
    const std::vector<Expected> expected{
        { 1321, Powerup::None, 0 }, { 1322, Powerup::Red, 600 }, { 1921, Powerup::Red, 1 }, { 1922, Powerup::None, 0 },
        { 4016, Powerup::Blue, 900 }, { 4253, Powerup::Blue, 663 }, { 4254, Powerup::Yellow, 1200 }, { 5453, Powerup::Yellow, 1 },
        { 5454, Powerup::None, 0 }, { 6410, Powerup::Blue, 900 }, { 6678, Powerup::Blue, 632 }, { 6679, Powerup::None, 0 },
        { 7451, Powerup::Red, 600 }, { 8050, Powerup::Red, 1 }, { 8051, Powerup::None, 0 } };

    const FilmResult& film{ Film("powerups") };
    if (!film.Pass.Error.empty()) FAIL() << film.Pass.Error;
    if (!film.Pass.Skip.empty()) GTEST_SKIP() << film.Pass.Skip;
    const auto& samples{ film.Powerups };

    for (const Expected& moment : expected)
    {
        const auto it{ samples.find(moment.Tick) };
        ASSERT_NE(it, samples.end()) << "tick " << moment.Tick;
        EXPECT_EQ(it->second.Powerup, moment.Kind) << "tick " << moment.Tick;
        EXPECT_EQ(it->second.Ticks, moment.Ticks) << "tick " << moment.Tick;
    }
}

TEST_F(ReplayTests, PowerupsFilmCountsDownOnePerTickAndTheTraitsFollowOneTickLater)
{
    using Common::Semantic::Type::Subtype::Powerup;

    const FilmResult& film{ Film("powerups") };
    if (!film.Pass.Error.empty()) FAIL() << film.Pass.Error;
    if (!film.Pass.Skip.empty()) GTEST_SKIP() << film.Pass.Skip;
    const auto& samples{ film.Powerups };
    ASSERT_FALSE(samples.empty());

    std::size_t active{ 0 };
    std::size_t counted{ 0 };
    std::size_t withTraits{ 0 };
    const PowerupSample* previous{ nullptr };
    std::uint32_t previousTick{ 0 };
    for (const auto& [tick, sample] : samples)
    {
        if (sample.Powerup == Powerup::None)
        {
            EXPECT_EQ(sample.Ticks, 0u) << "tick " << tick;
        }
        else
        {
            ++active;
            const std::uint32_t total{ sample.Durations[static_cast<std::size_t>(sample.Powerup) - 1] };
            EXPECT_GT(sample.Ticks, 0u) << "tick " << tick;
            EXPECT_LE(sample.Ticks, total) << "tick " << tick;

            // The traits are composed at the start of the tick and the pickup happens after: they follow the powerup one tick later.
            if (sample.Ticks < total)
            {
                ++withTraits;
                if (sample.Powerup == Powerup::Blue) EXPECT_EQ(sample.ActiveCamo, 5) << "tick " << tick;
                else EXPECT_TRUE(std::isinf(sample.DamageResistance)) << "tick " << tick;
            }

            // Down by one per tick, except in a pickup (it goes up to the total).
            if (previous && previousTick + 1 == tick && previous->Powerup == sample.Powerup && sample.Ticks != total)
            {
                ++counted;
                EXPECT_EQ(sample.Ticks, previous->Ticks - 1) << "tick " << tick;
            }
        }

        previous = &sample;
        previousTick = tick;
    }

    EXPECT_GT(active, 0u);
    EXPECT_GT(counted, 0u);
    EXPECT_GT(withTraits, 0u);
}