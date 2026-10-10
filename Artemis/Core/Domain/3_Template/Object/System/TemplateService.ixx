export module Template.Object.System;

import Service.Logs.System;
import Platform.Memory.System;
import Template.Object.State;
import std;

export namespace Template::Object::System
{
    // Copies the datum of every live object of the engine object table and publishes the copies to the store.
    class TemplateService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;

        using TemplateStore = Template::Object::State::TemplateStore;

    public:
        TemplateService(LogsService& logsService,
            MemoryReaderService& memoryReaderService,
            TemplateStore& templateStore) :
            m_LogsService(logsService), m_MemoryReaderService(memoryReaderService),
            m_TemplateStore(templateStore) {}
        ~TemplateService() = default;

        // Reads the object table and publishes a snapshot with the datum of each live object.
        // note: Publishes nothing while the table base is not found.
        auto UpdateObjectTable() -> void;

        // Clears the stored table base and snapshot.
        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        MemoryReaderService& m_MemoryReaderService;
        TemplateStore& m_TemplateStore;
    };
}