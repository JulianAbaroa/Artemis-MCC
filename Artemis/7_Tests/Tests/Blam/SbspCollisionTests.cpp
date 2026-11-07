#include <gtest/gtest.h>

import Common.Log.Type;
import Common.Log.System;
import Common.Math.Type;
import Common.Math.System;
import Common.Geometry.System;
import Blam.Map.Reader.Type;
import Blam.Map.Layer;
import Blam.MapModel.Layer;
import Blam.MapModel.Definitions.Type;
import Tests.Support.Log;
import Tests.Support.Maps;
import std;

// Collision of the structure bsps (R1.A, D272): the surfaces of the collision resource of each sbsp and of its instanced geometry,
// read from forge_halo.map as the AIThread does on load. The sweep over every map of the folder is DISABLED_: run it with
// Tools/test.py --disabled --filter "DISABLED_CollisionSweep.*". It writes build/bench/coll/sweep.txt.

namespace
{
    using Common::Log::Type::LogLevel;
    using Common::Log::System::Log;
    using Common::Math::Type::Triangle;
    using Common::Math::Type::Vec3;
    using Common::Math::System::Cross;
    using Common::Math::System::Length;
    using Common::Math::System::Subtract;
    using Common::Math::System::TransformPoint3x4;
    using Common::Geometry::System::AppendCollisionSurface;
    using Blam::Map::Reader::Type::Geometry::SbspCollision;
    using Blam::MapModel::Definitions::Type::Sbsp::Sbsp;
    using Tests::Support::Log::CollectingSink;
    using Tests::Support::Maps::LocatedFile;
    using Tests::Support::Maps::Locate;
    using Tests::Support::Maps::MapsFolder;
    using Tests::Support::Maps::k_ForgeHalo;
    using Tests::Support::Maps::k_Shared;

    // Meters that a vertex may lie outside the world bounds of its bsp.
    constexpr float k_BoundsMargin{ 1.0f };

    // Smallest accepted twice the area of a triangle, in square meters. The reader drops what is below.
    constexpr float k_MinDoubleArea{ 1e-8f };

    constexpr std::size_t k_FlagCount{ 8 };

    constexpr std::array<std::string_view, k_FlagCount> k_FlagNames{ "TwoSided", "Invisible", "Climbable", "Breakable", "Invalid", "Conveyor", "Slip", "PlaneNegated" };

    struct Box
    {
        Vec3 Min{ std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
        Vec3 Max{ std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };

        auto Add(const Vec3& point) -> void
        {
            Min = { (std::min)(Min.X, point.X), (std::min)(Min.Y, point.Y), (std::min)(Min.Z, point.Z) };
            Max = { (std::max)(Max.X, point.X), (std::max)(Max.Y, point.Y), (std::max)(Max.Z, point.Z) };
        }
    };

    auto IsFinite(const Vec3& point) -> bool
    {
        return std::isfinite(point.X) && std::isfinite(point.Y) && std::isfinite(point.Z);
    }

    // return: Meters that the point lies outside the box, 0 if inside.
    auto Excess(const Vec3& point, const Vec3& min, const Vec3& max) -> float
    {
        return (std::max)({ min.X - point.X, point.X - max.X, min.Y - point.Y, point.Y - max.Y, min.Z - point.Z, point.Z - max.Z, 0.0f });
    }

    auto BoxOf(const std::vector<Triangle>& triangles) -> Box
    {
        Box box{};
        for (const Triangle& triangle : triangles)
        {
            box.Add(triangle.A);
            box.Add(triangle.B);
            box.Add(triangle.C);
        }
        return box;
    }

    auto DoubleArea(const Triangle& triangle) -> float
    {
        return Length(Cross(Subtract(triangle.B, triangle.A), Subtract(triangle.C, triangle.A)));
    }

    auto OutFolder() -> std::filesystem::path
    {
        // This file is Artemis/7_Tests/Tests/Blam/SbspCollisionTests.cpp.
        const std::filesystem::path source{ std::source_location::current().file_name() };
        const std::filesystem::path folder{ source.parent_path().parent_path().parent_path().parent_path().parent_path() / "build" / "bench" / "coll" };
        std::error_code error{};
        std::filesystem::create_directories(folder, error);
        return folder;
    }

