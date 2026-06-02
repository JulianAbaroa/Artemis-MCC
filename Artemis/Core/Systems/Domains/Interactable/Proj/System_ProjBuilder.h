#pragma once

#include <cstdint>

enum class ProjectileType : uint8_t;
struct ProjectileData;
struct ProjObject;

class System_ProjBuilder
{
public:
	ProjectileData Build(const ProjObject& proj);

private:
	ProjectileType DeriveType(const ProjObject& proj);
	bool DeriveCanBounce(const ProjObject& proj);
};