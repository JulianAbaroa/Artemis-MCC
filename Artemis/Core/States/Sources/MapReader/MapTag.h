#pragma once

#include <unordered_map>
#include <cassert>
#include <atomic>
#include <string>

template <typename TObject>
class MapTag
{
public:
	bool Has(const std::string& tagName) const
	{
		assert(m_Frozen.load(std::memory_order_acquire));
		return m_Map.find(tagName) != m_Map.end();
	}

	const TObject* Get(const std::string& tagName) const
	{
		assert(m_Frozen.load(std::memory_order_acquire));
		auto it = m_Map.find(tagName);
		return it != m_Map.end() ? &it->second : nullptr;
	}

	void Add(const std::string& tagName, TObject data)
	{
		assert(!m_Frozen.load(std::memory_order_relaxed));
		m_Map.emplace(tagName, std::move(data));
	}

	const std::unordered_map<std::string, TObject>& All() const
	{
		assert(m_Frozen.load(std::memory_order_acquire));
		return m_Map;
	}

	void Freeze()
	{
		m_Frozen.store(true, std::memory_order_release);
	}

	void Cleanup()
	{
		m_Frozen.store(false, std::memory_order_relaxed);
		m_Map.clear();
	}

protected:
	std::unordered_map<std::string, TObject> m_Map;
	std::atomic<bool> m_Frozen{ false };
};