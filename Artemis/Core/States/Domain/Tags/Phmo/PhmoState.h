#pragma once

//#include "Core/Common/Types/Domain/Map/PhmoTypes.h"
#include <unordered_map>
#include <mutex>

class PhmoState
{
public:
	//// Returns nullptr if handle has no physics on this snapshot.
	//const LivePhmoObject* GetPhmos(uint32_t handle) const;

	//// Replaces the entire snapshot with the new snapshot's data.
	//void SetPhmos(std::unordered_map<uint32_t, LivePhmoObject> snapshot);

	void Cleanup();

private:
	//std::unordered_map<uint32_t, LivePhmoObject> m_Phmos;
	mutable std::mutex m_Mutex;
};