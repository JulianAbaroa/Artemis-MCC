export module DevTools.Workbench.Oracle.Type;

import Common.Math.Type;
import Blam.MapModel.World.Type;
import Views.Oracle.Type;
import Views.Raycast.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Blam::MapModel::World::Type::Raycast::Hit;
    using Views::Oracle::Type::Sample::RaySource;
    using Views::Raycast::Type::RaycastHit;
}

export namespace DevTools::Workbench::Oracle::Type
{
    inline constexpr int k_DefaultRays{ 16 };
    inline constexpr int k_MaxRays{ 64 };
    inline constexpr int k_DefaultEveryTicks{ 1 };
    inline constexpr int k_MaxEveryTicks{ 600 };

    // Seed of the random rays, fixed so two runs trace the same directions.
    inline constexpr std::uint32_t k_RandomSeed{ 0x52334F52u };

    // How often the summary goes to the log and the report file is written again.
    inline constexpr std::chrono::seconds k_SummaryInterval{ 10 };

    // Batches the AI thread remembers while the game thread has not traced them. Older ones are forgotten.
    inline constexpr std::size_t k_MaxPendingBatches{ 4 };

    // Folder of the report files inside the storage folder.
    inline constexpr std::string_view k_ReportFolder{ "RaycastOracle" };

    // The points of the oracle that log once after a start.
    enum class Gate : std::uint32_t
    {
        NoLocalPlayer = 1,
        EmptyBatch = 2,
        FirstBatch = 4,
        FirstOutcome = 8,
    };

    // A ray handed to the game thread, with what Artemis answered for it when it was handed over.
    struct PendingRay
    {
        RaySource Source{ RaySource::Tick };
        Vec3 Origin{};
        Vec3 Direction{};
        float MaxDistance{ 0.0f };

        // Artemis against the static world, for this same ray.
        Hit OursStatic{};

        // The ray as the tick published it, for the rays of the tick.
        std::optional<RaycastHit> OursTick{};
    };

    // The rays of one batch, in the order of the batch the game thread traces.
    struct PendingBatch
    {
        std::uint64_t Id{ 0 };
        std::vector<PendingRay> Rays{};
    };
}