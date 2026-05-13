#pragma once

#include <unordered_map>
#include <vector>
#include <string>

class System_Debug;
class State_Environment;
struct ClassifiedObject;
struct LiveObject;
struct WorldRigidBody;
struct ActivePhysicsInstance;

class System_Environment
{
public:
	// --- Static Data ---
	void BuildForMap();

	// --- Dynamic Data ---
	void UpdateEnvironment();

	// Cleanup.
	void Cleanup();

private:
	// --- Static Data ---

	bool BuildColl(const std::string& tagName, System_Debug& debug,
		State_Environment& environment);

	bool BuildPhmo(const std::string& tagName,
		System_Debug& debug, State_Environment& environment);

	bool BuildMode(const std::string& tagName,
		System_Debug& debug, State_Environment& environment);

	bool BuildScnr(const std::string& tagName,
		System_Debug& debug, State_Environment& environment);

	bool BuildBipd(const std::string& tagName,
		System_Debug& debug, State_Environment& environment);

	bool BuildScen(const std::string& tagName,
		System_Debug& debug, State_Environment& environment);

	// --- Dynamic Data ---

	void BuildPhysicsInstances(State_Environment& environment,
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