#include "pch.h"
#include "Core/States/Domain/Tags/Phmo/PhmoState.h"

//const LivePhmoObject* PhmoState::GetPhmos(
//	uint32_t handle) const
//{
//    std::lock_guard<std::mutex> lock(m_Mutex);
//    auto it = m_Phmos.find(handle);
//    return (it != m_Phmos.end()) ? &it->second : nullptr;
//}
//
//void PhmoState::SetPhmos(
//	std::unordered_map<uint32_t, LivePhmoObject> snapshot)
//{
//    std::lock_guard<std::mutex> lock(m_Mutex);
//    m_Phmos = std::move(snapshot);
//}
//
//void PhmoState::Cleanup()
//{
//    std::lock_guard<std::mutex> lock(m_Mutex);
//    m_Phmos.clear();
//}