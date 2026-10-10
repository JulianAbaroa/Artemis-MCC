export module Template.Object.State;

import Template.Object.Type;
import std;

export namespace Template::Object::State
{
    // Holds the base address of the engine object table and the last published snapshot of its objects.
    // note: Lock-free. A published snapshot is never modified.
    class TemplateStore
    {
    private:
        using Snapshot = Template::Object::Type::Raw::Snapshot;

    public:
        TemplateStore() = default;
        ~TemplateStore() = default;

        // return: Address of the object table, or 0 if it was not found yet.
        auto GetBase() const -> std::uintptr_t;
        auto SetBase(std::uintptr_t pointer) -> void;

        // Replaces the snapshot that Acquire returns.
        auto Publish(Snapshot snapshot) -> void;

        // return: Last published snapshot, or null before the first one and after Cleanup.
        auto Acquire() const -> std::shared_ptr<const Snapshot>;

        // Clears the base address and drops the published snapshot.
        auto Cleanup() -> void;

    private:
        std::atomic<std::uintptr_t> m_ObjectTableBase{ 0 };
        std::atomic<std::shared_ptr<const Snapshot>> m_Snapshot{};
    };
}