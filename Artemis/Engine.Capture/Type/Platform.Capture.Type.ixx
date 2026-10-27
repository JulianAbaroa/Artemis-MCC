export module Platform.Capture.Type;

import std;

export namespace Platform::Capture::Type
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
}