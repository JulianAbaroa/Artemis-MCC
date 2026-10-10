export module Template.Layer;

import Service.Layer;
import Platform.Layer;
import Template.Object.State;
import Template.Object.System;
import Template.Object.Hook;

export namespace Template
{
    // Reads the live memory of the game as raw copies, without interpreting it.
    // For now it copies the datum of every object of the object table.
    class Layer
    {
    private:
        using TemplateStore = Template::Object::State::TemplateStore;

        using TemplateService = Template::Object::System::TemplateService;

        using ObjectTableLocator = Template::Object::Hook::ObjectTableLocator;

    public:
        Layer(Service::Layer& service, Platform::Layer& platform) :
            m_TemplateService(service.m_LogsService, platform.m_MemoryReaderService, m_TemplateStore),
            m_ObjectTableLocator(service.m_LogsService, platform.m_AOBService, m_TemplateStore)
        {
            auto& lifecycle = platform.m_LifecycleService;

            lifecycle.OnEngineInitialized([this] {
                m_ObjectTableLocator.FindAndStoreTableBase();
            });

            lifecycle.OnCleanup([this] {
                m_TemplateService.Cleanup();
            });
        }
        ~Layer() = default;

        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        TemplateStore m_TemplateStore;

        // --- System ---
        TemplateService m_TemplateService;

        // --- Hook ---
        ObjectTableLocator m_ObjectTableLocator;
    };
}