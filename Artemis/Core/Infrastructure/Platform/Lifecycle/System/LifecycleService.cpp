module Platform.Lifecycle.System;
import :Lifecycle;
import std;

namespace Platform::Lifecycle::System
{
    auto LifecycleService::SignalShutdown() -> void
    {
        m_LifecycleStore.SetRunning(false);

        {
            std::lock_guard<std::mutex> lock(
                m_LifecycleStore.GetShutdownMutex());

            m_LifecycleStore.GetShutdownCV().notify_all();
        }

        m_Log.Message("[LifecycleService] WARNING:"
            " Shutdown signaled to all threads.");
    }

    auto LifecycleService::OnEngineInitialized(LifecycleCallback callback) -> void
    {
        m_OnEngineInitialized.push_back(std::move(callback));
    }

    auto LifecycleService::OnUnhook(LifecycleCallback callback) -> void
    {
        m_OnUnhook.push_back(std::move(callback));
    }

    auto LifecycleService::OnCleanup(LifecycleCallback callback) -> void
    {
        m_OnCleanup.push_back(std::move(callback));
    }

    auto LifecycleService::OnShutdown(LifecycleCallback callback) -> void
    {
        m_OnShutdown.push_back(std::move(callback));
    }

    auto LifecycleService::OnMapFileOpened(MapFileCallback callback) -> void
    {
        m_OnMapFileOpened.push_back(std::move(callback));
    }

    auto LifecycleService::RaiseEngineInitialized() -> void
    {
        for (auto& callback : m_OnEngineInitialized) callback();
    }

    auto LifecycleService::RaiseUnhook() -> void
    {
        for (auto& callback : m_OnUnhook) callback();
    }

    auto LifecycleService::RaiseCleanup() -> void
    {
        for (auto& callback : m_OnCleanup) callback();
    }

    auto LifecycleService::RaiseMapFileOpened(const std::string& filePath) -> void
    {
        for (auto& callback : m_OnMapFileOpened) callback(filePath);
    }

    auto LifecycleService::RaiseShutdown() -> void
    {
        if (m_IsShutdownRaised.exchange(true)) return;

        for (auto it = m_OnShutdown.rbegin(); it != m_OnShutdown.rend(); it++)
        {
            (*it)();
        }
    }
}