    // Elements shaped as AppendCollisionSurface reads them.
    struct TestSurface
    {
        std::int32_t FirstEdge{};
        std::uint8_t Flags{};
        std::int16_t Material{};
    };

    struct TestEdge
    {
        std::int32_t StartVertex{};
        std::int32_t EndVertex{};
        std::int32_t ForwardEdge{};
        std::int32_t ReverseEdge{};
        std::int32_t LeftSurface{};
    };

    struct TestVertex
    {
        Vec3 Point{};
    };

    // The map and its model, loaded once for the whole suite in the order of AIThread::LoadResources.
    class LoadedMap
    {
    public:
        LoadedMap() = default;
        ~LoadedMap() = default;

        CollectingSink m_Sink{};
        Log m_Log{ m_Sink };
        Blam::Map::Layer m_Map{ m_Log };
        Blam::MapModel::Layer m_Resolved{ m_Log, m_Map };
    };

    // Per-flag counts of the triangles.
    auto CountFlags(const std::vector<Triangle>& triangles) -> std::array<std::size_t, k_FlagCount>
    {
        std::array<std::size_t, k_FlagCount> counts{};
        for (const Triangle& triangle : triangles)
        {
            for (std::size_t bit = 0; bit < k_FlagCount; ++bit)
            {
                if (triangle.SurfaceFlags & (1u << bit)) ++counts[bit];
            }
        }
        return counts;
    }

