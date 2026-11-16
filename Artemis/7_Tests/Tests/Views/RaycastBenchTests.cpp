#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Math.Type;
import Common.Math.System;
import Blam.MapModel.World.System;
import Blam.MapModel.World.Type;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Player.Type;
import Views.Raycast.Type;
import Views.Raycast.System;
import Tests.Support.Bench;
import Tests.Support.FilmPass;
import Tests.Support.Pipeline;
import std;

// Offline bench of the rays of Views (D273, F3.B): what each part of RaycastService costs today for one agent, and what it would cost
// for 16 agents with a cache of their own, with one cache shared by all and with the static world only. The films are the golden
// recordings of Ignore/Regresion, replayed once through the pipeline. Nothing here changes the Tick. The tests are DISABLED_: run them
// with tools/test.py --disabled --filter "DISABLED_RaycastBench.CtfD".

namespace
{
    using Common::Math::Type::Vec3;
    using Blam::MapModel::World::Type::Raycast::BvhBounds;
    using Blam::MapModel::World::Type::Raycast::BvhNode;
    using Tests::Support::Bench::Micros;
    using Tests::Support::Bench::Series;
    using Tests::Support::Bench::Stats;
    using Tests::Support::FilmPass::FilmOutcome;
    using Tests::Support::FilmPass::RecordingsFolder;
    using Tests::Support::FilmPass::RunFilmPass;
    using Tests::Support::Pipeline::Chain;
    using Views::Raycast::System::ArmFixedScene;
    using Views::Raycast::System::ArmMobileScene;
    using Views::Raycast::System::BuildSelfExclusion;
    using Views::Raycast::System::CastSceneRay;
    using Views::Raycast::System::CastStatic;
    using Views::Raycast::System::GatherSceneObjects;
    using Views::Raycast::System::PerceptionDirections;
    using Views::Raycast::System::PrepareMeshBvhs;
    using Views::Raycast::System::PreparePart;
    using Views::Raycast::System::ResolveAimRange;
    using Views::Raycast::System::ResolvePerceptionOrigin;
    using Views::Raycast::Type::CachedPart;
    using Views::Raycast::Type::FixedRebuildRule;
    using Views::Raycast::Type::MobileRebuildRule;
    using Views::Raycast::Type::PerceptionOrigin;
    using Views::Raycast::Type::RaycastHit;
    using Views::Raycast::Type::RaycastScene;
    using Views::Raycast::Type::SceneCounters;
    using Views::Raycast::Type::SceneGatherCounters;
    using Views::Raycast::Type::SceneObject;
    using Views::Raycast::Type::SceneObjects;
    using Views::Raycast::Type::ScenePiece;
    using Views::Raycast::Type::ScenePartsCache;
    using Views::Raycast::Type::SelfExclusion;
    using std::chrono::steady_clock;

    using Common::Engine::Type::k_NoHandle;
    using Tests::Support::Bench::k_NanosPerMicro;
    using Blam::MapModel::World::Type::Raycast::k_BoxPadding;
    using Views::Raycast::Type::Constant::k_PerceptionRange;
    using Views::Raycast::Type::Constant::k_PerceptionRayCount;

    // Agents of the 16 agent metrics.
    constexpr std::size_t k_Agents{ 16 };
    // Rays of one agent: the aim ray and the perception rays.
    constexpr std::size_t k_RaysPerAgent{ 1 + k_PerceptionRayCount };
    // A tick of decision costs this many ticks of the pipeline (D273): the report divides the ms per tick by it.
    constexpr double k_DecisionEvery{ 4.0 };
    // A sample of the past positions of Self is kept every this many records.
    constexpr std::size_t k_HistoryEvery{ 60 };

    // The 16 agent metrics (M4, M5) run every K records, M0 and M1 run at every record. K = 30 on ctf_d, to stay under 150 s.
    constexpr std::size_t k_EveryRecords{ 30 };

    // The shared scene is armed and checked against RaycastStore every this many records: a decision every 4 ticks (D273).
    constexpr std::size_t k_SceneEvery{ 4 };

    // Differences between Self and the published rays that the report describes in full.
    constexpr std::size_t k_MaxDifferenceLines{ 6 };

    // Where and where to one agent looks from.
    struct Origin
    {
        Vec3 Camera{};
        Vec3 Forward{};
        std::uint32_t PlayerHandle{ k_NoHandle };
        std::uint32_t BipedHandle{ k_NoHandle };
        // A past position of Self: the perception rays start at the camera.
        bool IsPast{ false };
    };

    // What one agent needs before it casts: the same values RaycastService::Update resolves.
    struct Agent
    {
        Origin Source{};
        float AimRange{};
        SelfExclusion Exclusion{};
        Vec3 RayPosition{};
        std::array<Vec3, k_PerceptionRayCount> Directions{};
    };

    struct Ray
    {
        Vec3 Origin{};
        Vec3 Direction{};
        float Range{};
    };

    auto RaysOf(const Agent& agent) -> std::array<Ray, k_RaysPerAgent>
    {
        std::array<Ray, k_RaysPerAgent> rays{};
        rays[0] = Ray{ agent.Source.Camera, agent.Source.Forward, agent.AimRange };
        for (std::size_t index{ 0 }; index < k_PerceptionRayCount; ++index)
        {
            rays[1 + index] = Ray{ agent.RayPosition, agent.Directions[index], k_PerceptionRange };
        }

        return rays;
    }

    // The same values RaycastService::Update resolves for an agent.
    auto PrepareAgent(const Chain& chain, const Origin& source) -> Agent
    {
        Agent agent{};
        agent.Source = source;
        agent.AimRange = ResolveAimRange(chain.m_Tables.m_PlayerStore, chain.m_Tables.m_ObjectStore,
            chain.m_Resolved.m_DefinitionsStore, source.PlayerHandle);
        agent.Exclusion = BuildSelfExclusion(chain.m_Environment.m_PlayersStore, chain.m_Environment.m_EntitiesStore,
            chain.m_Tables.m_ObjectStore, source.PlayerHandle, source.BipedHandle);

        const PerceptionOrigin perceptionOrigin{ source.IsPast ? PerceptionOrigin{} :
            ResolvePerceptionOrigin(chain.m_Environment.m_EntitiesStore, chain.m_Tables.m_ObjectStore, source.BipedHandle) };
        agent.RayPosition = perceptionOrigin.Valid ? perceptionOrigin.Position : source.Camera;
        const Vec3 forward{ perceptionOrigin.Valid ? perceptionOrigin.Forward : source.Forward };
        agent.Directions = PerceptionDirections(forward);
        return agent;
    }

    auto SameHit(const RaycastHit& a, const RaycastHit& b) -> bool
    {
        return a.Hit == b.Hit && a.Kind == b.Kind && a.Distance == b.Distance && a.ObjectHandle == b.ObjectHandle;
    }

