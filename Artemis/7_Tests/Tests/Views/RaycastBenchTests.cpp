#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Math.Type;
import Common.Math.System;
import Common.Geometry.System;
import Blam.Runtime.Player.Type;
import Perception.Collidable.Type;
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
    using Common::Math::Type::Triangle;
    using Common::Math::Type::Vec3;
    using Perception::Collidable::Type::CollidablePart;
    using Tests::Support::Bench::Micros;
    using Tests::Support::Bench::Series;
    using Tests::Support::Bench::Stats;
    using Tests::Support::FilmPass::FilmOutcome;
    using Tests::Support::FilmPass::RecordingsFolder;
    using Tests::Support::FilmPass::RunFilmPass;
    using Tests::Support::Pipeline::Chain;
    using Views::Raycast::System::ArmDynamicCache;
    using Views::Raycast::System::BuildDynamicCache;
    using Views::Raycast::System::BuildSelfExclusion;
    using Views::Raycast::System::CastDynamic;
    using Views::Raycast::System::CastRay;
    using Views::Raycast::System::CastStatic;
    using Views::Raycast::System::DynamicCacheRadius;
    using Views::Raycast::System::PerceptionDirections;
    using Views::Raycast::System::PrepareParts;
    using Views::Raycast::System::RayHitsBox;
    using Views::Raycast::System::RayNearPoint;
    using Views::Raycast::System::ResolveAimRange;
    using Views::Raycast::System::ResolvePerceptionOrigin;
    using Views::Raycast::Type::CachedPart;
    using Views::Raycast::Type::DynamicCache;
    using Views::Raycast::Type::HitKind;
    using Views::Raycast::Type::PerceptionOrigin;
    using Views::Raycast::Type::RaycastHit;
    using Views::Raycast::Type::SelfExclusion;
    using std::chrono::steady_clock;

    using Common::Engine::Type::k_NoHandle;
    using Tests::Support::Bench::k_NanosPerMicro;
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

    // The 16 agent metrics (M2, M3, M4) run every K records, M0 and M1 run at every record. K = 30 on ctf_d, to stay under 150 s.
    constexpr std::size_t k_EveryRecords{ 30 };

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
        float Radius{};
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
        agent.Radius = DynamicCacheRadius(agent.AimRange, Common::Math::System::Distance(source.Camera, agent.RayPosition));
        agent.Directions = PerceptionDirections(forward);
        return agent;
    }

    // Copy of CastDynamic that counts the triangles it tests, so the production code carries no counter.
    // note: The bench checks every ray against CastDynamic.
    auto CastDynamicCounted(const DynamicCache& dynamicCache, const Vec3& origin, const Vec3& direction, float maxDistance,
        RaycastHit& best, std::size_t& triangles) -> void
    {
        for (const auto& [handle, entry] : dynamicCache)
        {
            const auto& collidable = entry.Data;

            float limit = best.Hit ? best.Distance : maxDistance;

            if (!RayNearPoint(origin, direction, limit, collidable.Position, entry.BoundingRadius)) continue;

            for (const CachedPart& part : entry.Parts)
            {
                limit = best.Hit ? best.Distance : maxDistance;

                if (!RayNearPoint(origin, direction, limit, part.Center, part.Radius)) continue;

                const auto& m = part.ToLocal;
                const Vec3 localOrigin{
                    m[0] * origin.X + m[1] * origin.Y + m[2] * origin.Z + m[3],
                    m[4] * origin.X + m[5] * origin.Y + m[6] * origin.Z + m[7],
                    m[8] * origin.X + m[9] * origin.Y + m[10] * origin.Z + m[11]
                };
                const Vec3 localDirection{
                    m[0] * direction.X + m[1] * direction.Y + m[2] * direction.Z,
                    m[4] * direction.X + m[5] * direction.Y + m[6] * direction.Z,
                    m[8] * direction.X + m[9] * direction.Y + m[10] * direction.Z
                };

                if (!RayHitsBox(localOrigin, localDirection, part.Source->LocalMin, part.Source->LocalMax, limit)) continue;

                for (const Triangle& triangle : part.Source->Triangles)
                {
                    float distance = 0.0f;
                    const float currentBest = best.Hit ? best.Distance : maxDistance;

                    ++triangles;
                    if (!Common::Geometry::System::RayIntersectsTriangle(
                        localOrigin, localDirection, triangle, currentBest, distance))
                    {
                        continue;
                    }

                    if (!best.Hit || distance < best.Distance)
                    {
                        best.Hit = true;
                        best.Kind = HitKind::Dynamic;
                        best.Distance = distance;
                        best.ObjectHandle = handle;
                        best.RegionIndex = part.Source->RegionIndex;
                        best.Point = {
                            origin.X + direction.X * distance,
                            origin.Y + direction.Y * distance,
                            origin.Z + direction.Z * distance
                        };
                    }
                }
            }
        }
    }

    auto SameHit(const RaycastHit& a, const RaycastHit& b) -> bool
    {
        return a.Hit == b.Hit && a.Kind == b.Kind && a.Distance == b.Distance && a.ObjectHandle == b.ObjectHandle;
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
                    m_StaticTriangles += sbsp.RenderGeometry.size();
                }
            }

            ++m_Measured;
            const RaycastHit down{ CastStatic(chain.m_Resolved.m_Raycaster, self.Camera, Vec3{ 0.0f, 0.0f, -1.0f }, 150.0f) };
            m_DownHits += down.Hit ? 1 : 0;
            this->MeasureUpdate(chain);
            this->MeasureOneAgent(chain, self);
            if (record % k_EveryRecords == 0) this->MeasureSixteenAgents(chain, self);
        }

        // return: The lines of the report, without the header.
        auto Lines() const -> std::vector<std::string>
        {
            std::vector<std::string> lines{};
            const auto time{ [&](std::string_view label, const Series& series)
            {
                const Stats s{ series.Summary() };
                lines.push_back(std::format("{:<34} n={:<6} mean={:>10.1f} p50={:>10.1f} p99={:>10.1f} max={:>10.1f} us   mean/N4={:.4f} ms",
                    label, s.N, s.Mean, s.P50, s.P99, s.Max, s.Mean / k_NanosPerMicro / k_DecisionEvery));
            } };
            const auto count{ [&](std::string_view label, const Series& series)
            {
                const Stats s{ series.Summary() };
                lines.push_back(std::format("{:<34} n={:<6} mean={:>10.2f} p99={:>10.1f} max={:>10.1f}", label, s.N, s.Mean, s.P99, s.Max));
            } };
            const auto ray{ [&](std::string_view label, const Series& series)
            {
                const Stats s{ series.Summary() };
                lines.push_back(std::format("{:<34} n={:<6} mean={:>10.3f} p50={:>10.3f} p99={:>10.3f} max={:>10.1f} us per ray",
                    label, s.N, s.Mean, s.P50, s.P99, s.Max));
            } };

            lines.push_back(std::format("Static world: {} sbsp, {} triangles. A ray straight down from the camera of Self hit it in {} of {} records.",
                m_Sbsps, m_StaticTriangles, m_DownHits, m_Measured));
            lines.push_back("");
            lines.push_back("M0 Update real (1 agent: Self), every record, us per tick");
            time("M0 Update", m_Update);
            lines.push_back("");
            lines.push_back("M1 breakdown of the same agent, every record, us per tick");
            time("M1 BuildSelfExclusion", m_Exclusion);
            time("M1 aim range + origin + directions", m_Setup);
            time("M1 QueryNearby", m_Query);
            time("M1 arm the cache (production call)", m_Arm);
            time("M1   of which CollectPartsFor", m_Collect);
            time("M1   of which PrepareParts", m_Prepare);
            time("M1 15 rays, static part (sum)", m_StaticTick);
            time("M1 15 rays, dynamic part (sum)", m_DynamicTick);
            ray("M1 ray, static part", m_StaticRay);
            ray("M1 ray, dynamic part", m_DynamicRay);
            count("M1 candidates of QueryNearby", m_Candidates);
            count("M1 objects in the cache", m_Objects);
            count("M1 parts in the cache", m_Parts);
            count("M1 triangles tested per tick", m_TrianglesTick);
            count("M1 triangles tested per ray", m_TrianglesRay);
            count("M1 rays that hit static (per tick)", m_HitsStatic);
            count("M1 rays that hit dynamic (per tick)", m_HitsDynamic);
            count("M1 rays that hit nothing (per tick)", m_HitsNone);
            lines.push_back(std::format("M1 check: {} rays compared with the instrumented copy and with RaycastStore, {} differences.",
                m_Compared, m_Differences));
            lines.push_back("");
            lines.push_back(std::format("M2 16 agents as today (own cache, 15 rays each), every {} records, us per tick", k_EveryRecords));
            time("M2 total", m_M2Total);
            time("M2   exclusion + aim + origin", m_M2Prepare);
            time("M2   caches (16)", m_M2Cache);
            time("M2   rays (240)", m_M2Rays);
            lines.push_back("");
            lines.push_back("M3 16 agents with ONE shared cache (no exclusion: an agent may hit its own biped), us per tick");
            time("M3 total", m_M3Total);
            time("M3   exclusion + aim + origin", m_M3Prepare);
            time("M3   shared cache (1)", m_M3Cache);
            time("M3   rays (240)", m_M3Rays);
            count("M3 objects in the shared cache", m_M3Objects);
            lines.push_back("");
            lines.push_back("M4 static world only: 16 x 15 rays");
            time("M4 total", m_M4Total);
            ray("M4 ray", m_M4Ray);

            lines.push_back("");
            lines.push_back("Heaviest, 1 agent (ms per tick, mean):");
            this->AppendHeaviest(lines, { { "QueryNearby", &m_Query }, { "arm the cache", &m_Arm }, { "BuildSelfExclusion", &m_Exclusion },
                { "static rays (15)", &m_StaticTick }, { "dynamic rays (15)", &m_DynamicTick }, { "Update (M0)", &m_Update } });
            lines.push_back("Heaviest, 16 agents (ms per tick, mean):");
            this->AppendHeaviest(lines, { { "M2 total", &m_M2Total }, { "M2 caches", &m_M2Cache }, { "M2 rays", &m_M2Rays },
                { "M3 total", &m_M3Total }, { "M3 shared cache", &m_M3Cache }, { "M3 rays", &m_M3Rays }, { "M4 static rays", &m_M4Total } });
            return lines;
        }

    private:
        std::vector<Origin> m_History{};
        std::size_t m_Measured{};
        std::size_t m_Sbsps{};
        std::size_t m_StaticTriangles{};
        std::size_t m_DownHits{};
        std::size_t m_Compared{};
        std::size_t m_Differences{};
        double m_Sink{};

        Series m_Update{};
        Series m_Exclusion{}, m_Setup{}, m_Query{}, m_Arm{}, m_Collect{}, m_Prepare{}, m_StaticTick{}, m_DynamicTick{};
        Series m_StaticRay{}, m_DynamicRay{};
        Series m_Candidates{}, m_Objects{}, m_Parts{}, m_TrianglesTick{}, m_TrianglesRay{}, m_HitsStatic{}, m_HitsDynamic{}, m_HitsNone{};
        Series m_M2Total{}, m_M2Prepare{}, m_M2Cache{}, m_M2Rays{};
        Series m_M3Total{}, m_M3Prepare{}, m_M3Cache{}, m_M3Rays{}, m_M3Objects{};
        Series m_M4Total{}, m_M4Ray{};

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

        // M1: the same agent as Update, piece by piece.
        auto MeasureOneAgent(Chain& chain, const Origin& self) -> void
        {
            auto& collidables{ chain.m_Environment.m_CollidableService };
            const auto& raycaster{ chain.m_Resolved.m_Raycaster };

            auto start{ steady_clock::now() };
            const SelfExclusion exclusion{ BuildSelfExclusion(chain.m_Environment.m_PlayersStore, chain.m_Environment.m_EntitiesStore,
                chain.m_Tables.m_ObjectStore, self.PlayerHandle, self.BipedHandle) };
            m_Exclusion.Add(Micros(start, steady_clock::now()));

            start = steady_clock::now();
            Agent agent{ PrepareAgent(chain, self) };
            m_Setup.Add(Micros(start, steady_clock::now()));

            start = steady_clock::now();
            const std::vector<std::uint32_t> candidates{ collidables.QueryNearby(self.Camera, agent.Radius) };
            m_Query.Add(Micros(start, steady_clock::now()));

            start = steady_clock::now();
            const DynamicCache cache{ ArmDynamicCache(collidables, candidates, agent.Exclusion) };
            m_Arm.Add(Micros(start, steady_clock::now()));

            // The same work as ArmDynamicCache, with a clock around each call.
            double collectUs{};
            double prepareUs{};
            std::size_t parts{};
            for (const std::uint32_t handle : candidates)
            {
                if (agent.Exclusion.contains(handle)) continue;

                start = steady_clock::now();
                const auto collidable{ collidables.CollectPartsFor(handle) };
                collectUs += Micros(start, steady_clock::now());
                if (!collidable) continue;

                start = steady_clock::now();
                const std::vector<CachedPart> prepared{ PrepareParts(collidable->Parts) };
                prepareUs += Micros(start, steady_clock::now());
                parts += prepared.size();
            }
            m_Collect.Add(collectUs);
            m_Prepare.Add(prepareUs);
            m_Candidates.Add(static_cast<double>(candidates.size()));
            m_Objects.Add(static_cast<double>(cache.size()));
            m_Parts.Add(static_cast<double>(parts));

            const auto published{ chain.m_Egocentric.m_RaycastStore.Acquire() };
            const std::array<Ray, k_RaysPerAgent> rays{ RaysOf(agent) };
            double staticUs{};
            double dynamicUs{};
            std::size_t triangles{};
            std::size_t hitsStatic{};
            std::size_t hitsDynamic{};
            std::size_t hitsNone{};
            for (std::size_t index{ 0 }; index < rays.size(); ++index)
            {
                const Ray& ray{ rays[index] };

                start = steady_clock::now();
                RaycastHit hit{ CastStatic(raycaster, ray.Origin, ray.Direction, ray.Range) };
                const auto middle{ steady_clock::now() };
                CastDynamic(cache, ray.Origin, ray.Direction, ray.Range, hit);
                const auto end{ steady_clock::now() };
                staticUs += Micros(start, middle);
                dynamicUs += Micros(middle, end);
                m_StaticRay.Add(Micros(start, middle));
                m_DynamicRay.Add(Micros(middle, end));

                RaycastHit counted{ CastStatic(raycaster, ray.Origin, ray.Direction, ray.Range) };
                std::size_t rayTriangles{};
                CastDynamicCounted(cache, ray.Origin, ray.Direction, ray.Range, counted, rayTriangles);
                triangles += rayTriangles;
                m_TrianglesRay.Add(static_cast<double>(rayTriangles));

                ++m_Compared;
                if (!SameHit(hit, counted)) ++m_Differences;
                if (published)
                {
                    const RaycastHit& expected{ index == 0 ? published->AimHit : published->PerceptionHits[index - 1] };
                    if (!SameHit(hit, expected)) ++m_Differences;
                }

                if (!hit.Hit) ++hitsNone;
                else if (hit.Kind == HitKind::Static) ++hitsStatic;
                else ++hitsDynamic;
            }
            m_StaticTick.Add(staticUs);
            m_DynamicTick.Add(dynamicUs);
            m_TrianglesTick.Add(static_cast<double>(triangles));
            m_HitsStatic.Add(static_cast<double>(hitsStatic));
            m_HitsDynamic.Add(static_cast<double>(hitsDynamic));
            m_HitsNone.Add(static_cast<double>(hitsNone));
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

        // M2, M3 and M4 for the origins of this record.
        auto MeasureSixteenAgents(Chain& chain, const Origin& self) -> void
        {
            auto& collidables{ chain.m_Environment.m_CollidableService };
            const auto& raycaster{ chain.m_Resolved.m_Raycaster };
            const std::vector<Origin> origins{ this->BuildOrigins(chain, self) };

            // M2: every agent resolves, builds its cache and casts.
            double prepareUs{};
            double cacheUs{};
            double raysUs{};
            const auto total2{ steady_clock::now() };
            for (const Origin& origin : origins)
            {
                auto start{ steady_clock::now() };
                const Agent agent{ PrepareAgent(chain, origin) };
                prepareUs += Micros(start, steady_clock::now());

                start = steady_clock::now();
                const DynamicCache cache{ BuildDynamicCache(collidables, agent.Source.Camera, agent.Radius, agent.Exclusion) };
                cacheUs += Micros(start, steady_clock::now());

                start = steady_clock::now();
                for (const Ray& ray : RaysOf(agent)) m_Sink += CastRay(raycaster, cache, ray.Origin, ray.Direction, ray.Range).Distance;
                raysUs += Micros(start, steady_clock::now());
            }
            m_M2Total.Add(Micros(total2, steady_clock::now()));
            m_M2Prepare.Add(prepareUs);
            m_M2Cache.Add(cacheUs);
            m_M2Rays.Add(raysUs);

            // M3: the same agents over one cache that holds the sphere of all of them.
            const auto total3{ steady_clock::now() };
            std::vector<Agent> agents{};
            agents.reserve(origins.size());
            for (const Origin& origin : origins) agents.push_back(PrepareAgent(chain, origin));
            const auto prepared3{ steady_clock::now() };

            Vec3 center{};
            for (const Agent& agent : agents)
            {
                center.X += agent.Source.Camera.X / static_cast<float>(agents.size());
                center.Y += agent.Source.Camera.Y / static_cast<float>(agents.size());
                center.Z += agent.Source.Camera.Z / static_cast<float>(agents.size());
            }
            float radius{};
            for (const Agent& agent : agents) radius = (std::max)(radius, Common::Math::System::Distance(center, agent.Source.Camera) + agent.Radius);
            const DynamicCache shared{ BuildDynamicCache(collidables, center, radius, SelfExclusion{}) };
            const auto cached3{ steady_clock::now() };

            for (const Agent& agent : agents)
            {
                for (const Ray& ray : RaysOf(agent)) m_Sink += CastRay(raycaster, shared, ray.Origin, ray.Direction, ray.Range).Distance;
            }
            const auto end3{ steady_clock::now() };
            m_M3Total.Add(Micros(total3, end3));
            m_M3Prepare.Add(Micros(total3, prepared3));
            m_M3Cache.Add(Micros(prepared3, cached3));
            m_M3Rays.Add(Micros(cached3, end3));
            m_M3Objects.Add(static_cast<double>(shared.size()));

            // M4: the same rays against the static world only.
            const auto start4{ steady_clock::now() };
            for (const Agent& agent : agents)
            {
                for (const Ray& ray : RaysOf(agent)) m_Sink += CastStatic(raycaster, ray.Origin, ray.Direction, ray.Range).Distance;
            }
            const double total4{ Micros(start4, steady_clock::now()) };
            m_M4Total.Add(total4);
            m_M4Ray.Add(total4 / static_cast<double>(agents.size() * k_RaysPerAgent));
        }
    };

    // Replays the film, measures, and writes the report to build/bench/raycast_<film>.txt of the repository.
    auto RunBench(std::string_view film) -> void
    {
        RaycastBench bench{};
        const FilmOutcome outcome{ RunFilmPass(film, false, [&](const Chain& chain, std::size_t record) { bench.Observe(chain, record); }) };
        if (!outcome.Skip.empty()) GTEST_SKIP() << outcome.Skip;
        ASSERT_TRUE(outcome.Error.empty()) << outcome.Error;

        const std::vector<std::string> lines{ bench.Lines() };
        const auto now{ std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()) };
        std::vector<std::string> report{};
        report.push_back(std::format("Raycast bench: film {}, {} records, K = {} (M2, M3 and M4 every K records), {:%Y-%m-%d %H:%M:%S} UTC, {}.",
            film, outcome.Records, k_EveryRecords, now, Tests::Support::Bench::Configuration()));
        report.push_back(std::format("Pass: {:.1f} s. Times in microseconds per tick unless it says otherwise. mean/N4 = ms per tick averaged over a decision every 4 ticks.", outcome.Seconds));
        report.push_back("");
        report.insert(report.end(), lines.begin(), lines.end());

        const std::filesystem::path file{ Tests::Support::Bench::WriteReport(std::format("raycast_{}.txt", film), report) };
        ASSERT_FALSE(file.empty()) << "Cannot write the report of " << film;

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
    RunBench("ctf_d");
}

TEST(DISABLED_RaycastBench, CtfC)
{
    RunBench("ctf_c");
}