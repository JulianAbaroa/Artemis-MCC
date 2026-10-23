export module Datum.Layer;

import Service.Layer;
import Platform.Layer;
import Datum.Object.State;
import Datum.Object.System;
import Datum.Object.Hook;

export namespace Datum
{
    // Reads the live memory of the game as raw copies, without interpreting it.
    // For now it copies the datum of every object of the object table.
    class Layer
    {
    private:
        using DatumStore = Datum::Object::State::DatumStore;

        using DatumService = Datum::Object::System::DatumService;

        using ObjectTableLocator = Datum::Object::Hook::ObjectTableLocator;

    public:
        Layer(Service::Layer& service, Platform::Layer& platform) :
            m_DatumService(service.m_Log, platform.m_MemoryReaderService, m_DatumStore),
            m_ObjectTableLocator(service.m_Log, platform.m_AOBService, m_DatumStore)
        {
            auto& lifecycle = platform.m_LifecycleService;

            lifecycle.OnEngineInitialized([this] {
                m_ObjectTableLocator.FindAndStoreTableBase();
            });

            lifecycle.OnCleanup([this] {
                m_DatumService.Cleanup();
            });
        }
        ~Layer() = default;

        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        DatumStore m_DatumStore{};

        // --- System ---
        DatumService m_DatumService;

        // --- Hook ---
        ObjectTableLocator m_ObjectTableLocator;
    };
}