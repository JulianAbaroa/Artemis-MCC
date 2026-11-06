export module Blam.Layout.Snapshot.Type:Objects;

import :Raw;

import Common.Engine.Type;
import std;

namespace
{
    using Common::Engine::Type::k_HandleIndexMask;
    using Common::Engine::Type::k_MaxObjects;

    // The arena of a table that holds nothing has no room yet. The first growth takes at least this much.
    constexpr std::size_t k_MinArenaBytes{ 64 * 1024 };
}

export namespace Blam::Layout::Snapshot::Type
{
    // The live objects of a Snapshot: the datums one after the other in one arena, the RawObject views of them in the order they were
    // added (the capture and the .sdump reader add them by slot), and an index by slot of the engine object table (D276).
    // note: The views point into the arena. Adding may grow it: the views are then rebased, so a RawObject taken from the table is
    // only valid until the next Add, Emplace, PopLast or Clear. A published Snapshot is never changed. Copying a table copies the arena
    // and rebases the views, moving it keeps them. Clear keeps the capacity, so a table that is reused allocates nothing.
    class RawObjectTable
    {
    public:
        using const_iterator = std::vector<RawObject>::const_iterator;

        RawObjectTable() = default;
        ~RawObjectTable() = default;

        RawObjectTable(const RawObjectTable& other)
        {
            this->Grow(other.m_Used);
            if (other.m_Used > 0) std::memcpy(m_Arena.get(), other.m_Arena.get(), other.m_Used);
            m_Used = other.m_Used;
            m_Objects = other.m_Objects;
            m_Slots = other.m_Slots;
            this->Rebase(other.m_Arena.get());
        }

        auto operator=(const RawObjectTable& other) -> RawObjectTable&
        {
            if (this != &other)
            {
                RawObjectTable copy{ other };
                *this = std::move(copy);
            }

            return *this;
        }

        RawObjectTable(RawObjectTable&&) noexcept = default;
        auto operator=(RawObjectTable&&) noexcept -> RawObjectTable& = default;

        auto begin() const -> const_iterator { return m_Objects.begin(); }
        auto end() const -> const_iterator { return m_Objects.end(); }
        auto Size() const -> std::size_t { return m_Objects.size(); }
        // return: Bytes the arena keeps allocated (what the table keeps alive, used or not).
        auto CapacityBytes() const -> std::size_t { return m_Capacity; }
        auto IsEmpty() const -> bool { return m_Objects.empty(); }

        // return: The object of the handle, or null. The whole handle must match: an object whose slot was reused under another salt
        // is not found.
        auto Find(std::uint32_t handle) const -> const RawObject*
        {
            const std::uint32_t slot{ handle & k_HandleIndexMask };
            if (slot >= k_MaxObjects || m_Slots[slot] == 0) return nullptr;

            const RawObject& object{ m_Objects[m_Slots[slot] - 1u] };
            return object.Handle == handle ? &object : nullptr;
        }

        auto Contains(std::uint32_t handle) const -> bool { return this->Find(handle) != nullptr; }

        // Adds an object with room for size bytes.
        // return: The bytes to fill in (their content is not defined), or nothing if the slot of the handle is outside the engine
        // table or already holds an object.
        auto Emplace(std::uint32_t handle, std::uintptr_t address, std::uint8_t kindId, std::size_t size) -> std::optional<std::span<std::uint8_t>>
        {
            const std::uint32_t slot{ handle & k_HandleIndexMask };
            if (slot >= k_MaxObjects || m_Slots[slot] != 0) return std::nullopt;

            if (m_Used + size > m_Capacity) this->Grow(m_Used + size);

            std::uint8_t* start{ m_Arena.get() + m_Used };
            m_Used += size;
            m_Objects.push_back(RawObject{ .Handle = handle, .Address = address, .KindId = kindId, .Bytes = std::span<const std::uint8_t>{ start, size } });
            m_Slots[slot] = static_cast<std::uint16_t>(m_Objects.size());
            return std::span<std::uint8_t>{ start, size };
        }

        // Adds an object with a copy of the bytes.
        // return: False if the slot of the handle is outside the engine table or already holds an object.
        auto Add(std::uint32_t handle, std::uintptr_t address, std::uint8_t kindId, std::span<const std::uint8_t> bytes) -> bool
        {
            const std::optional<std::span<std::uint8_t>> room{ this->Emplace(handle, address, kindId, bytes.size()) };
            if (!room) return false;

            if (!bytes.empty()) std::memcpy(room->data(), bytes.data(), bytes.size());
            return true;
        }

        // Takes back the object added last (the capture does it when it cannot read the datum).
        auto PopLast() -> void
        {
            if (m_Objects.empty()) return;

            m_Slots[m_Objects.back().Handle & k_HandleIndexMask] = 0;
            m_Used -= m_Objects.back().Bytes.size();
            m_Objects.pop_back();
        }

        // return: The bytes of the object to change, or nothing if it is not there.
        // note: For the builders of Snapshots and the tests. A published Snapshot is never changed.
        auto Edit(std::uint32_t handle) -> std::span<std::uint8_t>
        {
            const RawObject* object{ this->Find(handle) };
            if (object == nullptr) return {};

            return std::span<std::uint8_t>{ m_Arena.get() + (object->Bytes.data() - m_Arena.get()), object->Bytes.size() };
        }

        // Removes every object and keeps the memory.
        auto Clear() -> void
        {
            for (const RawObject& object : m_Objects) m_Slots[object.Handle & k_HandleIndexMask] = 0;

            m_Objects.clear();
            m_Used = 0;
        }

    private:
        std::unique_ptr<std::uint8_t[]> m_Arena{};
        std::size_t m_Capacity{};
        std::size_t m_Used{};
        std::vector<RawObject> m_Objects{};
        // By slot: the position in m_Objects plus one, 0 when the slot is empty.
        std::array<std::uint16_t, k_MaxObjects> m_Slots{};

        // Makes room for at least bytes in the arena and rebases the views.
        auto Grow(std::size_t bytes) -> void
        {
            if (bytes <= m_Capacity) return;

            const std::size_t capacity{ (std::max)({ bytes, m_Capacity * 2, k_MinArenaBytes }) };
            std::unique_ptr<std::uint8_t[]> arena{ std::make_unique_for_overwrite<std::uint8_t[]>(capacity) };
            if (m_Used > 0) std::memcpy(arena.get(), m_Arena.get(), m_Used);

            const std::uint8_t* old{ m_Arena.get() };
            m_Arena = std::move(arena);
            m_Capacity = capacity;
            this->Rebase(old);
        }

        // Points the views that were into the arena at oldBase to the same offsets of the current one.
        auto Rebase(const std::uint8_t* oldBase) -> void
        {
            for (RawObject& object : m_Objects)
            {
                const std::size_t offset{ static_cast<std::size_t>(object.Bytes.data() - oldBase) };
                object.Bytes = std::span<const std::uint8_t>{ m_Arena.get() + offset, object.Bytes.size() };
            }
        }
    };
}