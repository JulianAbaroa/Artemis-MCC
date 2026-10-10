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

		auto AddObject(std::uint32_t handle, const AliveObject& object) -> void;
		auto HasObject(std::uint32_t handle) const -> bool;
		auto RemoveObject(std::uint32_t handle) -> std::optional<AliveObject>;

		auto UpdateObjects(std::function<void(std::uint32_t, AliveObject&)> processor) -> void;

		auto Publish() -> void;
		auto Acquire() const -> std::shared_ptr<const ObjectTable>;

		auto Cleanup() -> void;

	private:
		std::atomic<std::shared_ptr<const ObjectTable>> m_pObjectTable{};

		std::unordered_map<std::uint32_t, AliveObject> m_ObjectTable{};
		mutable std::mutex m_Mutex{};
	};
}