    // Every field of two hits, the point and the region included.
    auto SameFullHit(const RaycastHit& a, const RaycastHit& b) -> bool
    {
        return SameHit(a, b) && a.RegionIndex == b.RegionIndex && a.SurfaceFlags == b.SurfaceFlags && a.Material == b.Material
            && a.Point.X == b.Point.X && a.Point.Y == b.Point.Y && a.Point.Z == b.Point.Z;
    }

    auto DescribeHit(const RaycastHit& hit) -> std::string
    {
        return std::format("hit={} kind={} d={:.9g} handle={:08X} region={} flags={} material={}", hit.Hit, static_cast<int>(hit.Kind),
            hit.Distance, hit.ObjectHandle, hit.RegionIndex, hit.SurfaceFlags, hit.Material);
    }

    auto TimeLine(std::string_view label, const Series& series) -> std::string
    {
        const Stats s{ series.Summary() };
        return std::format("{:<34} n={:<6} mean={:>10.1f} p50={:>10.1f} p99={:>10.1f} max={:>10.1f} us   mean/N4={:.4f} ms",
            label, s.N, s.Mean, s.P50, s.P99, s.Max, s.Mean / k_NanosPerMicro / k_DecisionEvery);
    }

    auto CountLine(std::string_view label, const Series& series) -> std::string
    {
        const Stats s{ series.Summary() };
        return std::format("{:<34} n={:<6} mean={:>10.2f} p99={:>10.1f} max={:>10.1f}", label, s.N, s.Mean, s.P99, s.Max);
    }

    auto RayLine(std::string_view label, const Series& series) -> std::string
    {
        const Stats s{ series.Summary() };
        return std::format("{:<34} n={:<6} mean={:>10.3f} p50={:>10.3f} p99={:>10.3f} max={:>10.1f} us per ray",
            label, s.N, s.Mean, s.P50, s.P99, s.Max);
    }

    // Gathers the numbers of the bench from the records of a film. It only reads the chain and times calls.
    class RaycastBench
    {
    public:
        // Called after each record of the pass.
        // note: The pass owns the chain and runs the observer on its own thread, so the bench may call the services that are not
        // const (CollectPartsFor, Update). That is what the production code does on the same thread.
        auto Observe(const Chain& constChain, std::size_t record) -> void
        {
            Chain& chain{ const_cast<Chain&>(constChain) };
            const auto selfPtr{ chain.m_Egocentric.m_SelfStore.Acquire() };
            if (!selfPtr || !selfPtr->IsAlive) return;

            const Origin self{ selfPtr->Position, selfPtr->Forward, selfPtr->Handle, selfPtr->BipedHandle, false };
            if (record % k_HistoryEvery == 1) m_History.push_back(Origin{ self.Camera, self.Forward, self.PlayerHandle, self.BipedHandle, true });

            if (m_Measured == 0)
            {
                for (const auto& [tagName, sbsp] : chain.m_Resolved.m_DefinitionsStore.m_Sbsp.All())
                {
                    ++m_Sbsps;
                    m_StaticTriangles += sbsp.CollisionGeometry.size();
                }
            }

            ++m_Measured;
            const RaycastHit down{ CastStatic(chain.m_Resolved.m_Raycaster, self.Camera, Vec3{ 0.0f, 0.0f, -1.0f }, 150.0f) };
            m_DownHits += down.Hit ? 1 : 0;
            this->MeasureUpdate(chain);
            this->MeasureOneAgent(chain, self);
            if (record % k_SceneEvery == 0) this->MeasureSelfScene(chain, self);
            if (record % k_EveryRecords == 0) this->MeasureSixteenAgents(chain, self);
        }

        // return: The lines of the shared scene part: M5 and the checks of the reuse and of Self.
        auto SceneLines() const -> std::vector<std::string>
        {
            std::vector<std::string> lines{};
            lines.push_back(std::format("M5 16 agents, shared two level scene, every {} records, us per tick", k_EveryRecords));
            lines.push_back(TimeLine("M5 total (gather with reuse)", m_M5Total));
            lines.push_back(TimeLine("M5   exclusion + aim + origin", m_M5Prepare));
            lines.push_back(TimeLine("M5   gather parts (reuse)", m_GatherReuse));
            lines.push_back(TimeLine("M5   gather parts (all objects)", m_Gather));
            lines.push_back(CountLine("M5   objects that kept their parts", m_Reused));
            lines.push_back(CountLine("M5   objects collected (CollectPartsFor)", m_Collected));
            lines.push_back(TimeLine("M5   key stage: Acquire of the table, once per object", m_StageAcquire));
            lines.push_back(TimeLine("M5   key stage: find by handle", m_StageFind));
            lines.push_back(TimeLine("M5   key stage: BoneMatricesStore Get", m_StageBones));
            lines.push_back(TimeLine("M5   key stage: DamageSectionsStore Get", m_StageDamage));
            lines.push_back(TimeLine("M5   key stage: any valid matrix", m_StageValid));
            lines.push_back(TimeLine("M5   key stage: copy bones and damage to the key", m_StageCopy));
            lines.push_back(TimeLine("M5   key stage: key == previous key", m_StageCompare));
            lines.push_back(TimeLine("M5   key stage: whole key of every object, with previous", m_StageKey));
            lines.push_back(TimeLine("M5   arm MOBILE tree", m_Mobile));
            lines.push_back(TimeLine("M5   rays (240)", m_M5Rays));
            lines.push_back(RayLine("M5   ray (mean of the 240)", m_M5Ray));
            lines.push_back(CountLine("M5 nodes visited per ray", m_SceneNodesRay));
            lines.push_back(CountLine("M5 triangles tested per ray", m_SceneTrianglesRay));
            lines.push_back(CountLine("M5 objects in the scene", m_SceneObjectsSeries));
            lines.push_back(CountLine("M5 objects at rest", m_RestObjects));
            lines.push_back(CountLine("M5 leaves of the FIXED tree", m_FixedLeaves));
            lines.push_back(CountLine("M5 leaves of the MOBILE tree", m_MobileLeaves));
            lines.push_back(CountLine("M5 handles filtered out of FIXED", m_Filtered));
            lines.push_back(CountLine("M5 objects at rest waiting for FIXED", m_Pending));
            lines.push_back(std::format("M5 FIXED tree rebuilt {} times in the {} armings of the film (a decision every {} records, and the K records): "
                "{} with objects waiting only, {} with filtered handles only, {} with both (the first build counts as waiting).",
                m_Scene.FixedBuilds, m_Armings, k_SceneEvery, m_RebuildWaiting, m_RebuildFiltered, m_RebuildBoth));
            lines.push_back(TimeLine("M5   each FIXED rebuild", m_FixedArm));
            lines.push_back(std::format("M5 mesh BVHs: {} built in total ({} meshes in the scene), {:.1f} us in total, first use of each.",
                m_MeshesBuilt, m_Scene.Meshes.size(), m_MeshBvhUs));
            lines.push_back(TimeLine("M5   each mesh BVH call that built", m_MeshBvhCall));
            lines.push_back(std::format("M5 exactness check against a reference scene with every object gathered without reuse and all in MOBILE (same tick, own scene): {} rays compared, {} differences.",
                m_ReuseCompared, m_ReuseDifferences));
            lines.push_back(std::format("M1 check of the scene for Self against RaycastStore: {} rays compared, {} differences.",
                m_SelfCompared, m_SelfDifferences));
            for (const std::string& line : m_SceneMessages) lines.push_back("  " + line);
            for (const std::string& line : this->MobileGroupLines()) lines.push_back(line);
            return lines;
        }

