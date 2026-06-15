#pragma once

#include "Core/Types/Tick/Tick.h"

#include <cstdint>

class State_Tick;
class State_ObjectTable;
class State_PlayerTable;
class State_InteractionTable;
class State_Classifier;
class State_ObjectGraph;
class State_PlayerGraph;
class State_Collidables;
class State_Fixtures;
class State_Vitality;
class State_Self;
class State_Affordances;

struct Sys_Tick_Deps
{
	State_Tick& State_Tick;
	State_ObjectTable& State_ObjectTable;
	State_PlayerTable& State_PlayerTable;
	State_InteractionTable& State_InteractionTable;
	State_Classifier& State_Classifier;
	State_ObjectGraph& State_ObjectGraph;
	State_PlayerGraph& State_PlayerGraph;
	State_Collidables& State_Collidables;
	State_Fixtures& State_Fixtures;
	State_Vitality& State_Vitality;
	State_Self& State_Self;
	State_Affordances& State_Affordances;
};

class System_Tick
{
public:
	System_Tick(Sys_Tick_Deps deps) : m_Deps(deps) {}
	~System_Tick() = default;

	void Assemble(uint64_t generation);

private:
	Sys_Tick_Deps m_Deps;
};