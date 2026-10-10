export module Template.Object.Hook;

import Service.Logs.System;
import Platform.Memory.System;
import Template.Object.State;
import std;

export namespace Template::Object::Hook
{
    // Finds the engine object table and stores its base address.
    class ObjectTableLocator
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using AOBService = Platform::Memory::System::AOBService;

        using TemplateStore = Template::Object::State::TemplateStore;

    public:
        ObjectTableLocator(LogsService& logsService, AOBService& aobService,
            TemplateStore& templateStore) : m_LogsService(logsService),
            m_AOBService(aobService), m_TemplateStore(templateStore) {}
        ~ObjectTableLocator() = default;

        // Stores the table base unless it is already stored.
        // note: Logs an error and stores nothing if the table is not found. Call it again later.
        auto FindAndStoreTableBase() -> void;

    private:
        LogsService& m_LogsService;
        AOBService& m_AOBService;
        TemplateStore& m_TemplateStore;

        // return: Address of the object table, or 0 if the thread context is not ready.
        auto GetObjectTable() -> std::uintptr_t;
    };
}