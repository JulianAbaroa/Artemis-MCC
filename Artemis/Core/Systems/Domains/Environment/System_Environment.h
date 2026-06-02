#pragma once

#include <unordered_map>
#include <vector>
#include <string>

struct Classified;
struct LiveObject;
struct WorldRigidBody;
struct ActivePhysicsInstance;

class State_MapReader;
class State_MapColl;
class State_MapPhmo;
class State_MapMode;
class State_MapScen;
class State_ObjectTable;
class State_Classifier;
class State_Environment;
class System_CollBuilder;
class System_ModeBuilder;
class System_PhmoBuilder;
class System_Logs;

struct System_Environment_Dependencies
{ 
	State_MapReader& State_Map;
	State_MapColl& State_MapColl;
	State_MapPhmo& State_MapPhmo;
	State_MapMode& State_MapMode;
	State_MapScen& State_MapScen;
	State_ObjectTable& State_ObjectTable;
	State_Classifier& State_Classification;
	State_Environment& State_Environment;
	System_CollBuilder& System_CollBuilder;
	System_ModeBuilder& System_ModeBuilder;
	System_PhmoBuilder& System_PhmoBuilder;
	System_Logs& System_Logs;
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

	// --- Dynamic Data ---

	void BuildPhysicsInstances(
		const std::vector<Classified>& classifieds,
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