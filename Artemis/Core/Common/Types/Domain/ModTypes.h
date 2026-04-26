#pragma once

// Represents the current lifecycle state of the Blam! (Halo Reach) game engine.
enum class EngineStatus
{
	// The engine it has either not been initialized yet or has been fuly deallocated.
	Waiting,		

	// The engine is active and currently executing game logic.
	Running,

	// The engine instance has been torn down or destroyed.
	Destroyed,
};