        // return: The lines of the report, without the header.
        auto Lines() const -> std::vector<std::string>
        {
            std::vector<std::string> lines{};
            const auto time{ [&](std::string_view label, const Series& series) { lines.push_back(TimeLine(label, series)); } };
            const auto count{ [&](std::string_view label, const Series& series) { lines.push_back(CountLine(label, series)); } };
            const auto ray{ [&](std::string_view label, const Series& series) { lines.push_back(RayLine(label, series)); } };

            lines.push_back(std::format("Static world: {} sbsp, {} triangles. A ray straight down from the camera of Self hit it in {} of {} records.",
                m_Sbsps, m_StaticTriangles, m_DownHits, m_Measured));
            lines.push_back("");
            lines.push_back("M0 Update real (1 agent: Self), every record, us per tick");
            time("M0 Update", m_Update);
            lines.push_back("");
            lines.push_back("M1 pieces of Update that do not depend on the scene, every record, us per tick");
            time("M1 BuildSelfExclusion", m_Exclusion);
            time("M1 aim range + origin + directions", m_Setup);
            time("M1 15 rays, static part (sum)", m_StaticTick);
            ray("M1 ray, static part", m_StaticRay);
            lines.push_back("");
            lines.push_back("M4 static world only: 16 x 15 rays");
            time("M4 total", m_M4Total);
            ray("M4 ray", m_M4Ray);

            lines.push_back("");
            for (const std::string& line : this->SceneLines()) lines.push_back(line);

            lines.push_back("");
            lines.push_back("Heaviest, 1 agent (ms per tick, mean):");
            this->AppendHeaviest(lines, { { "BuildSelfExclusion", &m_Exclusion }, { "static rays (15)", &m_StaticTick }, { "Update (M0)", &m_Update } });
            lines.push_back("Heaviest, 16 agents (ms per tick, mean):");
            this->AppendHeaviest(lines, { { "M4 static rays", &m_M4Total }, { "M5 total", &m_M5Total }, { "M5 gather (reuse)", &m_GatherReuse }, { "M5 rays", &m_M5Rays } });
            return lines;
        }

    private:
        std::vector<Origin> m_History{};
        std::size_t m_Measured{};
        std::size_t m_Sbsps{};
        std::size_t m_StaticTriangles{};
        std::size_t m_DownHits{};
        double m_Sink{};

        Series m_Update{};
        Series m_Exclusion{}, m_Setup{}, m_StaticTick{};
        Series m_StaticRay{};
        Series m_M4Total{}, m_M4Ray{};
        Series m_M5Total{}, m_M5Prepare{}, m_M5Rays{}, m_M5Ray{};
        Series m_Gather{}, m_GatherReuse{}, m_Reused{}, m_Collected{}, m_Mobile{}, m_FixedArm{}, m_MeshBvhCall{};
        Series m_SceneNodesRay{}, m_SceneTrianglesRay{}, m_SceneObjectsSeries{}, m_FixedLeaves{}, m_MobileLeaves{};
        Series m_RestObjects{}, m_Filtered{}, m_Pending{};
        Series m_StageAcquire{}, m_StageFind{}, m_StageBones{}, m_StageDamage{}, m_StageValid{}, m_StageCopy{}, m_StageCompare{}, m_StageKey{};

        // The shared scene lives for the whole film, as it would in the pipeline, with the parts of the objects it reuses.
        RaycastScene m_Scene{};
        ScenePartsCache m_Reuse{};

        // The reference scene: every object gathered without reuse, all in the MOBILE tree and the FIXED one empty. Only to check the exactness.
        RaycastScene m_FullScene{};
        std::size_t m_ReuseCompared{};
        std::size_t m_ReuseDifferences{};
        std::size_t m_Armings{};
        std::size_t m_RebuildWaiting{};
        std::size_t m_RebuildFiltered{};
        std::size_t m_RebuildBoth{};
        std::size_t m_MeshesBuilt{};
        double m_MeshBvhUs{};
        std::size_t m_SelfCompared{};
        std::size_t m_SelfDifferences{};
        std::size_t m_DifferenceLines{};
        std::vector<std::string> m_SceneMessages{};

        // What the MOBILE tree holds, by kind of object: objects and leaves per arming, and the part of them at rest but waiting.
        struct MobileGroup
        {
            double Objects{};
            double Leaves{};
            double WaitingObjects{};
            double WaitingLeaves{};
        };

        std::map<std::string, MobileGroup> m_MobileGroups{};
        std::size_t m_MobileArmings{};

        // Adds the objects the FIXED tree does not cover to the groups, named by four character code, physics and whether it has a parent.
        auto CountMobileGroups(Chain& chain, const SceneObjects& objects) -> void
        {
            const auto table{ chain.m_Tables.m_ObjectStore.Acquire() };
            if (!table) return;

            ++m_MobileArmings;
            for (const SceneObject& object : objects)
            {
                if (object.IsCovered) continue;

                std::string name{ "?" };
                const auto row{ table->find(object.Handle) };
                if (row != table->end())
                {
                    const auto& alive{ row->second };
                    const int physics{ alive.Placement ? static_cast<int>(alive.Placement->Physics) : -1 };
                    name = std::format("{} physics {} {}", alive.FourCC, physics, alive.ParentHandle != k_NoHandle && alive.ParentHandle != 0 ? "parented" : "free");
                }

                MobileGroup& group{ m_MobileGroups[name] };
                group.Objects += 1.0;
                group.Leaves += static_cast<double>(object.Parts.size());
                if (!object.IsFixed) continue;

                group.WaitingObjects += 1.0;
                group.WaitingLeaves += static_cast<double>(object.Parts.size());
            }
        }

        auto MobileGroupLines() const -> std::vector<std::string>
        {
            std::vector<std::string> lines{};
            lines.push_back(std::format("MOBILE by group, mean per arming over {} armings: objects / leaves / objects at rest waiting for FIXED / their leaves",
                m_MobileArmings));
            std::vector<std::pair<std::string, MobileGroup>> rows{ m_MobileGroups.begin(), m_MobileGroups.end() };
            std::ranges::sort(rows, [](const auto& a, const auto& b) { return a.second.Leaves > b.second.Leaves; });
            const double armings{ m_MobileArmings > 0 ? static_cast<double>(m_MobileArmings) : 1.0 };
            for (const auto& [name, group] : rows)
            {
                lines.push_back(std::format("  {:<40} {:8.1f} {:8.1f} {:8.1f} {:8.1f}", name, group.Objects / armings, group.Leaves / armings,
                    group.WaitingObjects / armings, group.WaitingLeaves / armings));
            }

            return lines;
        }

