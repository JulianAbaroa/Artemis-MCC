export module Tests.Support.EngineMemory;

import Common.Engine.Type;
import Engine.Capture.State;
import Blam.Layout.Object.Type;
import Blam.Layout.Snapshot.Type;
import std;

// The "game memory" of the capture tests (F1 C.5 c, S2.B): buffers of the test process laid out like the engine memory that
// CaptureService walks (the thread context, the headers of the data arrays, the object table, the regions), which MemoryReaderService
// reads like any address. It is filled by hand or from a recorded Snapshot.

namespace
{
    using Common::Engine::Type::k_MaxObjects;
    using Common::Engine::Type::k_MaxPlayers;
    using Engine::Capture::State::CaptureStore;
    using ::Blam::Layout::Snapshot::Type::RawObject;
    using ::Blam::Layout::Snapshot::Type::RegionId;
    using ::Blam::Layout::Snapshot::Type::Snapshot;
    using ::Blam::Layout::Snapshot::Type::k_RegionCount;

    using ::Blam::Layout::Object::Type::Constant::k_DataArrayCapacity;
    using ::Blam::Layout::Object::Type::Constant::k_DataArrayData;
    using ::Blam::Layout::Object::Type::Constant::k_DataArrayFirstUnusedIndex;
    using ::Blam::Layout::Object::Type::Constant::k_DataArrayLiveCount;
    using ::Blam::Layout::Object::Type::Constant::k_EntryAddressOffset;
    using ::Blam::Layout::Object::Type::Constant::k_EntryDatumSizeOffset;
    using ::Blam::Layout::Object::Type::Constant::k_EntryKindOffset;
    using ::Blam::Layout::Object::Type::Constant::k_EntrySaltOffset;
    using ::Blam::Layout::Object::Type::Constant::k_EntrySize;
    using ::Blam::Layout::Object::Type::Constant::k_MapVariantAdjust;
    using ::Blam::Layout::Object::Type::Constant::k_MapVariantAlignMask;
    using ::Blam::Layout::Object::Type::Constant::k_MinDatumSize;
    using ::Blam::Layout::Object::Type::Constant::k_PlacementsOffset;
    using ::Blam::Layout::Object::Type::Constant::k_ThreadContextEventGlobalsOffset;
    using ::Blam::Layout::Object::Type::Constant::k_ThreadContextGameEngineOffset;
    using ::Blam::Layout::Object::Type::Constant::k_ThreadContextGameModeOffset;
    using ::Blam::Layout::Object::Type::Constant::k_ThreadContextMapVariantOffset;
    using ::Blam::Layout::Object::Type::Constant::k_ThreadContextMegaloObjectsOffset;
    using ::Blam::Layout::Object::Type::Constant::k_ThreadContextObjectDataOffset;
    using ::Blam::Layout::Object::Type::Constant::k_ThreadContextTimeGlobalsOffset;
    using ::Blam::Layout::Object::Type::Constant::k_ThreadContextTelemetryDataOffset;
}

export namespace Tests::Support::EngineMemory
{
    class FakeEngineMemory
    {
    public:
        // An object of a recorded Snapshot as the fake memory holds it.
        struct LiveObject
        {
            std::uint32_t Handle{};
            std::uint8_t KindId{};
            std::uintptr_t Address{};
            std::size_t Size{};
        };

        // Where the fake memory holds the bytes of a region. Address 0 if the region was not in the Snapshot.
        struct RegionSource
        {
            std::uintptr_t Address{};
            std::size_t Size{};
        };

        FakeEngineMemory() = default;
        ~FakeEngineMemory() = default;

        FakeEngineMemory(const FakeEngineMemory&) = delete;
        auto operator=(const FakeEngineMemory&) -> FakeEngineMemory& = delete;

        // The thread context, the headers of the two data arrays and the object table the capture walks.
        std::vector<std::uintptr_t> m_Context{ std::vector<std::uintptr_t>(0x100) };
        std::vector<std::uint8_t> m_ObjectHeader{ std::vector<std::uint8_t>(0x100) };
        std::vector<std::uint8_t> m_PlayerHeader{ std::vector<std::uint8_t>(0x100) };
        // Entries past k_MaxObjects exist so that a walk that goes too far finds something to copy.
        static constexpr std::size_t k_TableEntries{ k_MaxObjects + 16 };
        std::vector<std::uint8_t> m_ObjectTable{ std::vector<std::uint8_t>(k_TableEntries * k_EntrySize) };
        // Datum bytes every entry made by AddLive points at.
        std::vector<std::uint8_t> m_Datum{ std::vector<std::uint8_t>(k_MinDatumSize) };

