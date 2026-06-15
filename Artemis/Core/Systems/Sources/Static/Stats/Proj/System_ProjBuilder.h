#pragma once

#include <cstdint>

enum class ProjectileType : uint8_t;
struct Proj;
struct ProjObject;

class System_Logs;

struct Sys_ProjBuilder_Deps
{ 
	System_Logs& System_Logs;
};

class System_ProjBuilder
{
public:
	System_ProjBuilder(Sys_ProjBuilder_Deps deps) : m_Deps(deps) {}
	~System_ProjBuilder() = default;

	Proj Build(const ProjObject& proj);

private:
	Sys_ProjBuilder_Deps m_Deps;

	ProjectileType DeriveType(const ProjObject& proj);
	bool DeriveCanBounce(const ProjObject& proj);
};