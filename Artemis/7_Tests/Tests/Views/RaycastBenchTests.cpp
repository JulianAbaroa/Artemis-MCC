#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Math.Type;
import Common.Math.System;
import Blam.MapModel.World.System;
import Blam.MapModel.World.Type;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Player.Type;
import Perception.Entities.Type;
import Perception.ObjectGraph.Type;
import Perception.Players.Type;
import Views.Raycast.Type;
import Views.Raycast.System;
import Views.View.Type;
import Tests.Support.Bench;
import Tests.Support.FilmPass;
import Tests.Support.Pipeline;
import Tests.Support.ViewProbe;
import std;

// Offline bench of the rays of Views (D273, F3.B): what each part of the rays of the Self of before cost for one agent, and what it would cost
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
    using Blam::MapModel::World::System::Raycaster;
    using Blam::Runtime::Object::Type::Alive::ObjectTable;
    using Perception::Players::Type::Player;
    using Perception::Players::Type::Players;
    using Tests::Support::ViewProbe::EyeProbe;
    using Tests::Support::ViewProbe::PlayerEye;
    using Views::Raycast::System::ArmFixedScene;
    using Views::Raycast::System::ArmMobileScene;
    using Views::Raycast::System::BuildGraphExclusions;
    using Views::Raycast::System::CastScene;
    using Views::Raycast::System::CastSceneRay;
    using Views::Raycast::System::CastSceneRayReach;
    using Views::Raycast::System::CastStatic;
    using Views::Raycast::System::GatherSceneObjects;
    using Views::Raycast::System::GatherSceneReach;
    using Views::Raycast::System::PrepareMeshBvhs;
    using Views::Raycast::System::PreparePart;
    using Views::Raycast::System::FindVehicleRoot;
    using Views::Raycast::Type::CachedPart;
    using Views::Raycast::Type::EyePoint;
    using Views::Raycast::Type::FixedRebuildRule;
    using Views::Raycast::Type::MobileRebuildRule;
    using Views::Raycast::Type::PlayerExclusions;
    using Views::Raycast::Type::RaycastHit;
    using Views::Raycast::Type::RaycastScene;
    using Views::Raycast::Type::SceneCounters;
    using Views::Raycast::Type::SceneReach;
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
    using Views::Raycast::Type::Constant::k_MobileRebuildArea;
    using Views::Raycast::Type::Constant::k_FallbackAimRange;
    using Views::Raycast::Type::Constant::k_MaxRayRange;
    using Common::Engine::Type::k_PrimaryLocalUser;

    // The rays of the perception of the Self of before: a cube of 14 rays of 20 m, kept as the reference of the cost of one agent.
    constexpr int k_PerceptionRayCount{ 14 };
    constexpr float k_PerceptionRange{ 20.0f };

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

    // The shared scene is armed every this many records: a decision every 4 ticks (D273).
    constexpr std::size_t k_SceneEvery{ 4 };

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

    // The cube of the perception rays of the Self of before: six faces and eight corners, 14 rays, kept for the cost of the rays of one agent.
    constexpr float k_InvSqrt3{ 0.57735026918962576451f };
    constexpr std::array<Vec3, k_PerceptionRayCount> k_CubeDirections{ {
        {  1.0f,  0.0f,  0.0f }, { -1.0f,  0.0f,  0.0f }, {  0.0f,  1.0f,  0.0f }, {  0.0f, -1.0f,  0.0f }, {  0.0f,  0.0f,  1.0f }, {  0.0f,  0.0f, -1.0f },
        {  k_InvSqrt3,  k_InvSqrt3,  k_InvSqrt3 }, {  k_InvSqrt3,  k_InvSqrt3, -k_InvSqrt3 }, {  k_InvSqrt3, -k_InvSqrt3,  k_InvSqrt3 },
        {  k_InvSqrt3, -k_InvSqrt3, -k_InvSqrt3 }, { -k_InvSqrt3,  k_InvSqrt3,  k_InvSqrt3 }, { -k_InvSqrt3,  k_InvSqrt3, -k_InvSqrt3 },
        { -k_InvSqrt3, -k_InvSqrt3,  k_InvSqrt3 }, { -k_InvSqrt3, -k_InvSqrt3, -k_InvSqrt3 } } };

    // Where the perception rays of an agent start and the forward of their frame.
    struct PerceptionOrigin
    {
        Vec3 Position{};
        Vec3 Forward{};
        bool Valid{ false };
    };

    // return: The 14 directions of the cube in the frame of forward.
    auto PerceptionDirections(const Vec3& forward) -> std::array<Vec3, k_PerceptionRayCount>
    {
        Vec3 originForward{}, originRight{}, originUp{};
        Common::Math::System::BuildFrame(forward, originForward, originRight, originUp);

        std::array<Vec3, k_PerceptionRayCount> directions{};
        for (std::size_t index{ 0 }; index < directions.size(); ++index)
        {
            const Vec3& local{ k_CubeDirections[index] };
            directions[index] = Vec3{
                originRight.X * local.X + originUp.X * local.Y + originForward.X * local.Z,
                originRight.Y * local.X + originUp.Y * local.Y + originForward.Y * local.Z,
                originRight.Z * local.X + originUp.Z * local.Y + originForward.Z * local.Z };
        }

        return directions;
    }

    // return: The aim range of the primary weapon of the player, capped at the maximum ray range. The fallback range if it has none.
    auto ResolveAimRange(const Chain& chain, std::uint32_t playerHandle) -> float
    {
        float range{ k_FallbackAimRange };
        const auto players{ chain.m_Tables.m_PlayerStore.Acquire() };
        const auto objects{ chain.m_Tables.m_ObjectStore.Acquire() };
        if (players && objects)
        {
            const auto player{ players->find(playerHandle) };
            const std::uint32_t weaponHandle{ player != players->end() ? player->second.PrimaryWeaponHandle : k_NoHandle };
            const auto weapon{ objects->find(weaponHandle) };
            if (weaponHandle != 0 && weapon != objects->end())
            {
                const auto* weap{ chain.m_Resolved.m_DefinitionsStore.m_Weap.Get(weapon->second.TagName) };
                if (weap && weap->AutoaimRange > 0.0f) range = weap->AutoaimRange;
            }
        }

        return (std::min)(range, k_MaxRayRange);
    }

    // return: The center and the forward of the biped, or of the vehicle it rides.
    auto ResolvePerceptionOrigin(const Chain& chain, std::uint32_t bipedHandle) -> PerceptionOrigin
    {
        std::uint32_t target{ bipedHandle };
        const auto entities{ chain.m_Environment.m_EntitiesStore.Acquire() };
        if (entities && bipedHandle != k_NoHandle)
        {
            const std::uint32_t vehicle{ FindVehicleRoot(*entities, bipedHandle) };
            if (vehicle != k_NoHandle) target = vehicle;
        }

        const auto objects{ chain.m_Tables.m_ObjectStore.Acquire() };
        if (target == k_NoHandle || !objects) return {};

        const auto object{ objects->find(target) };
        if (object == objects->end()) return {};

        return { object->second.BoundingCenter, object->second.Forward, true };
    }

    // return: The exclusion of one player, from the pass over the object graph. Empty if the player is not in the tick.
    auto ExclusionOf(const Chain& chain, std::uint32_t playerHandle) -> SelfExclusion
    {
        const auto players{ chain.m_Environment.m_PlayersStore.Acquire() };
        const auto entities{ chain.m_Environment.m_EntitiesStore.Acquire() };
        const auto nodes{ chain.m_Environment.m_ObjectGraphStore.Acquire() };
        if (!players || !entities || !nodes) return {};

        const Player* player{ players->Find(playerHandle) };
        if (!player) return {};

        const PlayerExclusions exclusions{ BuildGraphExclusions(*players, *entities, *nodes) };
        const std::size_t row{ static_cast<std::size_t>(player - players->Rows.data()) };
        return row < exclusions.size() ? exclusions[row] : SelfExclusion{};
    }

    // return: The primary local player as the bench sees it: the camera, the forward of the camera and the handles. Empty if it is not alive.
    auto LocalOrigin(const Chain& chain) -> std::optional<Origin>
    {
        const auto table{ chain.m_Tables.m_PlayerStore.Acquire() };
        if (!table) return std::nullopt;

        for (const auto& entry : *table)
        {
            const auto& player{ entry.second };
            if (player.LocalIndex != k_PrimaryLocalUser) continue;
            if (player.AliveBipedHandle == k_NoHandle) return std::nullopt;

            Vec3 forward{}, right{}, up{};
            Common::Math::System::BuildFrame(player.CameraForward, forward, right, up);
            return Origin{ player.CameraPosition, forward, player.Handle, player.CurrentBipedHandle, false };
        }

        return std::nullopt;
    }

    // What one agent needs before it casts: the aim range, the exclusion and the perception origin and directions of the old Self.
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

    // The values the Self of before resolved for an agent.
    auto PrepareAgent(const Chain& chain, const Origin& source) -> Agent
    {
        Agent agent{};
        agent.Source = source;
        agent.AimRange = ResolveAimRange(chain, source.PlayerHandle);
        agent.Exclusion = ExclusionOf(chain, source.PlayerHandle);

        const PerceptionOrigin perceptionOrigin{ source.IsPast ? PerceptionOrigin{} : ResolvePerceptionOrigin(chain, source.BipedHandle) };
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

    // Sight (design 1.4, F4.B): a player is a candidate to see another when it is within the range and the other falls inside the cone around
    // its aim. The range and the half angle are the ones the Views use (Data/Views/Sight.xml).
    using Views::Raycast::Type::Sight::k_SightHalfAngle;
    using Views::Raycast::Type::Sight::k_SightRange;

    constexpr float k_DegreesToRadians{ 0.017453292519943295f };

    // Same result of an aim ray: the same hit or miss, the same kind and object, and the distance within this many world units.
    constexpr float k_AimTolerance{ 0.01f };

    // The same call as a decision: the cone and range of the design for the player of this row.
    struct SightPoint
    {
        std::size_t Row{};
        Vec3 Eye{};
        Vec3 Aim{};
    };

    // What the sight pass of a decision counted, and who sees whom (rows of Players, a viewer in the first index).
    struct SightCounts
    {
        std::size_t Alive{};
        std::size_t Pairs{};
        std::size_t Candidates{};
        std::size_t Free{};
        std::array<std::array<std::uint8_t, k_Agents>, k_Agents> Seen{};
    };

    auto LengthOf(const Vec3& v) -> float
    {
        return std::sqrt((v.X * v.X) + (v.Y * v.Y) + (v.Z * v.Z));
    }

    auto DotOf(const Vec3& a, const Vec3& b) -> float
    {
        return (a.X * b.X) + (a.Y * b.Y) + (a.Z * b.Z);
    }

    // return: The vector with length 1, or the fallback when it has no length.
    auto UnitOf(const Vec3& v, const Vec3& fallback) -> Vec3
    {
        const float length{ LengthOf(v) };
        return length > 1.0e-6f ? Vec3{ v.X / length, v.Y / length, v.Z / length } : fallback;
    }

    // return: Whether two aim rays hit the same thing: both miss, or both hit the same kind and object.
    auto SameAimTarget(const RaycastHit& a, const RaycastHit& b) -> bool
    {
        if (a.Hit != b.Hit) return false;
        return !a.Hit || (a.Kind == b.Kind && a.ObjectHandle == b.ObjectHandle);
    }

    // return: The eye and aim of every live player with a biped and a tag, in the order of the rows (at most k_Agents).
    auto BuildSightPoints(const Chain& chain, const Players& players, const ObjectTable& objects) -> std::vector<SightPoint>
    {
        std::vector<SightPoint> points{};
        for (std::size_t row{ 0 }; row < players.Rows.size() && row < k_Agents; ++row)
        {
            const Player& player{ players.Rows[row] };
            if (!player.IsAlive()) continue;

            const EyePoint eye{ PlayerEye(chain, objects, player) };
            if (!eye.Valid) continue;

            const Vec3 aim{ UnitOf(player.Body->Aim, Vec3{}) };
            if (LengthOf(aim) == 0.0f) continue;

            points.push_back(SightPoint{ row, eye.Position, aim });
        }

        return points;
    }

    // The sight pass: one ray from eye to eye for each pair of live players with the other in the cone of at least one of them, cast through the
    // scene excluding both. Free = nothing hit before the other eye.
    // param exclusions: The exclusion of each row of Players.
    // param hits: Receives the hit of every ray in order, to compare scenes. Null for none.
    auto RunSight(const Raycaster& raycaster, const RaycastScene& scene, const std::vector<SightPoint>& points, const PlayerExclusions& exclusions,
        SightCounts& counts, std::vector<RaycastHit>* hits) -> void
    {
        const float cosHalf{ std::cos(k_SightHalfAngle * k_DegreesToRadians) };
        counts.Alive = points.size();
        counts.Pairs = points.size() * (points.size() - 1) / 2;
        for (std::size_t first{ 0 }; first < points.size(); ++first)
        {
            for (std::size_t second{ first + 1 }; second < points.size(); ++second)
            {
                const SightPoint& a{ points[first] };
                const SightPoint& b{ points[second] };
                const Vec3 delta{ b.Eye.X - a.Eye.X, b.Eye.Y - a.Eye.Y, b.Eye.Z - a.Eye.Z };
                const float distance{ LengthOf(delta) };
                if (distance > k_SightRange || distance < 1.0e-4f) continue;

                const Vec3 direction{ delta.X / distance, delta.Y / distance, delta.Z / distance };
                const bool aSeesB{ DotOf(a.Aim, direction) >= cosHalf };
                const bool bSeesA{ -DotOf(b.Aim, direction) >= cosHalf };
                if (!aSeesB && !bSeesA) continue;

                ++counts.Candidates;
                SelfExclusion both{ exclusions[a.Row] };
                both.insert(exclusions[b.Row].begin(), exclusions[b.Row].end());
                const RaycastHit hit{ CastSceneRay(raycaster, scene, both, a.Eye, direction, distance, nullptr) };
                if (hits) hits->push_back(hit);
                if (hit.Hit) continue;

                ++counts.Free;
                if (aSeesB) counts.Seen[a.Row][b.Row] = 1;
                if (bSeesA) counts.Seen[b.Row][a.Row] = 1;
            }
        }
    }

    // The size of a View of the design (1.2) as one flat struct: a bound for the cost of building them, not the final type (F4.1).
    struct ViewSelfRow
    {
        Vec3 Eye{}, Feet{}, Aim{}, Velocity{}, OwnVelocity{};
        float Crouch{}, Shield{}, Energy{}, Yaw{}, Pitch{};
        std::array<std::uint32_t, 8> Counters{};
        std::array<std::byte, 32> Weapons{};
    };

    struct ViewRayRow
    {
        float Distance{};
        std::uint32_t Handle{};
        std::uint32_t Flags{};
        std::uint8_t Kind{}, Function{}, Relation{}, Padding{};
    };

    struct ViewOtherRow
    {
        Vec3 Position{};
        Vec3 Velocity{};
        float Facing{};
        std::array<std::uint8_t, 4> Flags{};
    };

    struct ViewFlagRow
    {
        Vec3 Position{};
        Vec3 Stand{};
        std::array<std::uint32_t, 6> Values{};
    };

    struct ViewSoundRow
    {
        Vec3 Direction{};
        std::array<float, 3> Values{};
        std::array<std::uint32_t, 4> Tags{};
    };

    struct ViewNoticeRow
    {
        std::array<std::uint32_t, 4> Values{};
    };

    struct LocalView
    {
        std::array<std::uint32_t, 4> Head{};
        ViewSelfRow Self{};
        ViewRayRow Aim{};
        std::array<ViewRayRow, 16> Rays{};
        std::array<ViewOtherRow, 16> Others{};
        std::array<ViewFlagRow, 2> Flags{};
        std::array<ViewSoundRow, 8> Sounds{};
        std::array<ViewNoticeRow, 8> Notices{};
    };

    // One scene kept up to date at one cadence, with what each arming did (F4.B).
    struct SceneArming
    {
        double Gather{};
        double Meshes{};
        double Fixed{};
        double Mobile{};

        auto Total() const -> double { return Gather + Meshes + Fixed + Mobile; }
    };

    class SceneStream
    {
    public:
        RaycastScene Scene{};

        // Gathers the objects of the tick and arms both trees, as the Views do on a decision.
        auto Arm(Chain& chain) -> SceneArming
        {
            SceneArming times{};
            SceneGatherCounters gathered{};
            auto start{ steady_clock::now() };
            SceneObjects objects{ GatherSceneObjects(m_Reuse, chain.m_Environment.m_CollidableService, chain.m_Tables.m_ObjectStore, &gathered) };
            times.Gather = Micros(start, steady_clock::now());

            start = steady_clock::now();
            PrepareMeshBvhs(Scene, objects, false);
            times.Meshes = Micros(start, steady_clock::now());

            start = steady_clock::now();
            ArmFixedScene(Scene, objects);
            times.Fixed = Micros(start, steady_clock::now());

            const std::size_t refits{ Scene.MobileRefits };
            const std::size_t degraded{ Scene.MobileDegraded };
            start = steady_clock::now();
            ArmMobileScene(Scene, objects);
            times.Mobile = Micros(start, steady_clock::now());

            ++m_Armings;
            if (Scene.MobileRefits > refits || Scene.MobileDegraded > degraded) m_Ratio.Add(static_cast<double>(Scene.MobileAreaRatio));
            m_Arming.Add(times.Total());
            m_Rest.Add(static_cast<double>(std::ranges::count_if(objects, [](const SceneObject& object) { return object.IsFixed; })));
            m_FixedLeaves.Add(static_cast<double>(Scene.Fixed.Pieces.size()));
            m_MobileLeaves.Add(static_cast<double>(Scene.Mobile.Pieces.size()));
            m_Waiting.Add(static_cast<double>(Scene.FixedPending));
            return times;
        }

        // return: The lines of what the armings of this cadence did.
        auto Lines(std::string_view label) const -> std::vector<std::string>
        {
            std::vector<std::string> lines{};
            lines.push_back(std::format("{}: {} armings. FIXED tree built {} times. MOBILE tree built {} times ({} asked for by the area rule), refitted {} times.",
                label, m_Armings, Scene.FixedBuilds, Scene.MobileBuilds, Scene.MobileDegraded, Scene.MobileRefits));
            const Stats ratio{ m_Ratio.Summary() };
            lines.push_back(std::format("{}: MobileAreaRatio of the refits  n={:<6} p50={:>8.4f} p99={:>8.4f} max={:>8.4f} (k_MobileRebuildArea = {})",
                label, ratio.N, ratio.P50, ratio.P99, ratio.Max, k_MobileRebuildArea));
            lines.push_back(TimeLine(std::format("{}: arming", label), m_Arming));
            lines.push_back(CountLine(std::format("{}: objects at rest", label), m_Rest));
            lines.push_back(CountLine(std::format("{}: leaves of FIXED", label), m_FixedLeaves));
            lines.push_back(CountLine(std::format("{}: leaves of MOBILE", label), m_MobileLeaves));
            lines.push_back(CountLine(std::format("{}: objects at rest waiting", label), m_Waiting));
            return lines;
        }

    private:
        ScenePartsCache m_Reuse{};
        std::size_t m_Armings{};
        Series m_Ratio{}, m_Arming{}, m_Rest{}, m_FixedLeaves{}, m_MobileLeaves{}, m_Waiting{};
    };

    // What one of the 16 agents casts from, resolved as a View would: the eye A, the aim of the engine and the perception origin.
    struct ViewAgent
    {
        Origin Source{};
        std::size_t Row{ k_Agents };
        EyePoint Eye{};
        Vec3 Aim{};
        float AimRange{};
        Vec3 RayPosition{};
        std::array<Vec3, k_PerceptionRayCount> Directions{};
    };

    // M6: everything RS.2 and the views per agent add to a decision, measured as the pipeline would run it once per decision (F4.B).
    class DecisionBench
    {
    public:
        // Gathers and arms the scene of this decision.
        auto ArmScene(Chain& chain) -> SceneArming { return m_Stream.Arm(chain); }

        // The decision: exclusions from the graph, eyes and origins, the 16 x 15 rays, sight and the 16 views, each timed. The scene was armed before.
        auto Measure(Chain& chain, const std::vector<Origin>& origins, const SceneArming& armed) -> void
        {
            const auto players{ chain.m_Environment.m_PlayersStore.Acquire() };
            const auto entities{ chain.m_Environment.m_EntitiesStore.Acquire() };
            const auto nodes{ chain.m_Environment.m_ObjectGraphStore.Acquire() };
            const auto objects{ chain.m_Tables.m_ObjectStore.Acquire() };
            if (!players || !entities || !nodes || !objects) return;

            const auto& raycaster{ chain.m_Resolved.m_Raycaster };
            const RaycastScene& scene{ m_Stream.Scene };
            const SelfExclusion none{};

            // The exclusions of the players: one pass over the graph.
            auto start{ steady_clock::now() };
            const PlayerExclusions graph{ BuildGraphExclusions(*players, *entities, *nodes) };
            const double exclusionUs{ Micros(start, steady_clock::now()) };

            // The eye, the aim, the range and the perception origin of each agent.
            start = steady_clock::now();
            std::vector<ViewAgent> agents{};
            agents.reserve(origins.size());
            for (const Origin& origin : origins) agents.push_back(this->Prepare(chain, *players, *objects, origin));
            const double prepareUs{ Micros(start, steady_clock::now()) };

            const auto exclusionOf{ [&](const ViewAgent& agent) -> const SelfExclusion&
            {
                return agent.Row < graph.size() ? graph[agent.Row] : none;
            } };

            // The aim ray from the eye along the aim of the engine, and the perception rays through the reach of each agent.
            std::vector<RaycastHit> aimHits{};
            std::vector<RaycastHit> perceptionHits{};
            aimHits.reserve(agents.size());
            perceptionHits.reserve(agents.size() * k_PerceptionRayCount);
            double aimUs{ 0.0 };
            double perceptionUs{ 0.0 };
            for (const ViewAgent& agent : agents)
            {
                const SelfExclusion& exclusion{ exclusionOf(agent) };
                start = steady_clock::now();
                aimHits.push_back(CastSceneRay(raycaster, scene, exclusion, agent.Eye.Position, agent.Aim, agent.AimRange, nullptr));
                aimUs += Micros(start, steady_clock::now());

                start = steady_clock::now();
                const SceneReach reach{ GatherSceneReach(scene, exclusion, agent.RayPosition, k_PerceptionRange, nullptr) };
                for (const Vec3& direction : agent.Directions)
                {
                    perceptionHits.push_back(CastSceneRayReach(raycaster, scene, reach, exclusion, agent.RayPosition, direction, k_PerceptionRange, nullptr));
                }
                perceptionUs += Micros(start, steady_clock::now());
            }

            // Sight.
            start = steady_clock::now();
            const std::vector<SightPoint> points{ BuildSightPoints(chain, *players, *objects) };
            SightCounts sight{};
            RunSight(raycaster, scene, points, graph, sight, nullptr);
            const double sightUs{ Micros(start, steady_clock::now()) };

            // The 16 views, copied without a bridge.
            start = steady_clock::now();
            this->Assemble(agents, *players, *objects, aimHits, perceptionHits, sight);
            const double viewsUs{ Micros(start, steady_clock::now()) };

            m_Exclusion.Add(exclusionUs);
            m_Prepare.Add(prepareUs);
            m_Scene.Add(armed.Total());
            m_Aim.Add(aimUs);
            m_Perception.Add(perceptionUs);
            m_SightUs.Add(sightUs);
            m_Views.Add(viewsUs);
            m_Total.Add(exclusionUs + prepareUs + armed.Total() + aimUs + perceptionUs + sightUs + viewsUs);
            m_Alive.Add(static_cast<double>(sight.Alive));
            m_Pairs.Add(static_cast<double>(sight.Pairs));
            m_Candidates.Add(static_cast<double>(sight.Candidates));
            m_Free.Add(static_cast<double>(sight.Free));
            if (sight.Candidates > 0) m_SightRay.Add(sightUs / static_cast<double>(sight.Candidates));

            // Outside the clocks: the aim ray of today, from the camera of the player, and which aim rays change.
            start = steady_clock::now();
            std::vector<RaycastHit> todayHits{};
            todayHits.reserve(agents.size());
            for (const ViewAgent& agent : agents)
            {
                todayHits.push_back(CastSceneRay(raycaster, scene, exclusionOf(agent), agent.Source.Camera, agent.Source.Forward, agent.AimRange, nullptr));
            }
            m_AimToday.Add(Micros(start, steady_clock::now()));

            for (std::size_t index{ 0 }; index < agents.size(); ++index)
            {
                const ViewAgent& agent{ agents[index] };
                if (agent.Source.IsPast || agent.Row >= players->Rows.size()) continue;

                const bool isLocal{ players->Rows[agent.Row].LocalIndex.has_value() };
                ++(isLocal ? m_LocalAimRays : m_OtherAimRays);
                if (isLocal)
                {
                    const float cosine{ std::clamp(DotOf(agent.Aim, agent.Source.Forward), -1.0f, 1.0f) };
                    m_AimAngle.Add(static_cast<double>(std::acos(cosine) / k_DegreesToRadians));
                }

                const bool isSameTarget{ SameAimTarget(aimHits[index], todayHits[index]) };
                const bool isSameDistance{ !aimHits[index].Hit || std::fabs(aimHits[index].Distance - todayHits[index].Distance) <= k_AimTolerance };
                if (!isSameTarget || !isSameDistance) ++(isLocal ? m_LocalAimChanged : m_OtherAimChanged);
                if (isLocal && !isSameTarget) ++m_LocalAimTargetChanged;
                if (isLocal && isSameTarget && aimHits[index].Hit) m_AimDistance.Add(static_cast<double>(std::fabs(aimHits[index].Distance - todayHits[index].Distance)));
            }
        }

        // return: The lines of the table M6 of the decisions.
        auto Lines() const -> std::vector<std::string>
        {
            std::vector<std::string> lines{};
            lines.push_back(std::format("M6 16 agents, one decision every {} records (all of RS.2 and the views per agent), us per decision", k_SceneEvery));
            lines.push_back(TimeLine("M6 exclusions: one pass over the graph", m_Exclusion));
            lines.push_back(TimeLine("M6 eye A + aim + range + origin x16", m_Prepare));
            lines.push_back(TimeLine("M6 scene: gather + FIXED + MOBILE", m_Scene));
            lines.push_back(TimeLine("M6 aim rays x16 from Eye A + Body.Aim", m_Aim));
            lines.push_back(TimeLine("M6   aim rays x16 from the camera (today)", m_AimToday));
            lines.push_back(std::format("M6   aim rays that change against today (other object, kind or distance > {} wu): local {} of {}, other players {} of {}.",
                k_AimTolerance, m_LocalAimChanged, m_LocalAimRays, m_OtherAimChanged, m_OtherAimRays));
            const Stats delta{ m_AimDistance.Summary() };
            lines.push_back(std::format("M6   of the local ones, {} hit another target (object, kind or hit against miss). Distance difference of the local rays on the same target, wu: n={} p50={:.4f} p99={:.4f} max={:.4f}.",
                m_LocalAimTargetChanged, delta.N, delta.P50, delta.P99, delta.Max));
            const Stats angle{ m_AimAngle.Summary() };
            lines.push_back(std::format("M6   angle between Body.Aim and the camera forward of today, local players, degrees: n={} p50={:.4f} p99={:.4f} max={:.4f}",
                angle.N, angle.P50, angle.P99, angle.Max));
            lines.push_back(TimeLine("M6 perception rays x224 (reach + casts)", m_Perception));
            lines.push_back(CountLine("M6 sight: live players", m_Alive));
            lines.push_back(CountLine("M6 sight: pairs of live players", m_Pairs));
            lines.push_back(CountLine("M6 sight: candidates (= rays)", m_Candidates));
            lines.push_back(CountLine("M6 sight: free lines", m_Free));
            lines.push_back(TimeLine("M6 sight: total", m_SightUs));
            lines.push_back(RayLine("M6 sight: per ray", m_SightRay));
            lines.push_back(TimeLine("M6 views: 16 Views assembled, no bridge", m_Views));
            lines.push_back(std::format("M6 views: {} bytes per View, {} bytes for 16 (a bound: one flat struct of the size of the design, not the final type).",
                sizeof(LocalView), sizeof(LocalView) * k_Agents));
            lines.push_back(TimeLine("M6 TOTAL per decision", m_Total));
            for (const std::string& line : m_Stream.Lines("M6 scene at one gather per decision")) lines.push_back(line);
            return lines;
        }

        auto Stream() const -> const SceneStream& { return m_Stream; }
        auto TotalMeanMs() const -> double { return m_Total.Summary().Mean / k_NanosPerMicro; }
        auto TotalWithoutSightMs() const -> double { return (m_Total.Summary().Mean - m_SightUs.Summary().Mean) / k_NanosPerMicro; }

    private:
        // The eye, aim, range and perception origin of an agent. A filler (a past position of Self) keeps the camera of that moment.
        auto Prepare(Chain& chain, const Players& players, const ObjectTable& objects, const Origin& origin) const -> ViewAgent
        {
            ViewAgent agent{};
            agent.Source = origin;
            const Player* player{ players.Find(origin.PlayerHandle) };
            if (player) agent.Row = static_cast<std::size_t>(player - players.Rows.data());

            const EyePoint eye{ (!origin.IsPast && player) ? PlayerEye(chain, objects, *player) : EyePoint{} };
            agent.Eye = eye.Valid ? eye : EyePoint{ origin.Camera, true };
            agent.Aim = (!origin.IsPast && player && player->Body) ? UnitOf(player->Body->Aim, origin.Forward) : origin.Forward;
            agent.AimRange = ResolveAimRange(chain, origin.PlayerHandle);

            const PerceptionOrigin perceptionOrigin{ origin.IsPast ? PerceptionOrigin{} : ResolvePerceptionOrigin(chain, origin.BipedHandle) };
            agent.RayPosition = perceptionOrigin.Valid ? perceptionOrigin.Position : origin.Camera;
            agent.Directions = PerceptionDirections(perceptionOrigin.Valid ? perceptionOrigin.Forward : origin.Forward);
            return agent;
        }

        // Copies what each View holds from what the pipeline already computed: Self, the aim and perception hits, and one row per player.
        auto Assemble(const std::vector<ViewAgent>& agents, const Players& players, const ObjectTable& objects, const std::vector<RaycastHit>& aimHits,
            const std::vector<RaycastHit>& perceptionHits, const SightCounts& sight) -> void
        {
            struct Row
            {
                Vec3 Position{};
                Vec3 Velocity{};
                float Facing{};
                bool IsLive{ false };
            };
            std::array<Row, k_Agents> rows{};
            for (std::size_t index{ 0 }; index < players.Rows.size() && index < k_Agents; ++index)
            {
                const Player& player{ players.Rows[index] };
                const auto object{ objects.find(player.BipedHandle) };
                if (!player.Body || object == objects.end()) continue;

                rows[index] = Row{ object->second.WorldOrigin, player.Body->OwnVelocity, std::atan2(object->second.Forward.Y, object->second.Forward.X), true };
            }

            for (std::size_t index{ 0 }; index < agents.size() && index < m_Views16.size(); ++index)
            {
                const ViewAgent& agent{ agents[index] };
                LocalView& view{ m_Views16[index] };
                view = LocalView{};

                view.Head = { static_cast<std::uint32_t>(agent.Row), agent.Source.PlayerHandle, 1, 0 };
                view.Self.Eye = agent.Eye.Position;
                view.Self.Aim = agent.Aim;
                view.Self.Yaw = std::atan2(agent.Aim.Y, agent.Aim.X);
                view.Self.Pitch = std::asin(std::clamp(agent.Aim.Z, -1.0f, 1.0f));
                if (agent.Row < k_Agents && rows[agent.Row].IsLive)
                {
                    view.Self.Feet = rows[agent.Row].Position;
                    view.Self.OwnVelocity = rows[agent.Row].Velocity;
                    view.Self.Crouch = players.Rows[agent.Row].Body->CrouchFraction;
                    view.Self.Counters[0] = players.Rows[agent.Row].Body->TicksAlive;
                }

                const RaycastHit& aim{ aimHits[index] };
                view.Aim = ViewRayRow{ aim.Distance, aim.ObjectHandle, static_cast<std::uint32_t>(aim.SurfaceFlags), static_cast<std::uint8_t>(aim.Kind), 0, 0, 0 };
                for (std::size_t ray{ 0 }; ray < k_PerceptionRayCount; ++ray)
                {
                    const RaycastHit& hit{ perceptionHits[(index * k_PerceptionRayCount) + ray] };
                    view.Rays[ray] = ViewRayRow{ hit.Distance, hit.ObjectHandle, static_cast<std::uint32_t>(hit.SurfaceFlags), static_cast<std::uint8_t>(hit.Kind), 0, 0, 0 };
                }

                for (std::size_t other{ 0 }; other < k_Agents; ++other)
                {
                    const Row& row{ rows[other] };
                    if (!row.IsLive) continue;

                    const bool isSeen{ agent.Row < k_Agents && sight.Seen[agent.Row][other] != 0 };
                    view.Others[other] = ViewOtherRow{ row.Position, row.Velocity, row.Facing, { 1, static_cast<std::uint8_t>(isSeen ? 1 : 0), 0, 0 } };
                }

                m_Sink += static_cast<double>(view.Rays[0].Distance + view.Others[0].Position.X + static_cast<float>(view.Head[0]));
            }
        }

        SceneStream m_Stream{};
        std::array<LocalView, k_Agents> m_Views16{};
        double m_Sink{};
        std::size_t m_LocalAimTargetChanged{};
        std::size_t m_LocalAimRays{}, m_LocalAimChanged{}, m_OtherAimRays{}, m_OtherAimChanged{};
        Series m_Exclusion{}, m_Prepare{}, m_Scene{}, m_Aim{}, m_AimToday{}, m_Perception{}, m_SightUs{}, m_SightRay{}, m_Views{}, m_Total{};
        Series m_Alive{}, m_Pairs{}, m_Candidates{}, m_Free{}, m_AimAngle{}, m_AimDistance{};
    };

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

            // The scenes of F4.B are armed before anything else: at every decision, and at every tick to see what the cadence changes.
            const bool isDecision{ record % k_SceneEvery == 0 };
            SceneArming armed{};
            if (isDecision) armed = m_Decision.ArmScene(chain);
            m_TickStream.Arm(chain);
            m_Eye.Observe(chain);

            const std::optional<Origin> local{ LocalOrigin(chain) };
            if (!local) return;

            const Origin self{ *local };
            if (record % k_HistoryEvery == 1) m_History.push_back(Origin{ self.Camera, self.Forward, self.PlayerHandle, self.BipedHandle, true });
            if (isDecision) m_Decision.Measure(chain, this->BuildOrigins(chain, self), armed);
            if (isDecision) this->MeasureViewsUpdate(chain);

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
            this->MeasureUpdate(chain, self);
            this->MeasureOneAgent(chain, self);
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
            lines.push_back(TimeLine("M5 profile: 16 aim rays, scene part through the trees (after the static hit)", m_ProfAimTrees));
            lines.push_back(TimeLine("M5 profile: 224 perception rays, scene part through the trees", m_ProfPerceptionTrees));
            lines.push_back(TimeLine("M5 profile: 224 perception rays through the reach, with the static world, no gather", m_ProfPerceptionReach));
            lines.push_back(TimeLine("M5   of rays: 16 gathers of the reach (pieces within 20 of the origin)", m_ProfReach));
            lines.push_back(CountLine("M5 reach: tree nodes visited per agent", m_ProfReachNodes));
            lines.push_back(CountLine("M5 reach: pieces kept per agent", m_ProfReachPieces));
            lines.push_back(CountLine("M5 profile: instance tree nodes per ray", m_ProfTreeNodes));
            lines.push_back(CountLine("M5 profile: mesh nodes per ray", m_ProfMeshNodes));
            lines.push_back(CountLine("M5 profile: pieces reached by the leaves per ray", m_ProfPieces));
            lines.push_back(CountLine("M5 profile: pieces past exclusion, filter and sphere per ray", m_ProfNear));
            lines.push_back(CountLine("M5 profile: pieces whose mesh is entered per ray", m_ProfEntered));
            lines.push_back(TimeLine("M5 profile: 240 rays, static world only", m_ProfStatic));
            lines.push_back(TimeLine("M5 profile: 240 rays, FIXED tree only (after the static hit)", m_ProfFixed));
            lines.push_back(TimeLine("M5 profile: 240 rays, MOBILE tree only (after the static hit)", m_ProfMobile));
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
            for (const std::string& line : this->MobileGroupLines()) lines.push_back(line);
            return lines;
        }

        // return: The rule of the design (2.4) applied to the mean TOTAL of a decision of this film.
        auto VerdictLine(std::string_view name) const -> std::string
        {
            const double limit{ name == "alien8v8" ? 2.9 : 2.2 };
            const double mean{ m_Decision.TotalMeanMs() };
            return std::format("M6 rule of the design (2.4): mean TOTAL per decision {:.3f} ms against {:.1f} ms: {}.", mean, limit, mean <= limit ? "wire RS.2" : "RK.2 first");
        }

        // return: The rule of the design (2.4) applied to the mean of the real ViewsService::Update of 16 agents (the limit D322 puts on the TOTAL).
        auto RealVerdictLine(std::string_view name) const -> std::string
        {
            const double limit{ name == "alien8v8" ? 2.9 : 2.2 };
            const double mean{ m_ViewsReal.Summary().Mean / k_NanosPerMicro };
            return std::format("M6 REAL ViewsService::Update with 16 agents: mean {:.3f} ms against {:.1f} ms: {}.", mean, limit, mean <= limit ? "within the limit" : "OVER the limit");
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
            lines.push_back("M0 ViewsService::Update (1 agent: the local player), every record, us per tick");
            time("M0 Update", m_Update);
            lines.push_back("");
            lines.push_back("M1 pieces of Update that do not depend on the scene, every record, us per tick");
            time("M1 exclusions (pass over the graph)", m_Exclusion);
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
            for (const std::string& line : m_Decision.Lines()) lines.push_back(line);
            lines.push_back(TimeLine("M6 REAL ViewsService::Update (16 agents, HumanLike: rays, sight, Others, Flags)", m_ViewsReal));
            lines.push_back(std::format("M6 REAL against the M6 TOTAL (sight included): {:.3f} ms against {:.3f} ms, {:+.1f}%. Without sight the M6 TOTAL is {:.3f} ms.",
                m_ViewsReal.Summary().Mean / k_NanosPerMicro, m_Decision.TotalMeanMs(), 100.0 * (m_ViewsReal.Summary().Mean / k_NanosPerMicro / m_Decision.TotalMeanMs() - 1.0),
                m_Decision.TotalWithoutSightMs()));
            lines.push_back("M6 REAL sight (the pass of ViewsService::Update, F4.3): candidate pairs, rays and ms (HumanLike Update minus Full Update, which casts the same rays and fills the same rows but has no pass)");
            lines.push_back(CountLine("M6 REAL sight: candidate pairs", m_RealCandidates));
            lines.push_back(CountLine("M6 REAL sight: rays", m_RealSightRays));
            lines.push_back(TimeLine("M6 REAL Update in Full mode (no sight pass)", m_ViewsRealFull));
            lines.push_back(std::format("M6 REAL sight: {:.1f} us per decision (mean).", m_ViewsReal.Summary().Mean - m_ViewsRealFull.Summary().Mean));
            lines.push_back(std::format("M6 sight exactness against the reference scene (every object gathered without reuse, all in MOBILE): {} rays compared, {} differences.",
                m_SightCompared, m_SightDifferences));
            for (const std::string& line : m_TickStream.Lines("M6 scene at one gather per tick")) lines.push_back(line);
            lines.push_back("");
            for (const std::string& line : m_Eye.Lines()) lines.push_back(line);

            lines.push_back("");
            lines.push_back("Heaviest, 1 agent (ms per tick, mean):");
            this->AppendHeaviest(lines, { { "exclusions (graph)", &m_Exclusion }, { "static rays (15)", &m_StaticTick }, { "Update (M0)", &m_Update } });
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
        Series m_ProfAimTrees{}, m_ProfPerceptionTrees{}, m_ProfPerceptionReach{};
        Series m_ProfReach{}, m_ProfReachNodes{}, m_ProfReachPieces{};
        Series m_ProfTreeNodes{}, m_ProfMeshNodes{}, m_ProfPieces{}, m_ProfNear{}, m_ProfEntered{}, m_ProfStatic{}, m_ProfFixed{}, m_ProfMobile{};
        Series m_SceneNodesRay{}, m_SceneTrianglesRay{}, m_SceneObjectsSeries{}, m_FixedLeaves{}, m_MobileLeaves{};
        Series m_RestObjects{}, m_Filtered{}, m_Pending{};
        DecisionBench m_Decision{};
        Series m_ViewsReal{}, m_ViewsRealFull{}, m_RealCandidates{}, m_RealSightRays{};
        SceneStream m_TickStream{};
        EyeProbe m_Eye{};
        std::size_t m_SightCompared{};
        std::size_t m_SightDifferences{};

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

        // Times ViewsService::Update with every player an agent, as the pipeline calls it (F4.2): scene, exclusions, aim and rays of 16 agents.
        // note: The chain is the one of this pass and Update only publishes again the Views the pipeline just made.
        auto MeasureViewsUpdate(Chain& chain) -> void
        {
            const auto start{ std::chrono::steady_clock::now() };
            chain.m_Egocentric.m_ViewsService.Update(Views::View::Type::AgentSet::All());
            const auto end{ std::chrono::steady_clock::now() };
            m_ViewsReal.Add(std::chrono::duration<double, std::micro>(end - start).count());

            // What the sight pass of that Update did, and the same Update without the pass: the difference is its cost.
            const RaycastScene& scene{ chain.m_Egocentric.m_RaycastSceneStore.Scene() };
            m_RealCandidates.Add(static_cast<double>(scene.SightCandidates));
            m_RealSightRays.Add(static_cast<double>(scene.SightRays));

            const auto fullStart{ std::chrono::steady_clock::now() };
            chain.m_Egocentric.m_ViewsService.Update(Views::View::Type::AgentSet{ .Mask = 0xFFFFFFFFu, .Mode = Views::View::Type::ViewMode::Full });
            const auto fullEnd{ std::chrono::steady_clock::now() };
            m_ViewsRealFull.Add(std::chrono::duration<double, std::micro>(fullEnd - fullStart).count());
        }

        // M0: ViewsService::Update with only the local player as an agent.
        auto MeasureUpdate(Chain& chain, const Origin& self) -> void
        {
            Views::View::Type::AgentSet agents{};
            agents.Add(self.PlayerHandle);
            const auto start{ steady_clock::now() };
            chain.m_Egocentric.m_ViewsService.Update(agents);
            m_Update.Add(Micros(start, steady_clock::now()));
        }

        // M1: the pieces of Update that do not depend on the scene: the exclusion, the agent values and the static rays.
        auto MeasureOneAgent(Chain& chain, const Origin& self) -> void
        {
            const auto& raycaster{ chain.m_Resolved.m_Raycaster };

            auto start{ steady_clock::now() };
            const SelfExclusion exclusion{ ExclusionOf(chain, self.PlayerHandle) };
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
        // Profile of M5 (RK.1): the 240 rays timed against the static world alone, then against each instance tree alone starting from the static hit.
        auto MeasureParts(Chain& chain, const std::vector<Agent>& agents) -> void
        {
            const auto& raycaster{ chain.m_Resolved.m_Raycaster };
            std::vector<Ray> rays{};
            std::vector<const SelfExclusion*> exclusions{};
            for (const Agent& agent : agents)
            {
                for (const Ray& ray : RaysOf(agent))
                {
                    rays.push_back(ray);
                    exclusions.push_back(&agent.Exclusion);
                }
            }

            std::vector<RaycastHit> statics{};
            statics.reserve(rays.size());
            auto start{ steady_clock::now() };
            for (const Ray& ray : rays) statics.push_back(CastStatic(raycaster, ray.Origin, ray.Direction, ray.Range));
            m_ProfStatic.Add(Micros(start, steady_clock::now()));

            // Scene part of the aim rays and of the perception rays apart, through the trees and, for the perception ones, through the reach.
            std::vector<SceneReach> reaches{};
            for (const Agent& agent : agents) reaches.push_back(GatherSceneReach(m_Scene, agent.Exclusion, agent.RayPosition, k_PerceptionRange, nullptr));
            double aimTrees{ 0.0 };
            double perceptionTrees{ 0.0 };
            double perceptionReach{ 0.0 };
            for (std::size_t index{ 0 }; index < rays.size(); ++index)
            {
                const bool isAim{ index % k_RaysPerAgent == 0 };
                auto begin{ steady_clock::now() };
                RaycastHit best{ statics[index] };
                CastScene(m_Scene, *exclusions[index], rays[index].Origin, rays[index].Direction, rays[index].Range, best, nullptr);
                (isAim ? aimTrees : perceptionTrees) += Micros(begin, steady_clock::now());
                m_Sink += best.Distance;

                if (isAim) continue;
                begin = steady_clock::now();
                const RaycastHit viaReach{ CastSceneRayReach(raycaster, m_Scene, reaches[index / k_RaysPerAgent], *exclusions[index], rays[index].Origin, rays[index].Direction, rays[index].Range, nullptr) };
                perceptionReach += Micros(begin, steady_clock::now());
                m_Sink += viaReach.Distance;
            }

            m_ProfAimTrees.Add(aimTrees);
            m_ProfPerceptionTrees.Add(perceptionTrees);
            m_ProfPerceptionReach.Add(perceptionReach);

            RaycastScene fixedOnly{};
            fixedOnly.Fixed = m_Scene.Fixed;
            fixedOnly.FixedFilter = m_Scene.FixedFilter;
            RaycastScene mobileOnly{};
            mobileOnly.Mobile = m_Scene.Mobile;

            for (int which = 0; which < 2; ++which)
            {
                const RaycastScene& scene{ which == 0 ? fixedOnly : mobileOnly };
                start = steady_clock::now();
                for (std::size_t index{ 0 }; index < rays.size(); ++index)
                {
                    RaycastHit best{ statics[index] };
                    CastScene(scene, *exclusions[index], rays[index].Origin, rays[index].Direction, rays[index].Range, best, nullptr);
                    m_Sink += best.Distance;
                }
                (which == 0 ? m_ProfFixed : m_ProfMobile).Add(Micros(start, steady_clock::now()));
            }
        }

        // The sight rays of this tick through the scene with rest against the reference scene: the same hits.
        auto CheckSight(Chain& chain) -> void
        {
            const auto players{ chain.m_Environment.m_PlayersStore.Acquire() };
            const auto entities{ chain.m_Environment.m_EntitiesStore.Acquire() };
            const auto nodes{ chain.m_Environment.m_ObjectGraphStore.Acquire() };
            const auto objects{ chain.m_Tables.m_ObjectStore.Acquire() };
            if (!players || !entities || !nodes || !objects) return;

            const PlayerExclusions graph{ BuildGraphExclusions(*players, *entities, *nodes) };
            const std::vector<SightPoint> points{ BuildSightPoints(chain, *players, *objects) };
            std::vector<RaycastHit> hits{};
            std::vector<RaycastHit> reference{};
            SightCounts counts{};
            SightCounts referenceCounts{};
            RunSight(chain.m_Resolved.m_Raycaster, m_Scene, points, graph, counts, &hits);
            RunSight(chain.m_Resolved.m_Raycaster, m_FullScene, points, graph, referenceCounts, &reference);

            m_SightCompared += reference.size();
            if (hits.size() != reference.size())
            {
                ++m_SightDifferences;
                return;
            }

            for (std::size_t index{ 0 }; index < hits.size(); ++index)
            {
                if (!SameFullHit(hits[index], reference[index])) ++m_SightDifferences;
            }
        }

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
            double reachUs{ 0.0 };
            for (const Agent& agent : agents)
            {
                const auto reachStart{ steady_clock::now() };
                const SceneReach reach{ GatherSceneReach(m_Scene, agent.Exclusion, agent.RayPosition, k_PerceptionRange, nullptr) };
                reachUs += Micros(reachStart, steady_clock::now());
                for (const Ray& ray : RaysOf(agent))
                {
                    m5Hits.push_back(CastSceneRayReach(raycaster, m_Scene, reach, agent.Exclusion, ray.Origin, ray.Direction, ray.Range, nullptr));
                }
            }
            const auto end5{ steady_clock::now() };
            m_ProfReach.Add(reachUs);

            m_M5Total.Add(Micros(total5, end5));
            m_M5Prepare.Add(Micros(total5, prepared5));
            m_GatherReuse.Add(gatherUs);
            m_Mobile.Add(mobileUs);
            m_M5Rays.Add(Micros(armed5, end5));
            m_M5Ray.Add(Micros(armed5, end5) / static_cast<double>(m5Hits.size()));

            this->MeasureParts(chain, agents);

            // The same tick gathered without reuse and all in MOBILE: its own timing, and its hits must be the ones of the scene with rest.
            const auto startAll{ steady_clock::now() };
            const AllObjects all{ GatherAll(chain) };
            m_Gather.Add(Micros(startAll, steady_clock::now()));
            const auto table{ chain.m_Tables.m_ObjectStore.Acquire() };
            if (table) this->MeasureKeyStages(chain, *table);
            PrepareMeshBvhs(m_FullScene, all.Objects, false);
            ArmMobileScene(m_FullScene, all.Objects);
            this->CheckSight(chain);

            // Counted pass and comparison, outside the clocks.
            std::size_t flat{ 0 };
            for (std::size_t agentIndex{ 0 }; agentIndex < agents.size(); ++agentIndex)
            {
                const Agent& agent{ agents[agentIndex] };
                const std::array<Ray, k_RaysPerAgent> rays{ RaysOf(agent) };
                SceneCounters gathered{};
                const SceneReach reach{ GatherSceneReach(m_Scene, agent.Exclusion, agent.RayPosition, k_PerceptionRange, &gathered) };
                m_ProfReachNodes.Add(static_cast<double>(gathered.TreeNodes));
                m_ProfReachPieces.Add(static_cast<double>(reach.Pieces.size()));
                for (std::size_t index{ 0 }; index < rays.size(); ++index, ++flat)
                {
                    SceneCounters counters{};
                    const Ray& ray{ rays[index] };
                    m_Sink += CastSceneRayReach(raycaster, m_Scene, reach, agent.Exclusion, ray.Origin, ray.Direction, ray.Range, &counters).Distance;
                    m_SceneNodesRay.Add(static_cast<double>(counters.Nodes));
                    m_SceneTrianglesRay.Add(static_cast<double>(counters.Triangles));
                    m_ProfTreeNodes.Add(static_cast<double>(counters.TreeNodes));
                    m_ProfMeshNodes.Add(static_cast<double>(counters.Nodes - counters.TreeNodes));
                    m_ProfPieces.Add(static_cast<double>(counters.Pieces));
                    m_ProfNear.Add(static_cast<double>(counters.NearPieces));
                    m_ProfEntered.Add(static_cast<double>(counters.EnteredPieces));

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
            const std::optional<Origin> local{ LocalOrigin(chain) };
            if (!local) return;

            const Origin self{ *local };
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

        std::vector<std::string> lines{ bench.Lines() };
        lines.push_back(bench.VerdictLine(name));
        lines.push_back(bench.RealVerdictLine(name));
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