    auto FlagText(const std::array<std::size_t, k_FlagCount>& counts) -> std::string
    {
        std::string text{};
        for (std::size_t bit = 0; bit < k_FlagCount; ++bit)
        {
            text += std::format("{}={} ", k_FlagNames[bit], counts[bit]);
        }
        return text;
    }
}

TEST(CollisionSurfaceTests, QuadBecomesTwoTrianglesWithFlagsAndMaterial)
{
    // Unit square in the XY plane, one surface whose ring is the four edges, the other side of each edge is surface 1.
    const std::vector<TestVertex> vertices{ { { 0, 0, 0 } }, { { 1, 0, 0 } }, { { 1, 1, 0 } }, { { 0, 1, 0 } } };
    const std::vector<TestEdge> edges{
        { .StartVertex = 0, .EndVertex = 1, .ForwardEdge = 1, .ReverseEdge = 3, .LeftSurface = 0 },
        { .StartVertex = 1, .EndVertex = 2, .ForwardEdge = 2, .ReverseEdge = 0, .LeftSurface = 0 },
        { .StartVertex = 2, .EndVertex = 3, .ForwardEdge = 3, .ReverseEdge = 1, .LeftSurface = 0 },
        { .StartVertex = 3, .EndVertex = 0, .ForwardEdge = 0, .ReverseEdge = 2, .LeftSurface = 0 } };
    const std::vector<TestSurface> surfaces{ { .FirstEdge = 0, .Flags = 0x05, .Material = 7 } };

    std::vector<Triangle> triangles{};
    EXPECT_EQ(AppendCollisionSurface(surfaces, edges, vertices, 0, triangles), 2u);
    ASSERT_EQ(triangles.size(), 2u);
    for (const Triangle& triangle : triangles)
    {
        EXPECT_EQ(triangle.SurfaceFlags, 0x05);
        EXPECT_EQ(triangle.Material, 7);
        EXPECT_FLOAT_EQ(DoubleArea(triangle), 1.0f);
    }
}

TEST(CollisionSurfaceTests, OpenRingAndBadIndicesAddNothing)
{
    const std::vector<TestVertex> vertices{ { { 0, 0, 0 } }, { { 1, 0, 0 } }, { { 1, 1, 0 } } };
    // The ring never returns to the first edge.
    const std::vector<TestEdge> open{
        { .StartVertex = 0, .EndVertex = 1, .ForwardEdge = 1, .ReverseEdge = 0, .LeftSurface = 0 },
        { .StartVertex = 1, .EndVertex = 2, .ForwardEdge = 1, .ReverseEdge = 0, .LeftSurface = 0 } };
    const std::vector<TestEdge> outOfRange{ { .StartVertex = 0, .EndVertex = 9, .ForwardEdge = 0, .ReverseEdge = 0, .LeftSurface = 0 } };
    const std::vector<TestSurface> surfaces{ { .FirstEdge = 0 }, { .FirstEdge = 5 } };

    std::vector<Triangle> triangles{};
    EXPECT_EQ(AppendCollisionSurface(surfaces, open, vertices, 0, triangles), 0u);
    EXPECT_EQ(AppendCollisionSurface(surfaces, outOfRange, vertices, 0, triangles), 0u);
    EXPECT_EQ(AppendCollisionSurface(surfaces, open, vertices, 1, triangles), 0u);
    EXPECT_EQ(AppendCollisionSurface(surfaces, open, vertices, 2, triangles), 0u);
    EXPECT_TRUE(triangles.empty());
}

TEST(CollisionSurfaceTests, TransformPointAppliesRotationRowsThenTranslation)
{
    // 90 degrees about Z in row vector form, then a move of (1, 2, 3).
    const std::array<float, 12> matrix{ 0, 1, 0, -1, 0, 0, 0, 0, 1, 1, 2, 3 };
    const Vec3 moved{ TransformPoint3x4({ 2, 0, 0 }, matrix.data()) };
    EXPECT_FLOAT_EQ(moved.X, 1.0f);
    EXPECT_FLOAT_EQ(moved.Y, 4.0f);
    EXPECT_FLOAT_EQ(moved.Z, 3.0f);
}

class SbspCollisionTests : public ::testing::Test
{
protected:
    static auto SetUpTestSuite() -> void
    {
        const LocatedFile forge{ Locate(k_ForgeHalo) };
        const LocatedFile shared{ Locate(k_Shared) };
        if (forge.Path.empty() || shared.Path.empty())
        {
            s_SkipReason = forge.Path.empty() ? forge.Reason : shared.Reason;
            return;
        }

        s_Loaded = std::make_unique<LoadedMap>();
        auto& loaded = *s_Loaded;

        loaded.m_Map.m_MapLoaderService.OpenFile(shared.Path.string());
        loaded.m_Map.m_MapLoaderService.OpenFile((forge.Path.parent_path() / "campaign.map").string());
        loaded.m_Map.m_MapLoaderService.OpenFile(forge.Path.string());

        loaded.m_Map.m_MapBuilderService.LoadForMap();
        loaded.m_Resolved.m_DefinitionsBuilder.BuildForMap();

        std::vector<std::string> names{};
        for (const auto& [name, sbsp] : loaded.m_Map.m_TagCatalog.Sbsp.All()) names.push_back(name);

        const auto start{ std::chrono::steady_clock::now() };
        s_Collisions = loaded.m_Map.m_GeometryCollisionService.ReadCollision(names);
        s_ReadSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    }

    static auto TearDownTestSuite() -> void
    {
        s_Collisions.clear();
        s_Loaded.reset();
    }

    auto SetUp() -> void override
    {
        if (!s_Loaded) GTEST_SKIP() << s_SkipReason;
    }

