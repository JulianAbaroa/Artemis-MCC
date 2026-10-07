module;

#include <cassert>

export module Map.Reader.State:Tag;

import std;

export namespace Map::Reader::State
{
    // Tags of one group by tag name. They are added while the map is built, then the store is frozen and only read.
    // note: Reads assert that the store is frozen, and Add asserts that it is not.
    template <typename TObject>
    class TagStore
    {
    public:
        using ObjectType = TObject;

        auto Has(const std::string& tagName) const -> bool
        {
            assert(m_IsFrozen.load(std::memory_order_acquire));
            return m_Map.find(tagName) != m_Map.end();
        }

        // return: Null if the tag is not in the store.
        auto Get(const std::string& tagName) const -> const TObject*
        {
            assert(m_IsFrozen.load(std::memory_order_acquire));
            auto it = m_Map.find(tagName);
            return it != m_Map.end() ? &it->second : nullptr;
        }

        auto Add(const std::string& tagName, TObject data) -> void
        {
            assert(!m_IsFrozen.load(std::memory_order_relaxed));
            m_Map.emplace(tagName, std::move(data));
        }

        auto All() const -> const std::unordered_map<std::string, TObject>&
        {
            assert(m_IsFrozen.load(std::memory_order_acquire));
            return m_Map;
        }

        // Ends the build. From now on the store can only be read.
        auto Freeze() -> void
        {
            m_IsFrozen.store(true, std::memory_order_release);
        }

        auto IsFrozen() const -> bool
        {
            return m_IsFrozen.load(std::memory_order_acquire);
        }

        // Checks if a tag was added, without requiring the store to be frozen.
        // note: Only for the builder, which runs before the freeze.
        auto Contains(const std::string& tagName) const -> bool
        {
            return m_Map.find(tagName) != m_Map.end();
        }

        // Removes every tag and unfreezes the store.
        auto Cleanup() -> void
        {
            m_IsFrozen.store(false, std::memory_order_relaxed);
            m_Map.clear();
        }

    protected:
        std::unordered_map<std::string, TObject> m_Map{};
        std::atomic<bool> m_IsFrozen{ false };
    };
}