        static auto AppendHeaviest(std::vector<std::string>& lines, std::vector<std::pair<std::string_view, const Series*>> rows) -> void
        {
            std::ranges::sort(rows, [](const auto& a, const auto& b) { return a.second->Summary().Mean > b.second->Summary().Mean; });
            for (std::size_t index{ 0 }; index < (std::min)(rows.size(), std::size_t{ 5 }); ++index)
            {
                lines.push_back(std::format("  {:<22} {:>9.4f} ms per tick, {:.4f} ms averaged over N = 4",
                    rows[index].first, rows[index].second->Summary().Mean / k_NanosPerMicro, rows[index].second->Summary().Mean / k_NanosPerMicro / k_DecisionEvery));
            }
        }

        // M0: RaycastService::Update as it is.
        auto MeasureUpdate(Chain& chain) -> void
        {
            const auto start{ steady_clock::now() };
            chain.m_Egocentric.m_RaycastService.Update();
            m_Update.Add(Micros(start, steady_clock::now()));
        }

        // M1: the pieces of Update that do not depend on the scene: the exclusion, the agent values and the static rays.
        auto MeasureOneAgent(Chain& chain, const Origin& self) -> void
        {
            const auto& raycaster{ chain.m_Resolved.m_Raycaster };

            auto start{ steady_clock::now() };
            const SelfExclusion exclusion{ BuildSelfExclusion(chain.m_Environment.m_PlayersStore, chain.m_Environment.m_EntitiesStore,
                chain.m_Tables.m_ObjectStore, self.PlayerHandle, self.BipedHandle) };
            m_Exclusion.Add(Micros(start, steady_clock::now()));
            m_Sink += static_cast<double>(exclusion.size());

            start = steady_clock::now();
            const Agent agent{ PrepareAgent(chain, self) };
            m_Setup.Add(Micros(start, steady_clock::now()));

            double staticUs{};
            for (const Ray& ray : RaysOf(agent))
            {
                start = steady_clock::now();
                m_Sink += CastStatic(raycaster, ray.Origin, ray.Direction, ray.Range).Distance;
                const double rayUs{ Micros(start, steady_clock::now()) };
                staticUs += rayUs;
                m_StaticRay.Add(rayUs);
            }
            m_StaticTick.Add(staticUs);
        }

        // Every object of the tick with the parts collected again, and the vectors its objects view.
        struct AllObjects
        {
            std::deque<std::vector<Perception::Collidable::Type::CollidablePart>> Storage{};
            SceneObjects Objects{};
        };

        // Collects the parts of every object of the tick with no reuse and no rest, as RS.0 did.
        static auto GatherAll(Chain& chain) -> AllObjects
        {
            AllObjects all{};
            const auto table{ chain.m_Tables.m_ObjectStore.Acquire() };
            if (!table) return all;

            for (const auto& [handle, object] : *table)
            {
                auto collidable{ chain.m_Environment.m_CollidableService.CollectPartsFor(handle) };
                if (!collidable || collidable->Parts.empty()) continue;

                all.Storage.push_back(std::move(collidable->Parts));
                all.Objects.push_back(SceneObject{ handle, false, all.Storage.back(), false });
            }

            std::ranges::sort(all.Objects, [](const SceneObject& a, const SceneObject& b) { return a.Handle < b.Handle; });
            return all;
        }

        // Gathers every object of the tick with reuse and rest and arms the scene, the way a decision tick would.
        // return: The microseconds the gather and the mobile tree took.
        auto ArmScene(Chain& chain) -> std::pair<double, double>
        {
            ++m_Armings;

            SceneGatherCounters gathered{};
            auto start{ steady_clock::now() };
            SceneObjects objects{ GatherSceneObjects(m_Reuse, chain.m_Environment.m_CollidableService, chain.m_Tables.m_ObjectStore, &gathered) };
            const double gatherUs{ Micros(start, steady_clock::now()) };
            m_Reused.Add(static_cast<double>(gathered.Reused));
            m_Collected.Add(static_cast<double>(gathered.Collected));

            start = steady_clock::now();
            const std::size_t built{ PrepareMeshBvhs(m_Scene, objects, false) };
            const double meshUs{ Micros(start, steady_clock::now()) };
            if (built > 0)
            {
                m_MeshesBuilt += built;
                m_MeshBvhUs += meshUs;
                m_MeshBvhCall.Add(meshUs);
            }

            start = steady_clock::now();
            const bool rebuilt{ ArmFixedScene(m_Scene, objects) };
            if (rebuilt)
            {
                m_FixedArm.Add(Micros(start, steady_clock::now()));
                if (m_Scene.FixedPending > 0 && m_Scene.FixedFiltered > 0) ++m_RebuildBoth;
                else if (m_Scene.FixedPending > 0) ++m_RebuildWaiting;
                else ++m_RebuildFiltered;
            }

            start = steady_clock::now();
            ArmMobileScene(m_Scene, objects);
            const double mobileUs{ Micros(start, steady_clock::now()) };

            this->CountMobileGroups(chain, objects);
            m_SceneObjectsSeries.Add(static_cast<double>(objects.size()));
            m_RestObjects.Add(static_cast<double>(std::ranges::count_if(objects, [](const SceneObject& object) { return object.IsFixed; })));
            m_Filtered.Add(static_cast<double>(m_Scene.FixedFiltered));
            m_Pending.Add(static_cast<double>(m_Scene.FixedPending));
            m_FixedLeaves.Add(static_cast<double>(m_Scene.Fixed.Pieces.size()));
            m_MobileLeaves.Add(static_cast<double>(m_Scene.Mobile.Pieces.size()));
            return { gatherUs, mobileUs };
        }

        // M1 with the scene: the rays of Self cast through a scene of the bench against what RaycastService published.
        auto MeasureSelfScene(Chain& chain, const Origin& self) -> void
        {
            const auto published{ chain.m_Egocentric.m_RaycastStore.Acquire() };
            if (!published) return;

            this->ArmScene(chain);
            const Agent agent{ PrepareAgent(chain, self) };
            const std::array<Ray, k_RaysPerAgent> rays{ RaysOf(agent) };
            for (std::size_t index{ 0 }; index < rays.size(); ++index)
            {
                const Ray& ray{ rays[index] };
                const RaycastHit hit{ CastSceneRay(chain.m_Resolved.m_Raycaster, m_Scene, agent.Exclusion, ray.Origin, ray.Direction, ray.Range, nullptr) };
                const RaycastHit& expected{ index == 0 ? published->AimHit : published->PerceptionHits[index - 1] };

                ++m_SelfCompared;
                if (SameFullHit(hit, expected)) continue;

                ++m_SelfDifferences;
                if (m_DifferenceLines >= k_MaxDifferenceLines) continue;

                ++m_DifferenceLines;
                m_SceneMessages.push_back(std::format("Self ray {}: RaycastStore [{}] scene [{}]", index, DescribeHit(expected), DescribeHit(hit)));
            }
        }

