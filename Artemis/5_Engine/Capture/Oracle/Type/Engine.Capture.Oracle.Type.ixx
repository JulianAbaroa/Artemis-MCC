export module Engine.Capture.Oracle.Type;

import Common.Engine.Type;
import Blam.Layout.Collision.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Blam::Layout::Collision::Type::Result::CollisionResult;
}

export namespace Engine::Capture::Oracle::Type
{
    // How many outcomes wait for the AI thread before the oldest is dropped.
    inline constexpr std::size_t k_MaxPendingOutcomes{ 4 };

    // Longest vector (direction times range) the game thread passes to the engine, in meters. A longer one is skipped.
    inline constexpr float k_MaxVectorLength{ 1000.0f };

    // Engine function collision_test_vector (rva 0x12969c, A20): the ray test the bullets use.
    // param flags: What to test, see Blam.Layout.Collision.Type.
    // param keepLocation: Leave the point of the impact in a valid location. The client of the game passes false.
    // param point: Origin of the ray, three floats.
    // param vector: Direction times length, three floats. The fraction of the result is a fraction of its length.
    // param ignoreA: Handle of an object to ignore, or no handle. ignoreB and ignoreC are two more.
    // param result: Buffer of k_ResultSize bytes the engine fills.
    // return: Whether something was hit.
    using CollisionTestVectorFunction = auto(__fastcall*)(std::uint64_t flags, bool keepLocation, const float* point, const float* vector,
        std::uint32_t ignoreA, std::uint32_t ignoreB, std::uint32_t ignoreC, void* result) -> bool;

    // Whether the oracle is running (the status of the store).
    enum class OracleStatus : std::uint8_t
    {
        Off = 0,
        Waiting = 1,
        Running = 2,
        Unavailable = 3,
    };

    // One ray the game thread traces: its origin and the vector it travels (direction times range).
    struct OracleRay
    {
        std::array<float, 3> Origin{};
        std::array<float, 3> Vector{};

        // Handle of the object the ray ignores in the test with objects (the root object of the player), or no handle.
        std::uint32_t IgnoreHandle{ k_NoHandle };
    };

    // The rays the AI thread asks the game thread to trace.
    struct OracleBatch
    {
        std::uint64_t Id{ 0 };

        // The game thread sequence (ticks seen while the oracle is on) when the AI thread handed the batch over.
        std::uint64_t SequenceAtSubmit{ 0 };
        std::vector<OracleRay> Rays{};
    };

    // The two engine answers for one ray and what each call cost.
    struct OracleProbe
    {
        // The ray was not traced: it had a value that is not finite, no length or more than k_MaxVectorLength.
        bool IsSkipped{ false };

        // The test with the structure and the instanced geometry only.
        CollisionResult Static{};

        // The same test with every object type.
        CollisionResult Objects{};
        std::uint32_t StaticNanoseconds{ 0 };
        std::uint32_t ObjectsNanoseconds{ 0 };
    };

    // What the game thread traced for a batch. Probes[i] answers Rays[i].
    struct OracleOutcome
    {
        std::uint64_t BatchId{ 0 };
        std::uint64_t SequenceAtSubmit{ 0 };

        // The game thread sequence when the rays were traced. The difference with SequenceAtSubmit is the lag in ticks.
        std::uint64_t SequenceAtRun{ 0 };
        std::vector<OracleProbe> Probes{};
    };
}