        // The objects and regions put there by Load, for the benches that copy the same bytes by other means.
        std::vector<LiveObject> m_Live{};
        std::array<RegionSource, k_RegionCount> m_Sources{};

        // Points the context at the headers and the store at the context and the table, as the locators do at a map load.
        auto Connect(CaptureStore& store) -> void
        {
            m_Context[k_ThreadContextObjectDataOffset / sizeof(std::uintptr_t)] = reinterpret_cast<std::uintptr_t>(m_ObjectHeader.data());
            m_Context[k_ThreadContextTelemetryDataOffset / sizeof(std::uintptr_t)] = reinterpret_cast<std::uintptr_t>(m_PlayerHeader.data());
            store.SetThreadContext(reinterpret_cast<std::uintptr_t>(m_Context.data()));
            store.SetObjectTableBase(reinterpret_cast<std::uintptr_t>(m_ObjectTable.data()));
        }

        auto SetCapacities(std::int32_t objects, std::int32_t players) -> void
        {
            std::memcpy(m_ObjectHeader.data() + k_DataArrayCapacity, &objects, sizeof(objects));
            std::memcpy(m_PlayerHeader.data() + k_DataArrayCapacity, &players, sizeof(players));
        }

        // Sets the first-unused slot (+0x44) and the live count (+0x48) of the object data array.
        auto SetMark(std::int32_t firstUnused, std::int32_t liveCount) -> void
        {
            std::memcpy(m_ObjectHeader.data() + k_DataArrayFirstUnusedIndex, &firstUnused, sizeof(firstUnused));
            std::memcpy(m_ObjectHeader.data() + k_DataArrayLiveCount, &liveCount, sizeof(liveCount));
        }

        // Makes the slot of the table a live object with salt 1: its handle is 0x10000 | index.
        auto AddLive(std::uint32_t index) -> void
        {
            std::uint8_t* entry{ m_ObjectTable.data() + (index * k_EntrySize) };
            const std::uint16_t salt{ 1 };
            const std::uint16_t size{ k_MinDatumSize };
            const std::uintptr_t address{ reinterpret_cast<std::uintptr_t>(m_Datum.data()) };
            std::memcpy(entry + k_EntrySaltOffset, &salt, sizeof(salt));
            std::memcpy(entry + k_EntryDatumSizeOffset, &size, sizeof(size));
            std::memcpy(entry + k_EntryAddressOffset, &address, sizeof(address));
        }