        // The origins of the 16 agents: the live players first, then past positions of Self.
        auto BuildOrigins(const Chain& chain, const Origin& self) const -> std::vector<Origin>
        {
            std::vector<Origin> origins{};
            const auto players{ chain.m_Tables.m_PlayerStore.Acquire() };
            if (players)
            {
                std::vector<const std::remove_cvref_t<decltype(players->begin()->second)>*> alive{};
                for (const auto& [handle, player] : *players)
                {
                    if (player.AliveBipedHandle != k_NoHandle && player.CurrentBipedHandle != k_NoHandle) alive.push_back(&player);
                }
                std::ranges::sort(alive, [](const auto* a, const auto* b) { return a->Handle < b->Handle; });

                for (const auto* player : alive)
                {
                    if (origins.size() >= k_Agents) break;

                    Vec3 forward{}, right{}, up{};
                    Common::Math::System::BuildFrame(player->CameraForward, forward, right, up);
                    origins.push_back(Origin{ player->CameraPosition, forward, player->Handle, player->CurrentBipedHandle, false });
                }
            }

            // Newest first, around again if the film is young.
            for (std::size_t index{ 0 }; origins.size() < k_Agents; ++index)
            {
                if (m_History.empty()) origins.push_back(Origin{ self.Camera, self.Forward, self.PlayerHandle, self.BipedHandle, true });
                else origins.push_back(m_History[m_History.size() - 1 - (index % m_History.size())]);
            }

            return origins;
        }

        // M4 for the origins of this record, then M5.
        auto MeasureSixteenAgents(Chain& chain, const Origin& self) -> void
        {
            const auto& raycaster{ chain.m_Resolved.m_Raycaster };
            const std::vector<Origin> origins{ this->BuildOrigins(chain, self) };

            // M4: the rays of every agent against the static world only.
            std::vector<Agent> agents{};
            agents.reserve(origins.size());
            for (const Origin& origin : origins) agents.push_back(PrepareAgent(chain, origin));

            const auto start4{ steady_clock::now() };
            for (const Agent& agent : agents)
            {
                for (const Ray& ray : RaysOf(agent)) m_Sink += CastStatic(raycaster, ray.Origin, ray.Direction, ray.Range).Distance;
            }
            const double total4{ Micros(start4, steady_clock::now()) };
            m_M4Total.Add(total4);
            m_M4Ray.Add(total4 / static_cast<double>(agents.size() * k_RaysPerAgent));

            this->MeasureScene(chain, origins);
        }

        // The cost of each stage of the key of every object of the tick, one loop per stage, outside the clocks of the gather.
        template <typename Table>
        auto MeasureKeyStages(Chain& chain, const Table& table) -> void
        {
            auto& collidable{ chain.m_Environment.m_CollidableService };
            const auto& boneStore{ chain.m_Tables.m_BoneMatricesStore };
            const auto& damageStore{ chain.m_Tables.m_DamageSectionsStore };
            Perception::Collidable::Type::CollidablePartsKey key{};
            std::size_t sink{ 0 };

            auto start{ steady_clock::now() };
            for (std::size_t index{ 0 }; index < table.size(); ++index) sink += chain.m_Tables.m_ObjectStore.Acquire().use_count();
            m_StageAcquire.Add(Micros(start, steady_clock::now()));

            start = steady_clock::now();
            for (const auto& [handle, object] : table) sink += table.find(handle) != table.end() ? 1 : 0;
            m_StageFind.Add(Micros(start, steady_clock::now()));

            start = steady_clock::now();
            for (const auto& [handle, object] : table) sink += boneStore.Get(handle) != nullptr ? 1 : 0;
            m_StageBones.Add(Micros(start, steady_clock::now()));

            start = steady_clock::now();
            for (const auto& [handle, object] : table) sink += damageStore.Get(handle) != nullptr ? 1 : 0;
            m_StageDamage.Add(Micros(start, steady_clock::now()));

            start = steady_clock::now();
            for (const auto& [handle, object] : table)
            {
                const auto* bones{ boneStore.Get(handle) };
                if (bones) sink += std::ranges::any_of(bones->Matrices, Blam::Runtime::Object::Type::BoneMatrix::IsBoneMatrixValid) ? 1 : 0;
            }
            m_StageValid.Add(Micros(start, steady_clock::now()));

            start = steady_clock::now();
            for (const auto& [handle, object] : table)
            {
                const auto* bones{ boneStore.Get(handle) };
                const auto* damage{ damageStore.Get(handle) };
                if (bones) key.Bones.assign(bones->Matrices.begin(), bones->Matrices.end());
                if (damage) key.DamageRegions.assign(damage->Regions.Bytes.begin(), damage->Regions.Bytes.end());
                sink += key.Bones.size() + key.DamageRegions.size();
            }
            m_StageCopy.Add(Micros(start, steady_clock::now()));

            // The whole key as the gather makes it: updated in place in the kept key, which does not change in the same tick (so same stays 0).
            std::size_t same{ 0 };
            start = steady_clock::now();
            for (const auto& [handle, object] : table)
            {
                const auto entry{ m_Reuse.find(handle) };
                if (entry != m_Reuse.end()) same += collidable.UpdatePartsKey(object, entry->second.Key, true) ? 1 : 0;
            }
            m_StageKey.Add(Micros(start, steady_clock::now()));

            std::vector<Perception::Collidable::Type::CollidablePartsKey> keys{};
            keys.reserve(table.size());
            for (const auto& [handle, object] : table)
            {
                const auto entry{ m_Reuse.find(handle) };
                if (entry != m_Reuse.end()) keys.push_back(entry->second.Key);
            }
            start = steady_clock::now();
            std::size_t index{ 0 };
            for (const auto& [handle, object] : table)
            {
                const auto entry{ m_Reuse.find(handle) };
                if (entry == m_Reuse.end()) continue;

                same += keys[index] == entry->second.Key ? 1 : 0;
                ++index;
            }
            m_StageCompare.Add(Micros(start, steady_clock::now()));
            m_Sink += static_cast<double>(sink + same);
        }

