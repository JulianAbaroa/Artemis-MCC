export module Tables.Object.State;

export import :BoneMatrices;
export import :BoneOffsets;
export import :DamageSections;

import Tables.Object.Type;
import std;

export namespace Tables::Object::State
{
	class ObjectTableStore
	{
	private:
		using AliveObject = Tables::Object::Type::Alive::Object;
		using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;

	public:
		ObjectTableStore() = default;
		~ObjectTableStore() = default;

		auto GetBase() -> std::uintptr_t;
		auto SetBase(std::uintptr_t pointer) -> void;

		auto AddObject(std::uint32_t handle, const AliveObject& object) -> void;
		auto RemoveObject(std::uint32_t handle) -> std::optional<AliveObject>;

		auto UpdateObjects(std::function<void(std::uint32_t, AliveObject&)> processor) -> void;

		auto Publish() -> void;
		auto Acquire() const -> std::shared_ptr<const ObjectTable>;

		auto Cleanup() -> void;

	private:
		std::atomic<std::uintptr_t> m_ObjectTableBase{ 0 };
		std::atomic<std::shared_ptr<const ObjectTable>> m_pObjectTable{};

		std::unordered_map<std::uint32_t, AliveObject> m_ObjectTable{};
		mutable std::mutex m_Mutex{};
	};
}