    static inline std::unique_ptr<LoadedMap> s_Loaded{};
    static inline std::vector<SbspCollision> s_Collisions{};
    static inline double s_ReadSeconds{};
    static inline std::string s_SkipReason{};
};

TEST_F(SbspCollisionTests, EverySbspHasCollision)
{
    const auto& sbsps = s_Loaded->m_Resolved.m_DefinitionsStore.m_Sbsp.All();
    ASSERT_FALSE(sbsps.empty());

    for (const auto& [name, sbsp] : sbsps)
    {
        std::cout << std::format("[ Collision ] {}: {} collision triangles, {} render triangles.\n", name, sbsp.CollisionGeometry.size(), sbsp.RenderGeometry.size());
        EXPECT_FALSE(sbsp.CollisionGeometry.empty()) << name;
    }
}

TEST_F(SbspCollisionTests, ReaderTrianglesPerSbspAndPerPlacement)
{
    ASSERT_EQ(s_Collisions.size(), s_Loaded->m_Map.m_TagCatalog.Sbsp.All().size());

    for (const SbspCollision& collision : s_Collisions)
    {
        std::size_t placed{};
        std::size_t largestDefinition{};
        for (const auto& instance : collision.Instances)
        {
            const std::size_t count{ collision.Definitions[static_cast<std::size_t>(instance.DefinitionIndex)].size() };
            placed += count;
            largestDefinition = (std::max)(largestDefinition, count);
        }

        std::cout << std::format("[ Collision ] {}: {} structure triangles ({}), {} definitions, {} placements with {} triangles (largest definition {}),"
            " {} open surfaces, {} degenerate dropped.\n", collision.TagName, collision.Structure.size(), collision.IsLarge ? "large" : "normal",
            collision.Definitions.size(), collision.Instances.size(), placed, largestDefinition, collision.OpenSurfaces, collision.DegenerateTriangles);

        EXPECT_FALSE(collision.Structure.empty()) << collision.TagName;
        EXPECT_EQ(collision.UnreadDefinitions, 0u) << collision.TagName;
        EXPECT_EQ(collision.InvalidPlacements, 0u) << collision.TagName;
        EXPECT_EQ(collision.OpenSurfaces, 0u) << collision.TagName;

        const auto& tagInstances = s_Loaded->m_Map.m_TagCatalog.Sbsp.Get(collision.TagName)->InstancedGeometryInstances;
        EXPECT_EQ(collision.Instances.size(), tagInstances.size()) << collision.TagName;
    }
}

TEST_F(SbspCollisionTests, ReadTimeIsReported)
{
    std::cout << std::format("[ Collision ] forge_halo collision read in {:.2f} s.\n", s_ReadSeconds);
    ::testing::Test::RecordProperty("ReadSeconds", std::format("{:.2f}", s_ReadSeconds));
    EXPECT_LT(s_ReadSeconds, 30.0);
}

TEST_F(SbspCollisionTests, VerticesAreFiniteAndTheStructureIsInsideTheWorldBounds)
{
    for (const auto& [name, sbsp] : s_Loaded->m_Resolved.m_DefinitionsStore.m_Sbsp.All())
    {
        std::size_t notFinite{};
        for (const Triangle& triangle : sbsp.CollisionGeometry)
        {
            for (const Vec3* point : { &triangle.A, &triangle.B, &triangle.C }) if (!IsFinite(*point)) ++notFinite;
        }
        EXPECT_EQ(notFinite, 0u) << name;
    }

    // The placed instances may stick out of the bounds of the bsp (props near the edge), the structure may not.
    for (const SbspCollision& collision : s_Collisions)
    {
        const auto& data = s_Loaded->m_Map.m_TagCatalog.Sbsp.Get(collision.TagName)->Data;
        const Vec3 min{ data.WorldBoundsX.Min, data.WorldBoundsY.Min, data.WorldBoundsZ.Min };
        const Vec3 max{ data.WorldBoundsX.Max, data.WorldBoundsY.Max, data.WorldBoundsZ.Max };

        float worstStructure{};
        for (const Triangle& triangle : collision.Structure)
        {
            for (const Vec3* point : { &triangle.A, &triangle.B, &triangle.C }) worstStructure = (std::max)(worstStructure, Excess(*point, min, max));
        }

        float worstInstances{};
        for (const auto& instance : collision.Instances)
        {
            for (const Triangle& triangle : collision.Definitions[static_cast<std::size_t>(instance.DefinitionIndex)])
            {
                for (const Vec3* point : { &triangle.A, &triangle.B, &triangle.C })
                {
                    worstInstances = (std::max)(worstInstances, Excess(TransformPoint3x4(*point, instance.Matrix.data()), min, max));
                }
            }
        }

        std::cout << std::format("[ Collision ] {}: worst excess over the world bounds: structure {:.4f} m, instances {:.4f} m.\n", collision.TagName, worstStructure, worstInstances);
        EXPECT_LE(worstStructure, k_BoundsMargin) << collision.TagName;
    }
}

TEST_F(SbspCollisionTests, SurfaceFlagsAreCountedAndNoTriangleIsDegenerate)
{
    std::array<std::size_t, k_FlagCount> total{};

    for (const auto& [name, sbsp] : s_Loaded->m_Resolved.m_DefinitionsStore.m_Sbsp.All())
    {
        const auto counts{ CountFlags(sbsp.CollisionGeometry) };
        for (std::size_t bit = 0; bit < k_FlagCount; ++bit) total[bit] += counts[bit];

        std::cout << std::format("[ Collision ] {}: flags {}\n", name, FlagText(counts));

        std::size_t degenerate{};
        for (const Triangle& triangle : sbsp.CollisionGeometry)
        {
            if (DoubleArea(triangle) < k_MinDoubleArea * 0.5f) ++degenerate;
        }
        // Instances are transformed after the reader dropped the degenerate ones: a rotation cannot make one.
        EXPECT_EQ(degenerate, 0u) << name;
    }

    std::cout << std::format("[ Collision ] forge_halo flags {}\n", FlagText(total));
    ::testing::Test::RecordProperty("FlagCounts", FlagText(total));
    EXPECT_GT(total[0], 0u) << "no two sided surface";
}

TEST_F(SbspCollisionTests, CollisionBoxMatchesTheRenderBox)
{
    for (const auto& [name, sbsp] : s_Loaded->m_Resolved.m_DefinitionsStore.m_Sbsp.All())
    {
        const Box collision{ BoxOf(sbsp.CollisionGeometry) };
        const Box render{ BoxOf(sbsp.RenderGeometry) };

        const float difference{ (std::max)({ std::abs(collision.Min.X - render.Min.X), std::abs(collision.Min.Y - render.Min.Y), std::abs(collision.Min.Z - render.Min.Z),
            std::abs(collision.Max.X - render.Max.X), std::abs(collision.Max.Y - render.Max.Y), std::abs(collision.Max.Z - render.Max.Z) }) };

        std::cout << std::format("[ Collision ] {}: collision box ({:.2f}, {:.2f}, {:.2f}) to ({:.2f}, {:.2f}, {:.2f}), render box ({:.2f}, {:.2f}, {:.2f}) to ({:.2f}, {:.2f}, {:.2f}), largest difference {:.3f} m.\n",
            name, collision.Min.X, collision.Min.Y, collision.Min.Z, collision.Max.X, collision.Max.Y, collision.Max.Z,
            render.Min.X, render.Min.Y, render.Min.Z, render.Max.X, render.Max.Y, render.Max.Z, difference);
        ::testing::Test::RecordProperty(std::format("BoxDifference_{}", sbsp.TagName.substr(sbsp.TagName.rfind('\\') + 1)), std::format("{:.3f}", difference));
        // The collision reaches the invisible limits of the world, the render stops at the last visible surface.
        if (name.ends_with("forge_halo")) EXPECT_LT(difference, 25.0f) << name;
    }
}

TEST_F(SbspCollisionTests, MaterialsAreGlobalIndicesOrNone)
{
    for (const auto& [name, sbsp] : s_Loaded->m_Resolved.m_DefinitionsStore.m_Sbsp.All())
    {
        std::size_t none{};
        std::int16_t largest{ -1 };
        for (const Triangle& triangle : sbsp.CollisionGeometry)
        {
            EXPECT_GE(triangle.Material, -1) << name;
            if (triangle.Material < 0) ++none;
            largest = (std::max)(largest, triangle.Material);
        }
        std::cout << std::format("[ Collision ] {}: {} triangles without material, largest global material index {}.\n", name, none, largest);
    }
}

TEST(DISABLED_CollisionSweep, AllMaps)
{
    std::vector<std::filesystem::path> maps{};
    for (const auto& entry : std::filesystem::directory_iterator(MapsFolder()))
    {
        if (entry.path().extension() != ".map") continue;
        const std::string stem{ entry.path().stem().string() };
        if (stem == "shared" || stem == "campaign" || stem == "single_player_shared" || stem == "mainmenu") continue;
        maps.push_back(entry.path());
    }
    std::ranges::sort(maps);
    ASSERT_FALSE(maps.empty());

    std::ofstream text{ OutFolder() / "sweep.txt", std::ios::trunc };
    std::size_t sbspCount{};
    std::size_t problems{};

    for (const auto& path : maps)
    {
        const auto start{ std::chrono::steady_clock::now() };

        CollectingSink sink{};
        Log log{ sink };
        Blam::Map::Layer map{ log };
        map.m_MapLoaderService.OpenFile((path.parent_path() / "shared.map").string());
        map.m_MapLoaderService.OpenFile((path.parent_path() / "campaign.map").string());
        map.m_MapLoaderService.OpenFile(path.string());
        map.m_MapBuilderService.LoadForMap();

        std::vector<std::string> names{};
        for (const auto& [name, sbsp] : map.m_TagCatalog.Sbsp.All()) names.push_back(name);
        std::ranges::sort(names);

        const std::size_t before{ sink.m_Messages.size() };
        const std::vector<SbspCollision> collisions{ map.m_GeometryCollisionService.ReadCollision(names) };
        const double seconds{ std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() };

        text << std::format("== {} ({} sbsp, {} read, {:.1f} s with the load)\n", path.filename().string(), names.size(), collisions.size(), seconds);

        for (std::size_t i = before; i < sink.m_Messages.size(); ++i)
        {
            const auto& message{ sink.m_Messages[i] };
            if (message.Level != LogLevel::Info) { text << std::format("   LOG {}\n", message.Message); ++problems; }
        }

        for (const SbspCollision& collision : collisions)
        {
            ++sbspCount;
            const auto* tag{ map.m_TagCatalog.Sbsp.Get(collision.TagName) };

            const Vec3 min{ tag->Data.WorldBoundsX.Min, tag->Data.WorldBoundsY.Min, tag->Data.WorldBoundsZ.Min };
            const Vec3 max{ tag->Data.WorldBoundsX.Max, tag->Data.WorldBoundsY.Max, tag->Data.WorldBoundsZ.Max };

            std::size_t notFinite{};
            float worstStructure{};
            for (const Triangle& triangle : collision.Structure)
            {
                for (const Vec3* point : { &triangle.A, &triangle.B, &triangle.C })
                {
                    if (!IsFinite(*point)) { ++notFinite; continue; }
                    worstStructure = (std::max)(worstStructure, Excess(*point, min, max));
                }
            }

            std::size_t placed{};
            float worstInstances{};
            std::size_t emptyDefinitions{};
            for (const auto& definition : collision.Definitions) if (definition.empty()) ++emptyDefinitions;
            for (const auto& instance : collision.Instances)
            {
                const auto& definition{ collision.Definitions[static_cast<std::size_t>(instance.DefinitionIndex)] };
                placed += definition.size();
                for (const Triangle& triangle : definition)
                {
                    for (const Vec3* point : { &triangle.A, &triangle.B, &triangle.C })
                    {
                        const Vec3 world{ TransformPoint3x4(*point, instance.Matrix.data()) };
                        if (!IsFinite(world)) { ++notFinite; continue; }
                        worstInstances = (std::max)(worstInstances, Excess(world, min, max));
                    }
                }
            }

            std::vector<Triangle> all{ collision.Structure };
            for (const auto& definition : collision.Definitions) all.insert(all.end(), definition.begin(), definition.end());
            const auto flags{ CountFlags(all) };

            text << std::format("  {} {}: structure {} tris, {} defs ({} empty), {} placements, {} placed tris, open {}, degenerate {}, unread {}, invalid {}, notFinite {}, excess structure {:.3f} instances {:.3f}\n      flags(structure+definitions) {}\n",
                collision.IsLarge ? "LARGE" : "normal", collision.TagName, collision.Structure.size(), collision.Definitions.size(), emptyDefinitions,
                collision.Instances.size(), placed, collision.OpenSurfaces, collision.DegenerateTriangles, collision.UnreadDefinitions, collision.InvalidPlacements,
                notFinite, worstStructure, worstInstances, FlagText(flags));

            if (collision.OpenSurfaces || collision.UnreadDefinitions || collision.InvalidPlacements || notFinite || worstStructure > k_BoundsMargin) ++problems;
        }
        text.flush();
    }

    text << std::format("TOTAL {} sbsp, {} problems.\n", sbspCount, problems);
    EXPECT_GT(sbspCount, 0u);
}