        // M5: the 16 agents over the shared two level scene with the objects at rest, compared ray by ray with the reference scene.
        auto MeasureScene(Chain& chain, const std::vector<Origin>& origins) -> void
        {
            const auto& raycaster{ chain.m_Resolved.m_Raycaster };

            const auto total5{ steady_clock::now() };
            std::vector<Agent> agents{};
            agents.reserve(origins.size());
            for (const Origin& origin : origins) agents.push_back(PrepareAgent(chain, origin));
            const auto prepared5{ steady_clock::now() };

            const auto [gatherUs, mobileUs] { this->ArmScene(chain) };
            const auto armed5{ steady_clock::now() };

            std::vector<RaycastHit> m5Hits{};
            m5Hits.reserve(agents.size() * k_RaysPerAgent);
            for (const Agent& agent : agents)
            {
                for (const Ray& ray : RaysOf(agent))
                {
                    m5Hits.push_back(CastSceneRay(raycaster, m_Scene, agent.Exclusion, ray.Origin, ray.Direction, ray.Range, nullptr));
                }
            }
            const auto end5{ steady_clock::now() };

            m_M5Total.Add(Micros(total5, end5));
            m_M5Prepare.Add(Micros(total5, prepared5));
            m_GatherReuse.Add(gatherUs);
            m_Mobile.Add(mobileUs);
            m_M5Rays.Add(Micros(armed5, end5));
            m_M5Ray.Add(Micros(armed5, end5) / static_cast<double>(m5Hits.size()));

            // The same tick gathered without reuse and all in MOBILE: its own timing, and its hits must be the ones of the scene with rest.
            const auto startAll{ steady_clock::now() };
            const AllObjects all{ GatherAll(chain) };
            m_Gather.Add(Micros(startAll, steady_clock::now()));
            const auto table{ chain.m_Tables.m_ObjectStore.Acquire() };
            if (table) this->MeasureKeyStages(chain, *table);
            PrepareMeshBvhs(m_FullScene, all.Objects, false);
            ArmMobileScene(m_FullScene, all.Objects);

            // Counted pass and comparison, outside the clocks.
            std::size_t flat{ 0 };
            for (std::size_t agentIndex{ 0 }; agentIndex < agents.size(); ++agentIndex)
            {
                const Agent& agent{ agents[agentIndex] };
                const std::array<Ray, k_RaysPerAgent> rays{ RaysOf(agent) };
                for (std::size_t index{ 0 }; index < rays.size(); ++index, ++flat)
                {
                    SceneCounters counters{};
                    const Ray& ray{ rays[index] };
                    m_Sink += CastSceneRay(raycaster, m_Scene, agent.Exclusion, ray.Origin, ray.Direction, ray.Range, &counters).Distance;
                    m_SceneNodesRay.Add(static_cast<double>(counters.Nodes));
                    m_SceneTrianglesRay.Add(static_cast<double>(counters.Triangles));

                    ++m_ReuseCompared;
                    const RaycastHit full{ CastSceneRay(raycaster, m_FullScene, agent.Exclusion, ray.Origin, ray.Direction, ray.Range, nullptr) };
                    if (!SameFullHit(full, m5Hits[flat])) ++m_ReuseDifferences;
                }
            }
        }
    };

    // RS.1d part A: what the mobile tree costs by stage, how many of its leaves change pose from one record to the next and how many
    // records in a row it keeps exactly the same pieces. Every stage runs in its own loop over the objects of the mobile tree.
    class MobileStages
    {
    public:
        // Called with the scene after ArmFixedScene and before ArmMobileScene, so scene.Mobile is still the one of the previous record.
        auto Observe(RaycastScene& scene, const SceneObjects& objects) -> void
        {
            auto start{ steady_clock::now() };
            const std::size_t built{ PrepareMeshBvhs(scene, objects, false) };
            const double meshUs{ Micros(start, steady_clock::now()) };

            start = steady_clock::now();
            std::vector<std::vector<CachedPart>> prepared{};
            prepared.reserve(objects.size());
            for (const SceneObject& object : objects)
            {
                if (object.IsCovered) continue;
                std::vector<CachedPart>& parts{ prepared.emplace_back() };
                for (const auto& part : object.Parts)
                {
                    CachedPart cached{};
                    if (PreparePart(part, cached)) parts.push_back(cached);
                }
            }
            const double prepareUs{ Micros(start, steady_clock::now()) };

            start = steady_clock::now();
            std::vector<ScenePiece> pieces{};
            std::size_t group{ 0 };
            for (const SceneObject& object : objects)
            {
                if (object.IsCovered) continue;

                const std::vector<CachedPart>& parts{ prepared[group++] };
                for (std::size_t index{ 0 }; index < parts.size(); ++index)
                {
                    ScenePiece piece{};
                    piece.Part = parts[index];
                    piece.Bvh = &scene.Meshes.at(parts[index].Source);
                    piece.Handle = object.Handle;
                    piece.PartIndex = static_cast<std::uint32_t>(index);
                    pieces.push_back(piece);
                }
            }
            const double piecesUs{ Micros(start, steady_clock::now()) };

            start = steady_clock::now();
            std::vector<BvhBounds> bounds{};
            bounds.reserve(pieces.size());
            for (const ScenePiece& piece : pieces)
            {
                const Vec3& c{ piece.Part.Center };
                const float r{ piece.Part.Radius + k_BoxPadding };
                bounds.push_back(BvhBounds{ { c.X - r, c.Y - r, c.Z - r }, { c.X + r, c.Y + r, c.Z + r } });
            }
            const double boxesUs{ Micros(start, steady_clock::now()) };

            start = steady_clock::now();
            std::vector<BvhNode> nodes{};
            std::vector<std::uint32_t> order{};
            Blam::MapModel::World::System::BuildBoundsBvh(bounds, nodes, order);
            const double bvhUs{ Micros(start, steady_clock::now()) };
            m_Sink += static_cast<double>(nodes.size());

            // Against the pieces armed in the previous record.
            const std::vector<ScenePiece>& old{ scene.Mobile.Pieces };
            bool same{ old.size() == pieces.size() };
            std::size_t changed{ 0 };
            for (std::size_t index{ 0 }; same && index < pieces.size(); ++index)
            {
                same = old[index].Handle == pieces[index].Handle && old[index].Bvh == pieces[index].Bvh && old[index].PartIndex == pieces[index].PartIndex;
                if (!same) break;
                if (old[index].Part.ToLocal != pieces[index].Part.ToLocal) ++changed;
            }

            ++m_Records;
            if (built > 0) return;

            m_Mesh.Add(meshUs);
            m_Prepare.Add(prepareUs);
            m_Pieces.Add(piecesUs);
            m_Boxes.Add(boxesUs);
            m_Bvh.Add(bvhUs);
            m_Leaves.Add(static_cast<double>(pieces.size()));
            if (same)
            {
                ++m_Same;
                ++m_Streak;
                m_Changed.Add(static_cast<double>(changed));
                m_ChangedShare.Add(pieces.empty() ? 0.0 : 100.0 * static_cast<double>(changed) / static_cast<double>(pieces.size()));
                return;
            }

            if (m_Streak > 0) m_Streaks.Add(static_cast<double>(m_Streak));
            m_Streak = 0;
        }

