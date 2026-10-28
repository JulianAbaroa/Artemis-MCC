export module Runtime.Thread:Main;

import Service.Layer;
import Engine.Base.Layer;
import Engine.Match.Layer;
import Engine.Render.Layer;
import std;

export namespace Runtime::Thread
{
    // Thread that installs and watches the game hooks and drives the shutdown.
    // note: Reinstalls the lifecycle hooks when the game engine is destroyed or the hooks get corrupted.
    class MainThread
    {
    private:
        using milliseconds = std::chrono::milliseconds;

    public:
        MainThread(Service::Layer& service, Engine::Base::Layer& base, Engine::Match::Layer& match, Engine::Render::Layer& render) :
            m_Service(service), m_Base(base), m_Match(match), m_Render(render) {}
        ~MainThread() = default;

        // Installs the hooks and supervises them until the lifecycle stops.
        auto Run() -> void;

    private:
        Service::Layer& m_Service;
        Engine::Base::Layer& m_Base;
        Engine::Match::Layer& m_Match;
        Engine::Render::Layer& m_Render;

        // Sleeps for the given time unless the shutdown is signaled.
        // return: true if the time elapsed, false if the shutdown was signaled.
        auto WaitOrExit(milliseconds ms) -> bool;

        // Retries until the lifecycle hooks are installed or the shutdown is signaled.
        // return: true if both hooks were installed.
        auto InstallLifecycleHooks() -> bool;

        // Tries a few times to install the render and raw input hooks.
        // return: true if the render hooks were installed.
        auto InstallRenderHooks() -> bool;

        // Signals an emergency shutdown.
        auto Shutdown() -> void;

        // Marks the engine as destroyed when a lifecycle hook is corrupted.
        auto CheckHooksHealth() -> void;

        // Checks that the function at the address still starts with a jump.
        auto IsHookIntact(void* address) -> bool;

        // Waits a moment and checks that the lifecycle is still running.
        auto IsStillRunning() -> bool;
    };
}