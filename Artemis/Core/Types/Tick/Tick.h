#pragma once

#include <unordered_map>
#include <cstdint>
#include <memory>
	
struct LiveObject;
struct LivePlayer;
struct LiveInteraction;
struct Classified;
struct ObjectNode;
struct PlayerTree;
struct Collidable;
struct Fixtures;
struct ObjectVitality;
struct Self;
struct Affordance;

using ObjectTable = std::unordered_map<uint32_t, LiveObject>;
using PlayerTable = std::unordered_map<uint32_t, LivePlayer>;
using Classifieds = std::vector<Classified>;
using ObjectGraph = std::unordered_map<uint32_t, ObjectNode>;
using PlayerGraph = std::vector<PlayerTree>;

using Collidables = std::vector<Collidable>;
using Vitalities = std::unordered_map<uint32_t, ObjectVitality>;

using Affordances = std::vector<Affordance>;

struct Tick
{
	uint64_t Generation = 0;

	// --- Layer 0: Sources ---
	std::shared_ptr<const ObjectTable> ObjectTable;
	std::shared_ptr<const PlayerTable> PlayerTable;
	std::shared_ptr<const LiveInteraction> Interaction;

	// --- Layer 1: Structure ---
	std::shared_ptr<const Classifieds> Classifieds;
	std::shared_ptr<const ObjectGraph> ObjectGraph;
	std::shared_ptr<const PlayerGraph> PlayerGraph;

	// --- Layer 2: Environment ---
	std::shared_ptr<const Collidables> Collidables;
	std::shared_ptr<const Fixtures> Fixtures;
	std::shared_ptr<const Vitalities> Vitalities;

	// --- Layer 3: Egocentric ---
	std::shared_ptr<const Self> Self;
	std::shared_ptr<const Affordances> Affordances;
};