        auto Lines() const -> std::vector<std::string>
        {
            std::vector<std::string> lines{};
            lines.push_back(std::format("RS.1d part A: mobile tree by stage, every record, us per arming ({} records, {} with exactly the pieces of the previous record).", m_Records, m_Same));
            lines.push_back(TimeLine("A PrepareMeshBvhs (all built)", m_Mesh));
            lines.push_back(TimeLine("A PrepareParts of every object", m_Prepare));
            lines.push_back(TimeLine("A pieces (push_back, meshes.at)", m_Pieces));
            lines.push_back(TimeLine("A boxes of the leaves", m_Boxes));
            lines.push_back(TimeLine("A BuildBoundsBvh", m_Bvh));
            lines.push_back(CountLine("A leaves of the mobile tree", m_Leaves));
            lines.push_back(CountLine("A leaves that changed pose (same pieces)", m_Changed));
            lines.push_back(CountLine("A share of leaves that changed pose, % (same pieces)", m_ChangedShare));
            lines.push_back(CountLine("A records in a row with the same pieces", m_Streaks));
            return lines;
        }

    private:
        Series m_Mesh{}, m_Prepare{}, m_Pieces{}, m_Boxes{}, m_Bvh{}, m_Leaves{}, m_Changed{}, m_ChangedShare{}, m_Streaks{};
        std::size_t m_Records{};
        std::size_t m_Same{};
        std::size_t m_Streak{};
        double m_Sink{};
    };

    // One rule of the sweep: when the fixed tree (D301) and the mobile tree (D302) are built again.
    struct SweepRule
    {
        FixedRebuildRule Fixed{};
        MobileRebuildRule Mobile{};
    };

    // Several rules for the rebuild of the fixed tree (D301) run side by side over the same film, each with its own scene and cache, the
    // way Self runs them: a gather at every record, the arming, and the 15 rays of Self. Only the scene part of Update is timed.
    class RuleSweep
    {
    public:
        explicit RuleSweep(const std::vector<SweepRule>& rules)
        {
            for (const SweepRule& rule : rules) m_Runs.push_back(Run{ .Rule = rule.Fixed, .Mobile = rule.Mobile });
        }

        // Called after each record of the pass.
        auto Observe(const Chain& constChain, std::size_t record) -> void
        {
            Chain& chain{ const_cast<Chain&>(constChain) };
            const auto selfPtr{ chain.m_Egocentric.m_SelfStore.Acquire() };
            if (!selfPtr || !selfPtr->IsAlive) return;

            const Origin self{ selfPtr->Position, selfPtr->Forward, selfPtr->Handle, selfPtr->BipedHandle, false };
            const Agent agent{ PrepareAgent(chain, self) };
            const std::array<Ray, k_RaysPerAgent> rays{ RaysOf(agent) };

            ++m_Records;
            std::vector<RaycastHit> reference{};
            for (std::size_t index{ 0 }; index < m_Runs.size(); ++index)
            {
                Run& run{ m_Runs[index] };
                std::vector<RaycastHit> hits{};
                hits.reserve(rays.size());

                const auto start{ steady_clock::now() };
                SceneObjects objects{ GatherSceneObjects(run.Reuse, chain.m_Environment.m_CollidableService, chain.m_Tables.m_ObjectStore, nullptr) };
                const auto gathered{ steady_clock::now() };
                const bool rebuilt{ ArmFixedScene(run.Scene, objects, run.Rule) };
                if (index == 0) m_Stages.Observe(run.Scene, objects);
                const std::size_t degradedBefore{ run.Scene.MobileDegraded };
                const auto armStart{ steady_clock::now() };
                ArmMobileScene(run.Scene, objects, run.Mobile);
                const auto armed{ steady_clock::now() };
                for (const Ray& ray : rays) hits.push_back(CastSceneRay(chain.m_Resolved.m_Raycaster, run.Scene, agent.Exclusion, ray.Origin, ray.Direction, ray.Range, nullptr));
                const auto end{ steady_clock::now() };

                run.Arm.Add(Micros(armStart, armed));
                run.Rays.Add(Micros(armed, end));
                if (run.Scene.MobileDegraded > degradedBefore) run.Ratio.Add(static_cast<double>(run.Scene.MobileAreaRatio));

                run.Total.Add(Micros(start, end));
                run.Gather.Add(Micros(start, gathered));
                if (rebuilt) run.Rebuild.Add(Micros(gathered, end));
                run.MobileLeaves.Add(static_cast<double>(run.Scene.Mobile.Pieces.size()));
                run.Filtered.Add(static_cast<double>(run.Scene.FixedFiltered));
                run.Pending.Add(static_cast<double>(run.Scene.FixedPending));

                if (index == 0)
                {
                    reference = std::move(hits);
                    continue;
                }

                for (std::size_t ray{ 0 }; ray < hits.size(); ++ray)
                {
                    ++m_Compared;
                    if (!SameFullHit(hits[ray], reference[ray])) ++m_Differences;
                }
            }

            m_Sink += static_cast<double>(record);
        }

        // return: One row per rule: the scene part of Update in us, the rebuilds and what waits.
        auto Lines() const -> std::vector<std::string>
        {
            std::vector<std::string> lines{};
            lines.push_back(std::format("{} records. Per record, in us: gather + arm + 15 rays of Self (total), the gather alone, and the whole record in which the fixed tree was built.", m_Records));
            lines.push_back("  spacing  N  T    builds  total mean     p99     max  | gather mean | build rec mean | MOBILE leaves | filtered | pending");
            for (const Run& run : m_Runs)
            {
                const Stats total{ run.Total.Summary() };
                const Stats gather{ run.Gather.Summary() };
                const Stats rebuild{ run.Rebuild.Summary() };
                lines.push_back(std::format("  {:>6} {:>3} {:>4} {:>6} {:>10.1f} {:>7.1f} {:>7.1f}  | {:>11.1f} | {:>14.1f} | {:>13.1f} | {:>8.2f} | {:>7.2f}",
                    run.Rule.Spacing, run.Rule.Pending, run.Rule.MaxGathers, run.Scene.FixedBuilds, total.Mean, total.P99, total.Max,
                    gather.Mean, rebuild.Mean, run.MobileLeaves.Summary().Mean, run.Filtered.Summary().Mean, run.Pending.Summary().Mean));
            }

            lines.push_back("  RS.1d mobile tree per rule: R = largest area growth before a build, builds (all / asked by the rule), refits, arm MOBILE us, 15 rays us, mean area ratio at the rule's builds");
            for (const Run& run : m_Runs)
            {
                const Stats arm{ run.Arm.Summary() };
                const Stats rays{ run.Rays.Summary() };
                lines.push_back(std::format("  R {:>6.2f} {:>6} {:>6} {:>7} | arm mean {:>7.1f} p99 {:>7.1f} | rays mean {:>7.1f} p99 {:>7.1f} | ratio at rebuild {:>6.2f}",
                    run.Mobile.MaxArea, run.Scene.MobileBuilds, run.Scene.MobileDegraded, run.Scene.MobileRefits, arm.Mean, arm.P99, rays.Mean, rays.P99,
                    run.Ratio.Summary().Mean));
            }

            lines.push_back(std::format("Rays of every rule against the first one: {} compared, {} differences.", m_Compared, m_Differences));
            for (const std::string& line : m_Stages.Lines()) lines.push_back(line);
            return lines;
        }

