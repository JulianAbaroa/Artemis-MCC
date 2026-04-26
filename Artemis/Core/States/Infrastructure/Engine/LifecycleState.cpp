#include "pch.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"

bool LifecycleState::IsRunning() const { return m_IsRunning.load(); }
void LifecycleState::SetRunning(bool value) { m_IsRunning.store(value); }

HMODULE LifecycleState::GetHandleModule() const { return m_HandleModule.load(); }
void LifecycleState::SetHandleModule(HMODULE value) { m_HandleModule.store(value); }

EngineStatus LifecycleState::GetEngineStatus() const { return m_EngineStatus.load(); }
void LifecycleState::SetEngineStatus(EngineStatus value) { m_EngineStatus.store(value); }

std::mutex& LifecycleState::GetMutex() const { return m_Mutex; }
std::condition_variable& LifecycleState::GetCV() const { return m_ShutdownCV; }