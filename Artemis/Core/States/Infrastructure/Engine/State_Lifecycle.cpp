#include "pch.h"
#include "Core/States/Infrastructure/Engine/State_Lifecycle.h"

bool State_Lifecycle::IsRunning() const { return m_IsRunning.load(); }
void State_Lifecycle::SetRunning(bool value) { m_IsRunning.store(value); }

HMODULE State_Lifecycle::GetHandleModule() const { return m_HandleModule.load(); }
void State_Lifecycle::SetHandleModule(HMODULE value) { m_HandleModule.store(value); }

EngineStatus State_Lifecycle::GetEngineStatus() const { return m_EngineStatus.load(); }
void State_Lifecycle::SetEngineStatus(EngineStatus value) { m_EngineStatus.store(value); }

std::mutex& State_Lifecycle::GetMutex() const { return m_Mutex; }
std::condition_variable& State_Lifecycle::GetCV() const { return m_ShutdownCV; }