    private:
        struct Run
        {
            FixedRebuildRule Rule{};
            MobileRebuildRule Mobile{};
            RaycastScene Scene{};
            ScenePartsCache Reuse{};
            Series Total{}, Gather{}, Rebuild{}, MobileLeaves{}, Filtered{}, Pending{}, Arm{}, Rays{}, Ratio{};
        };

        std::vector<Run> m_Runs{};
        MobileStages m_Stages{};
        std::size_t m_Records{};
        std::size_t m_Compared{};
        std::size_t m_Differences{};
        double m_Sink{};
    };

    // The rules of the sweep: the lines "N T R" of build/bench/rs1c_sweep.txt, each with the spacing of the constants.
    // note: Without the file, the fixed rule of the constants and R in {1, 1.25, 1.5, 2}.
    auto SweepRules() -> std::vector<SweepRule>
    {
        const std::filesystem::path source{ std::source_location::current().file_name() };
        std::ifstream in{ source.parent_path().parent_path().parent_path().parent_path().parent_path() / "build" / "bench" / "rs1c_sweep.txt" };
        std::vector<SweepRule> rules{};
        std::size_t pending{};
        std::size_t maxGathers{};
        float maxArea{};
        while (in >> pending >> maxGathers >> maxArea)
        {
            // An area of 0 in the file stands for never building the mobile tree again while its pieces stay the same.
            rules.push_back(SweepRule{ FixedRebuildRule{ .Pending = pending, .MaxGathers = maxGathers },
                MobileRebuildRule{ .MaxArea = maxArea > 0.0f ? maxArea : std::numeric_limits<float>::infinity() } });
        }

        if (rules.empty())
        {
            for (const float areaRule : { 1.0f, 1.25f, 1.5f, 2.0f }) rules.push_back(SweepRule{ FixedRebuildRule{}, MobileRebuildRule{ .MaxArea = areaRule } });
        }

        return rules;
    }

    // Replays the film with the rules of SweepRules and writes build/bench/rs1c_sweep_<name>.txt.
    auto RunSweep(std::string_view film, std::string_view name) -> void
    {
        RuleSweep sweep{ SweepRules() };
        const FilmOutcome outcome{ RunFilmPass(film, false, [&](const Chain& chain, std::size_t record) { sweep.Observe(chain, record); }) };
        if (!outcome.Skip.empty()) GTEST_SKIP() << outcome.Skip;
        ASSERT_TRUE(outcome.Error.empty()) << outcome.Error;

        std::vector<std::string> report{};
        report.push_back(std::format("RS.1c rebuild rule sweep: film {}, pass {:.1f} s, {}.", film, outcome.Seconds, Tests::Support::Bench::Configuration()));
        for (const std::string& line : sweep.Lines()) report.push_back(line);
        ASSERT_FALSE(Tests::Support::Bench::WriteReport(std::format("rs1c_sweep_{}.txt", name), report).empty());
        EXPECT_GT(outcome.Records, 0u);
    }

    // Replays the film, measures, and writes the report to build/bench/raycast_<name>.txt of the repository.
    // param film: Folder of the film, relative to RecordingsFolder.
    // param name: Name of the film in the reports.
    auto RunBench(std::string_view film, std::string_view name) -> void
    {
        RaycastBench bench{};
        const FilmOutcome outcome{ RunFilmPass(film, false, [&](const Chain& chain, std::size_t record) { bench.Observe(chain, record); }) };
        if (!outcome.Skip.empty()) GTEST_SKIP() << outcome.Skip;
        ASSERT_TRUE(outcome.Error.empty()) << outcome.Error;

        const std::vector<std::string> lines{ bench.Lines() };
        const auto now{ std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()) };
        std::vector<std::string> report{};
        report.push_back(std::format("Raycast bench: film {}, {} records, K = {} (M4 and M5 every K records), {:%Y-%m-%d %H:%M:%S} UTC, {}.",
            film, outcome.Records, k_EveryRecords, now, Tests::Support::Bench::Configuration()));
        report.push_back(std::format("Pass: {:.1f} s. Times in microseconds per tick unless it says otherwise. mean/N4 = ms per tick averaged over a decision every 4 ticks.", outcome.Seconds));
        report.push_back("");
        report.insert(report.end(), lines.begin(), lines.end());

        const std::filesystem::path file{ Tests::Support::Bench::WriteReport(std::format("raycast_{}.txt", name), report) };
        ASSERT_FALSE(file.empty()) << "Cannot write the report of " << film;

        std::vector<std::string> sceneReport{};
        sceneReport.push_back(std::format("RS.1b objects at rest in the fixed tree: film {}, {} records, K = {}, scene every {} records, {:%Y-%m-%d %H:%M:%S} UTC, {}.",
            film, outcome.Records, k_EveryRecords, k_SceneEvery, now, Tests::Support::Bench::Configuration()));
        sceneReport.push_back("");
        for (const std::string& line : bench.SceneLines()) sceneReport.push_back(line);
        const std::filesystem::path sceneFile{ Tests::Support::Bench::WriteReport(std::format("rs1_{}.txt", name), sceneReport) };
        ASSERT_FALSE(sceneFile.empty()) << "Cannot write the scene report of " << film;

        ::testing::Test::RecordProperty("ReportFile", file.string());
        for (std::size_t index{ 0 }; index < report.size(); ++index)
        {
            if (!report[index].empty()) ::testing::Test::RecordProperty(std::format("Line{:02}", index), report[index]);
        }
        EXPECT_GT(outcome.Records, 0u);
    }
}

TEST(DISABLED_RaycastBench, CtfD)
{
    RunBench("ctf_d", "ctf_d");
}

TEST(DISABLED_RaycastBench, CtfC)
{
    RunBench("ctf_c", "ctf_c");
}

// Forge World with about 650 pieces (D300). The film is in BenchFilms so the replay tests do not walk it.
TEST(DISABLED_RaycastBench, Forge650)
{
    RunBench("BenchFilms/forge650", "forge650");
}

// Alien Landing 8v8 with 16 players acting at x10 (RS.2): the map of the in-game breakdown, to set the bench against the game.
TEST(DISABLED_RaycastBench, Alien8v8)
{
    RunBench("BenchFilms/alien8v8", "alien8v8");
}

// The rules of the rebuild of the fixed tree side by side (D301). The rules are in build/bench/rs1c_sweep.txt, one "N T" per line.
TEST(DISABLED_RaycastBench, SweepForge650)
{
    RunSweep("BenchFilms/forge650", "forge650");
}

TEST(DISABLED_RaycastBench, SweepCtfD)
{
    RunSweep("ctf_d", "ctf_d");
}

TEST(DISABLED_RaycastBench, SweepCtfC)
{
    RunSweep("ctf_c", "ctf_c");
}