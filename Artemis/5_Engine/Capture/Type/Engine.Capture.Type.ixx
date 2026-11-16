export module Engine.Capture.Type;

import std;

export namespace Engine::Capture::Type
{
    // Engine function that runs the logic ticks of one frame (FUN_180057434).
    // note: remainingTime is the frame time left after the ticks. The engine lowers it if it stops before ticksToAdvance.
    using SimulationTicksFunction = auto(__fastcall*)(int ticksToAdvance, float* remainingTime) -> void;

    // Engine function that runs one logic tick (FUN_1800575AC). SimulationTicks calls it once per tick.
    using ExecuteGameFrameFunction = auto(__fastcall*)() -> void;

    // Engine function that ends the round (FUN_18007b3ec, D144).
    // param winner: Team or player index, or k_EndRoundNoWinner to let the scores decide.
    // param isGameEnd: Ends the match too. The engine also ends it when the round limit is reached.
    // param reason: Why the round ends, read by the engine for the winner and the end of the match.
    using EndRoundFunction = auto(__fastcall*)(std::int32_t winner, bool isGameEnd, std::int32_t reason) -> void;

    // Ticks the queue between the game thread and the AI thread holds (D305). One decision cycle is N = 4 ticks (D114), so the AI thread
    // can be one whole cycle behind (a decision tick of 3.5 ms next to ticks of 1.25 ms) before the game thread has to wait for room.
    inline constexpr std::size_t k_TickQueueDepth{ 4 };

    // Longest time the game thread waits for room in the tick queue. Past it the tick is not queued and counts as lost, so a stuck AI
    // thread slows the game down but never freezes it.
    inline constexpr std::chrono::milliseconds k_ReadTimeout{ 100 };

    // What the game thread spent on one tick it handed to the AI thread.
    struct ProducedTick
    {
        // Time copying the game into a Snapshot (the clock of the bridge included), in nanoseconds.
        std::uint64_t CaptureNs{ 0 };
        // Time waiting for room in the queue, in nanoseconds.
        std::uint64_t QueueWaitNs{ 0 };
        // Entries in the queue right after the push.
        std::size_t QueueDepth{ 0 };
        // The queue stayed full for k_ReadTimeout: the tick was not queued and is lost.
        bool IsLost{ false };
    };

    // The tick hooks hand every completed tick to it (D305). Implemented by the owner (Mod): it reads the bridge clock, copies the game with
    // CaptureService and queues the result for the AI thread.
    class TickProducer
    {
    public:
        virtual ~TickProducer() = default;

        // Copies the tick that just completed and queues it, if the AI thread is consuming ticks and has the map resources loaded.
        // return: What it cost the game thread. All zero when nothing was captured.
        // note: Game thread only, between two ticks (the engine writes nothing then, so the copy is stable). May wait for room in the queue,
        // at most k_ReadTimeout.
        virtual auto Produce() -> ProducedTick = 0;
    };
}