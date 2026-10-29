export module Blam.Runtime.Object.State;

export import :BoneMatrices;
export import :DamageSections;

import Blam.Runtime.Object.Type;
import std;

export namespace Blam::Runtime::Object::State
{
    class ObjectTableStore
    {
    private:
        using AliveObject = Blam::Runtime::Object::Type::Alive::AliveObject;
        using ObjectTable = Blam::Runtime::Object::Type::Alive::ObjectTable;

    public:
        ObjectTableStore() = default;
        ~ObjectTableStore() = default;

        auto AddObject(std::uint32_t handle, const AliveObject& object) -> void;
        auto HasObject(std::uint32_t handle) const -> bool;
        auto RemoveObject(std::uint32_t handle) -> std::optional<AliveObject>;

        auto UpdateObjects(std::function<void(std::uint32_t, AliveObject&)> processor) -> void;

        auto Publish() -> void;
        auto Acquire() const -> std::shared_ptr<const ObjectTable>;

        auto Cleanup() -> void;

    private:
        std::atomic<std::shared_ptr<const ObjectTable>> m_PublishedTable{};

        ObjectTable m_ObjectTable{};
        mutable std::mutex m_Mutex{};
    };
}