        // Lays the objects and regions of a recorded Snapshot out like the engine does and points the store at them: a table entry per
        // object (salt, kind, datum size and the address of a copy of the datum, in the slot handle & 0xFFFF), the headers with the
        // capacity, the first-unused mark and the live count, the thread context with the pointers to the copies of the regions and the
        // map variant that leads to the placements.
        // note: The buffers are reused between calls, so the memory of one Load is only valid until the next one.
        auto Load(const Snapshot& snapshot, CaptureStore& store) -> void
        {
            std::ranges::fill(m_Context, std::uintptr_t{});
            std::ranges::fill(m_ObjectTable, std::uint8_t{});
            std::ranges::fill(m_ObjectHeader, std::uint8_t{});
            std::ranges::fill(m_PlayerHeader, std::uint8_t{});
            m_Live.clear();
            m_Sources = {};

            std::size_t total{};
            for (const RawObject& raw : snapshot.RawObjects) total += raw.Bytes.size();
            m_Arena.resize(total);

            std::size_t used{};
            std::uint32_t firstUnused{};
            for (const RawObject& raw : snapshot.RawObjects)
            {
                const std::uint32_t handle{ raw.Handle };
                const std::uint32_t index{ handle & 0xFFFFu };
                if (index >= k_TableEntries) continue;

                std::uint8_t* datum{ m_Arena.data() + used };
                std::memcpy(datum, raw.Bytes.data(), raw.Bytes.size());
                used += raw.Bytes.size();

                std::uint8_t* entry{ m_ObjectTable.data() + (index * k_EntrySize) };
                const std::uint16_t salt{ static_cast<std::uint16_t>(handle >> 16) };
                const std::uint16_t size{ static_cast<std::uint16_t>(raw.Bytes.size()) };
                const std::uintptr_t address{ reinterpret_cast<std::uintptr_t>(datum) };
                std::memcpy(entry + k_EntrySaltOffset, &salt, sizeof(salt));
                std::memcpy(entry + k_EntryKindOffset, &raw.KindId, sizeof(raw.KindId));
                std::memcpy(entry + k_EntryDatumSizeOffset, &size, sizeof(size));
                std::memcpy(entry + k_EntryAddressOffset, &address, sizeof(address));

                m_Live.push_back(LiveObject{ .Handle = handle, .KindId = raw.KindId, .Address = address, .Size = raw.Bytes.size() });
                firstUnused = (std::max)(firstUnused, index + 1);
            }
            std::ranges::sort(m_Live, {}, [](const LiveObject& object) { return object.Handle & 0xFFFFu; });

            this->SetCapacities(static_cast<std::int32_t>(k_MaxObjects), static_cast<std::int32_t>(k_MaxPlayers));
            this->SetMark(static_cast<std::int32_t>(firstUnused), static_cast<std::int32_t>(m_Live.size()));

            for (std::size_t id{ 0 }; id < k_RegionCount; ++id)
            {
                const auto& bytes{ snapshot.Regions[id].Bytes };
                std::vector<std::uint8_t>& copy{ m_RegionCopies[id] };
                if (bytes.empty())
                {
                    copy.clear();
                    continue;
                }

                // The placements live inside the map variant: the copy goes where the engine has them.
                if (static_cast<RegionId>(id) == RegionId::Placements) copy.resize(k_PlacementsOffset + bytes.size() + k_MapVariantAlignMask + 1);
                else copy.resize(bytes.size());

                std::uint8_t* destination{ copy.data() };
                if (static_cast<RegionId>(id) == RegionId::Placements)
                {
                    const std::uintptr_t aligned{ (reinterpret_cast<std::uintptr_t>(copy.data()) + k_MapVariantAlignMask) & ~k_MapVariantAlignMask };
                    destination = reinterpret_cast<std::uint8_t*>(aligned) + k_PlacementsOffset;
                    m_Context[k_ThreadContextMapVariantOffset / sizeof(std::uintptr_t)] = aligned - k_MapVariantAdjust;
                }
                std::memcpy(destination, bytes.data(), bytes.size());

                m_Sources[id] = RegionSource{ .Address = reinterpret_cast<std::uintptr_t>(destination), .Size = bytes.size() };
            }

            const auto context{ [&](std::uintptr_t offset, RegionId id) {
                m_Context[offset / sizeof(std::uintptr_t)] = m_Sources[static_cast<std::size_t>(id)].Address;
            } };
            context(k_ThreadContextTimeGlobalsOffset, RegionId::TimeGlobals);
            context(k_ThreadContextGameEngineOffset, RegionId::GameEngineGlobals);
            context(k_ThreadContextEventGlobalsOffset, RegionId::EventGlobals);
            context(k_ThreadContextGameModeOffset, RegionId::GameGlobals);

            // The megalo objects are reached through the data array: the array holds the pointer to its elements.
            m_MegaloArray.assign(k_DataArrayData + sizeof(std::uintptr_t), std::uint8_t{});
            const std::uintptr_t megaloData{ m_Sources[static_cast<std::size_t>(RegionId::MegaloObjects)].Address };
            std::memcpy(m_MegaloArray.data() + k_DataArrayData, &megaloData, sizeof(megaloData));
            if (megaloData != 0) m_Context[k_ThreadContextMegaloObjectsOffset / sizeof(std::uintptr_t)] = reinterpret_cast<std::uintptr_t>(m_MegaloArray.data());

            this->Connect(store);
            if (!snapshot.HasThreadContext) store.SetThreadContext(0);
            if (!snapshot.HasObjectTable) store.SetObjectTableBase(0);
            store.SetPlayerTableBase(m_Sources[static_cast<std::size_t>(RegionId::PlayerArray)].Address);
            store.SetMappingBase(m_Sources[static_cast<std::size_t>(RegionId::PlayerMapping)].Address);
            store.SetInteractionTableBase(m_Sources[static_cast<std::size_t>(RegionId::Interaction)].Address);
        }

    private:
        // The datums copied by Load, one after the other.
        std::vector<std::uint8_t> m_Arena{};
        // The regions copied by Load. The placements copy holds the whole map variant offset.
        std::array<std::vector<std::uint8_t>, k_RegionCount> m_RegionCopies{};
        std::vector<std::uint8_t> m_MegaloArray{};
    };
}