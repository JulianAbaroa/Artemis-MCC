#pragma once

#include <unordered_map>
#include <vector>
#include <string>

struct ClassifiedObject;
struct LiveObject;
struct WorldRigidBody;
struct ActivePhysicsInstance;

class State_Map;
class State_MapColl;
class State_MapPhmo;
class State_MapMode;
class State_MapScnr;
class State_MapBipd;
class State_MapScen;
class State_ObjectTable;
class State_Classification;
class State_Environment;
class System_CollGeometryBuilder;
class System_PhmoGeometryBuilder;
class System_ModeGeometryBuilder;
class System_ScnrZoneBuilder;
class System_BipdDataBuilder;
class System_ScenZoneBuilder;
class System_Debug;

struct System_Environment_Dependencies
{ 
	State_Map& State_Map;
	State_MapColl& State_MapColl;
	State_MapPhmo& State_MapPhmo;
	State_MapMode& State_MapMode;
	State_MapScnr& State_MapScnr;
	State_MapBipd& State_MapBipd;
	State_MapScen& State_MapScen;
	State_ObjectTable& State_ObjectTable;
	State_Classification& State_Classification;
	State_Environment& State_Environment;
	System_CollGeometryBuilder& System_CollGeometryBuilder;
	System_PhmoGeometryBuilder& System_PhmoGeometryBuilder;
	System_ModeGeometryBuilder& System_ModeGeometryBuilder;
	System_ScnrZoneBuilder& System_ScnrZoneBuilder;
	System_BipdDataBuilder& System_BipdDataBuilder;
	System_ScenZoneBuilder& System_ScenZoneBuilder;
	System_Debug& System_Debug;
};

class System_Environment
{
public:
	System_Environment(System_Environment_Dependencies dependencies) : 
		m_Deps(dependencies) {}
	~System_Environment() = default;

	// --- Static Data ---
	void BuildForMap();

	// --- Dynamic Data ---
	void UpdateEnvironment();

	// Cleanup.
	void Cleanup();

private:
	System_Environment_Dependencies m_Deps;

	// --- Static Data ---

	bool BuildColl(const std::string& tagName);
	bool BuildPhmo(const std::string& tagName);
	bool BuildMode(const std::string& tagName);
	bool BuildScnr(const std::string& tagName);
	bool BuildBipd(const std::string& tagName);
	bool BuildScen(const std::string& tagName);

	// --- Dynamic Data ---

	void BuildPhysicsInstances(
		const std::vector<ClassifiedObject>& classifieds,
		const std::unordered_map<uint32_t, LiveObject>& objects);

	// --- Helpers ---

	std::array<float, 3> Cross(const std::array<float, 3>& a, 
		const std::array<float, 3>& b);

	std::array<float, 3> TransformPoint(const std::array<float, 3>& pos,
		const std::array<float, 3>& right, const std::array<float, 3>& forward,
		const std::array<float, 3>& up, float lx, float ly, float lz);

	std::vector<WorldRigidBody> BuildWorldRigidBodies(
		const ActivePhysicsInstance& inst);
};