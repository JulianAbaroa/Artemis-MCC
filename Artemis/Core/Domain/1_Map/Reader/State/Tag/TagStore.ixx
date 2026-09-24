module;

#include <cassert>

export module Map.Reader.State:Tag;

import std;

export namespace Map::Reader::State
{
	template <typename TObject>
	class TagStore
	{
	public:
		using ObjectType = TObject;

		auto Has(const std::string& tagName) const -> bool
		{
			assert(m_Frozen.load(std::memory_order_acquire));
			return m_Map.find(tagName) != m_Map.end();
		}

		auto Get(const std::string& tagName) const -> const TObject*
		{
			assert(m_Frozen.load(std::memory_order_acquire));
			auto it = m_Map.find(tagName);
			return it != m_Map.end() ? &it->second : nullptr;
		}

		auto Add(const std::string& tagName, TObject data) -> void
		{
			assert(!m_Frozen.load(std::memory_order_relaxed));
			m_Map.emplace(tagName, std::move(data));
		}

		auto All() const -> const std::unordered_map<std::string, TObject>&
		{
			assert(m_Frozen.load(std::memory_order_acquire));
			return m_Map;
		}

		auto Freeze() -> void
		{
			m_Frozen.store(true, std::memory_order_release);
		}

		// Must be called just by the builder class.
		auto Contains(const std::string& tagName) const -> bool
		{
			return m_Map.find(tagName) != m_Map.end();
		}

		auto Cleanup() -> void
		{
			m_Frozen.store(false, std::memory_order_relaxed);
			m_Map.clear();
		}

	protected:
		std::unordered_map<std::string, TObject> m_Map;
		std::atomic<bool> m_Frozen{ false };
	};
}