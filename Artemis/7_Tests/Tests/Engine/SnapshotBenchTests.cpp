#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Log.System;
import Common.Math.Type;
import Engine.Base.Lifecycle.State;
import Engine.Base.Memory.System;
import Engine.Capture.State;
import Engine.Capture.System;
import Services.Telemetry.State;
import Blam.Layout.Object.Type;
import Blam.Layout.Player.Type;
import Blam.Layout.Snapshot.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Object.State;
import Bridge.Codec.Pipeline.System;
import Tests.Support.Bench;
import Tests.Support.EngineMemory;
import Tests.Support.FilmPass;
import Tests.Support.Log;
import Tests.Support.Pipeline;
import std;

// Offline bench of Capture and of the ObjectTable stage (S2.B, D183): what each costs today and in which parts, over the golden
// recordings of Ignore/Regresion, to decide S2.C and S2.O with numbers and to measure before and after with the same bench. Capture
// reads fake memory built from each record, outside the measured time. Nothing here changes the pipeline. The tests are DISABLED_: run
// them with tools/test.py --disabled --filter "DISABLED_SnapshotBench.CtfD".

namespace
{
    using Common::Engine::Type::k_HandleIndexMask;
    using Common::Engine::Type::k_MaxObjects;
    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_SaltShift;
    using Common::Log::System::Log;
    using Common::Math::Type::Vec3;
    using Engine::Base::Lifecycle::State::LifecycleStore;
    using Engine::Base::Memory::System::MemoryReaderService;
    using Engine::Capture::State::CaptureStore;
    using Engine::Capture::System::CaptureService;
    using Services::Telemetry::State::TelemetryStore;
    using Tests::Support::Bench::Micros;
    using Tests::Support::Bench::Series;
    using Tests::Support::Bench::Stats;
    using Tests::Support::EngineMemory::FakeEngineMemory;
    using Tests::Support::FilmPass::FilmOutcome;
    using Tests::Support::FilmPass::RunFilmPass;
    using Tests::Support::Log::CollectingSink;
    using Tests::Support::Pipeline::Chain;
    using Bridge::Codec::Pipeline::System::k_PipelineStageNames;
    using Blam::MapModel::Definitions::Type::Object::ObjectKind;
    using Blam::MapModel::Definitions::Type::Object::ToObjectKind;
    using Blam::Layout::Snapshot::Type::RawObject;
    using Blam::Layout::Snapshot::Type::Region;
    using Blam::Layout::Snapshot::Type::RegionId;
    using Blam::Layout::Snapshot::Type::RegionSize;
    using Blam::Layout::Snapshot::Type::Snapshot;
    using Blam::Layout::Snapshot::Type::k_RegionCount;
    using Blam::Runtime::Object::State::BoneMatricesStore;
    using Blam::Runtime::Object::State::DamageSectionsStore;
    using Blam::Runtime::Object::State::ObjectTableStore;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::Alive::ObjectTable;
    using Blam::Runtime::Object::Type::BoneMatrix::BoneMatrix;
    using Blam::Runtime::Object::Type::BoneMatrix::BoneMatrixTable;
    using Blam::Runtime::Object::Type::DamageSection::DamageSection;
    using Blam::Runtime::Object::Type::DamageSection::DamageSectionTable;
    using Blam::Runtime::Object::Type::HandleAudit::HandleAudit;
    using std::chrono::steady_clock;

    using Blam::Layout::Object::Type::Constant::k_DataArrayCapacity;
    using Blam::Layout::Object::Type::Constant::k_DataArrayFirstUnusedIndex;
    using Blam::Layout::Object::Type::Constant::k_EntryAddressOffset;
    using Blam::Layout::Object::Type::Constant::k_EntryDatumSizeOffset;
    using Blam::Layout::Object::Type::Constant::k_EntryKindOffset;
    using Blam::Layout::Object::Type::Constant::k_EntrySaltOffset;
    using Blam::Layout::Object::Type::Constant::k_EntrySize;
    using Blam::Layout::Object::Type::Constant::k_MinDatumSize;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextObjectDataOffset;
    using Blam::Runtime::Object::Type::Constant::k_DamageSectionStride;
    using Blam::Runtime::Object::Type::Constant::k_RegionBlockMaxSize;

    namespace ObjeOffset = Blam::Layout::Object::Type::Obje::Offset;
    namespace UnitOffset = Blam::Layout::Object::Type::Unit::Offset;
    namespace ProjOffset = Blam::Layout::Object::Type::Proj::Offset;
    namespace PlayerOffset = Blam::Layout::Player::Type::Offset;
    using Blam::Layout::Player::Type::Size::k_Base;

    // The pipeline of the film runs on every record. The measures that copy a whole Snapshot (Capture, its parts, the ObjectTable parts)
    // run every K records: the first record and then each K-th. K = 1 is every record. K = 5 keeps ctf_c (33092 records, 287 s with K = 1)
    // under 150 s. The stages (P0, O0) are timed on every record.
    constexpr std::size_t k_EveryRecords{ 5 };

    // Weapon slots of a unit and the bytes between two handles (as ObjectTableService.cpp).
    constexpr std::uint8_t k_WeaponSlots{ 4 };
    constexpr std::size_t k_WeaponSlotStride{ sizeof(std::uint32_t) };
    // Nodes a model can have (as ObjectTableService.cpp).
    constexpr std::uint32_t k_MaxNodes{ 1024 };

    constexpr std::size_t k_StageCount{ k_PipelineStageNames.size() };

    // Copy of ResolveWorldOrigin of ObjectTableService.cpp (it is private to that file).
    template <typename Raws>
    auto ResolveWorldOrigin(const Raws& raws, const RawObject& datum, int depth = 0) -> Vec3
    {
        const Vec3 local = datum.Read<Vec3>(ObjeOffset::k_Origin);
        const std::uint32_t parentHandle = datum.Read<std::uint32_t>(ObjeOffset::k_ParentHandle);

        if (parentHandle == k_NoHandle || depth >= 8) return local;

        auto parentIt = raws.find(parentHandle);
        if (parentIt == raws.end()) return local;

        const RawObject& parent = parentIt->second;
        const Vec3 parentOrigin = ResolveWorldOrigin(raws, parent, depth + 1);
        const Vec3 forward = parent.Read<Vec3>(ObjeOffset::k_Forward);
        const Vec3 up = parent.Read<Vec3>(ObjeOffset::k_Up);
        const Vec3 left{
            up.Y * forward.Z - up.Z * forward.Y,
            up.Z * forward.X - up.X * forward.Z,
            up.X * forward.Y - up.Y * forward.X };

        return Vec3{
            parentOrigin.X + forward.X * local.X + left.X * local.Y + up.X * local.Z,
            parentOrigin.Y + forward.Y * local.X + left.Y * local.Y + up.Y * local.Z,
            parentOrigin.Z + forward.Z * local.X + left.Z * local.Y + up.Z * local.Z };
    }

    // Copy of AuditHandles of ObjectTableService.cpp.
    auto AuditHandles(const Snapshot& snapshot) -> HandleAudit
    {
        std::array<std::uint16_t, k_MaxObjects> salts{};
        for (const auto& [handle, datum] : snapshot.RawObjects)
        {
            const std::uint32_t index{ handle & k_HandleIndexMask };
            if (index < salts.size()) salts[index] = static_cast<std::uint16_t>(handle >> k_SaltShift);
        }

        HandleAudit audit{};
        const auto count{ [&](std::uint32_t handle) {
            if (handle == k_NoHandle || handle == 0) return;

            ++audit.Checked;

            const std::uint32_t index{ handle & k_HandleIndexMask };
            const std::uint16_t salt{ static_cast<std::uint16_t>(handle >> k_SaltShift) };
            if (salt == 0) ++audit.ZeroSalt;

            if (index >= salts.size() || salts[index] == 0) ++audit.Free;
            else if (salts[index] == salt) ++audit.Matching;
            else ++audit.Stale;
        } };

        for (const auto& [handle, datum] : snapshot.RawObjects)
        {
            count(datum.Read<std::uint32_t>(ObjeOffset::k_ParentHandle));
            count(datum.Read<std::uint32_t>(ObjeOffset::k_ChildHandle));
            count(datum.Read<std::uint32_t>(ObjeOffset::k_NextSiblingHandle));

            switch (ToObjectKind(datum.KindId))
            {
            case ObjectKind::Biped:
            case ObjectKind::Vehicle:
                for (std::uint8_t slot{ 0 }; slot < k_WeaponSlots; ++slot)
                {
                    count(datum.Read<std::uint32_t>(UnitOffset::k_PrimaryWeaponHandle + (slot * k_WeaponSlotStride)));
                }
                break;

            case ObjectKind::Projectile:
                count(datum.Read<std::uint32_t>(ObjeOffset::k_OwnerBipedHandle));
                count(datum.Read<std::uint32_t>(ProjOffset::k_ShooterRootHandle));
                break;

            default:
                break;
            }
        }

        if (snapshot.IsCaptured(RegionId::PlayerArray))
        {
            for (std::size_t index{ 0 }; index < k_MaxPlayers; ++index)
            {
                const std::size_t entry{ index * k_Base };
                if (snapshot.Read<std::uint16_t>(RegionId::PlayerArray, entry + PlayerOffset::k_Handle) == 0) continue;

                for (const std::uintptr_t field : { PlayerOffset::k_AliveBipedHandle, PlayerOffset::k_DeadBipedHandle,
                    PlayerOffset::k_CurrentBipedHandle, PlayerOffset::k_PrimaryWeaponHandle, PlayerOffset::k_SecondaryWeaponHandle,
                    PlayerOffset::k_ObjectiveHandle })
                {
                    count(snapshot.Read<std::uint32_t>(RegionId::PlayerArray, entry + field));
                }
            }
        }

        return audit;
    }

    // Copy of ReadBoneMatrixTable of ObjectTableService.cpp, into the store.
    // return: Whether it set a table.
    auto FillBones(const RawObject& datum, std::uint32_t handle, BoneMatricesStore& store) -> bool
    {
        const std::uint16_t size = datum.Read<std::uint16_t>(ObjeOffset::k_NodeMatricesSize);
        const std::uint16_t offset = datum.Read<std::uint16_t>(ObjeOffset::k_NodeMatricesOffset);

        if (offset == 0xFFFF || offset < k_MinDatumSize) return false;
        if (size == 0 || size % sizeof(BoneMatrix) != 0) return false;

        const std::uint32_t nodeCount = static_cast<std::uint32_t>(size / sizeof(BoneMatrix));
        if (nodeCount >= k_MaxNodes || nodeCount == 0) return false;

        BoneMatrixTable table;
        table.Matrices.resize(nodeCount);
        if (!datum.ReadRaw(offset, table.Matrices.data(), nodeCount * sizeof(BoneMatrix))) return false;

        store.Set(handle, std::move(table));
        return true;
    }

    // Copy of ReadDamageSectionTable of ObjectTableService.cpp, into the store.
    // return: Whether it set a table.
    auto FillDamage(const RawObject& datum, std::uint32_t handle, DamageSectionsStore& store) -> bool
    {
        DamageSectionTable table{};

        const std::uint16_t regionsSize = datum.Read<std::uint16_t>(ObjeOffset::k_DamageSectionsSize);
        const std::uint16_t regionsOffsetRaw = datum.Read<std::uint16_t>(ObjeOffset::k_DamageSectionsOffset);
        const std::uint16_t count = regionsSize / k_DamageSectionStride;
        if (regionsOffsetRaw != 0xFFFF && count != 0 && count < 256)
        {
            std::vector<DamageSection> raw(count);
            if (datum.ReadRaw(regionsOffsetRaw, raw.data(), count * sizeof(DamageSection)))
            {
                table.Sections.resize(count);
                for (std::uint16_t section = 0; section < count; ++section)
                {
                    table.Sections[section].DamageLevelMask = raw[section].DamageLevelMask;
                    table.Sections[section].Vitality = raw[section].Vitality;
                }
            }
        }

        const std::uint16_t size = datum.Read<std::uint16_t>(ObjeOffset::k_RegionStateSize);
        const std::uint16_t offset = datum.Read<std::uint16_t>(ObjeOffset::k_RegionStateOffset);
        if (offset != 0xFFFF && size != 0 && size <= k_RegionBlockMaxSize)
        {
            std::vector<std::uint8_t> bytes(size);
            if (datum.ReadRaw(offset, bytes.data(), size))
            {
                table.Regions.Size = size;
                table.Regions.Offset = offset;
                table.Regions.Bytes = std::move(bytes);
            }
        }

        if (table.Sections.empty() && table.Regions.Bytes.empty()) return false;

        store.Set(handle, std::move(table));
        return true;
    }

    auto TimeRow(std::string_view label, const Series& series) -> std::string
    {
        const Stats s{ series.Summary() };
        return std::format("{:<46} n={:<6} mean={:>9.1f} p50={:>9.1f} p99={:>9.1f} max={:>10.1f} us", label, s.N, s.Mean, s.P50, s.P99, s.Max);
    }

    auto CounterRow(std::string_view label, const Series& series) -> std::string
    {
        const Stats s{ series.Summary() };
        return std::format("{:<46} n={:<6} mean={:>9.2f} p99={:>9.1f} max={:>10.1f}", label, s.N, s.Mean, s.P99, s.Max);
    }

    class SnapshotBench
    {
    public:
        SnapshotBench() = default;
        ~SnapshotBench() = default;

        SnapshotBench(const SnapshotBench&) = delete;
        auto operator=(const SnapshotBench&) -> SnapshotBench& = delete;

        // Runs before the pipeline of a record: the measures that need the Snapshot and the state the previous record left.
        // return: The endStage that times the 24 stages.
        auto Before(const Chain& chain, const Snapshot& snapshot, std::size_t record) -> std::function<void()>
        {
            m_Snapshot = &snapshot;
            m_Alive.Add(static_cast<double>(snapshot.RawObjects.size()));
            m_IsReplicaPending = false;

            if ((record - 1) % k_EveryRecords == 0)
            {
                this->MeasureCapture(snapshot);
                this->MeasureObjectTable(chain, snapshot);
            }

            m_StageIndex = 0;
            m_Mark = steady_clock::now();
            return [this]() {
                const auto now{ steady_clock::now() };
                if (m_StageIndex < m_Stages.size()) m_Stages[m_StageIndex].Add(Micros(m_Mark, now));
                ++m_StageIndex;
                m_Mark = now;
            };
        }

        // Runs after the pipeline of a record: checks the copies of the ObjectTable stage against what the pipeline published.
        auto After(const Chain& chain) -> void
        {
            if (m_IsReplicaPending) this->VerifyObjectTable(chain);
        }

        auto Differences() const -> std::size_t { return m_CaptureDiffs + m_ObjectDiffs; }
        auto Notes() const -> const std::vector<std::string>& { return m_Notes; }
        auto Measured() const -> std::size_t { return m_C0.Summary().N; }
        auto AliveStats() const -> Stats { return m_Alive.Summary(); }

        // return: The report lines.
        auto Lines() const -> std::vector<std::string>
        {
            std::vector<std::string> lines{};
            const auto add{ [&](std::string text) { lines.push_back(std::move(text)); } };

            add(std::format("Capture, over fake memory built from each record: {} records measured.", m_C0.Summary().N));
            add(TimeRow("C0 Capture (production)", m_C0));
            add(TimeRow("C0 + destroying the previous Snapshot", m_C0Destroy));
            add(std::format("C0 check: {} Snapshots compared with the record (handles, KindId and bytes of every object, every region, the flags), {} differences.",
                m_CaptureCompared, m_CaptureDiffs));
            add("");
            add("C1 breakdown (a copy of the steps of Capture with a clock around each):");
            add(TimeRow("C1 make_shared<Snapshot>", m_C1Make));
            add(TimeRow("C1 objects (walk + vector + ReadRaw + emplace)", m_C1Objects));
            add(TimeRow("C1   of which the walk of the table alone", m_C1Walk));
            add(TimeRow("C1   of which one vector per object (resize)", m_C1Alloc));
            add(TimeRow("C1   of which ReadRaw into prepared buffers", m_C1ReadObjects));
            add(TimeRow("C1 regions (resize + ReadRaw, 9)", m_C1Regions));
            add(TimeRow("C1   of which ReadRaw into prepared buffers", m_C1ReadRegions));
            add(TimeRow("C1 destroy the previous Snapshot", m_C1Destroy));
            add(TimeRow("C1 sum of make + objects + regions", m_C1Sum));
            add(CounterRow("C1 objects copied", m_CObjects));
            add(CounterRow("C1 bytes of objects", m_CObjectBytes));
            add(CounterRow("C1 bytes of regions", m_CRegionBytes));
            add("");
            add("C2 reference (only the memcpy of the same bytes into prepared buffers):");
            add(TimeRow("C2 objects", m_C2Objects));
            add(TimeRow("C2 regions", m_C2Regions));
            add(TimeRow("C2 total", m_C2));
            add("");
            add("ObjectTable stage:");
            add(TimeRow("O0 stage ObjectTable (production, every record)", m_Stages[0]));
            add(std::format("O1 breakdown on a copy of the previous table, {} records measured:", m_O1Publish.Summary().N));
            add(TimeRow("O1 Clear of bones and damage stores", m_O1Clear));
            add(TimeRow("O1 DiscoverObjects (HasObject of each object)", m_O1Discover));
            add(TimeRow("O1 AuditHandles", m_O1Audit));
            add(TimeRow("O1 loop UpdateObjects (find + ResolveWorldOrigin)", m_O1Loop));
            add(TimeRow("O1 RemoveObject of the dead", m_O1Remove));
            add(TimeRow("O1 Publish (copy of the table)", m_O1Publish));
            add(TimeRow("O1 bones store: Set of a new vector per object", m_O1Bones));
            add(TimeRow("O1 damage store: Set of a new table per object", m_O1Damage));
            add(TimeRow("O1 sum of the parts above", m_O1Sum));
            add(TimeRow("O1 rest of O0 (UpdateObjectData etc.), O0 - sum", m_O1Rest));
            add(CounterRow("O1 alive objects in the table", m_OTable));
            add(CounterRow("O1 new objects of the tick", m_ONew));
            add(CounterRow("O1 dead objects of the tick", m_ODead));
            add(CounterRow("O1 bone tables set", m_OBoneSets));
            add(CounterRow("O1 damage tables set", m_ODamageSets));
            add(std::format("O1 check: {} tables compared (WorldOrigin, who is alive, audit, bone and damage tables), {} differences.", m_ObjectCompared, m_ObjectDiffs));
            add("O2 reference for Publish:");
            add(TimeRow("O2 copy of the table into an existing map", m_O2Assign));
            add(TimeRow("O2 publish a shared_ptr without copy", m_O2Pointer));
            add(TimeRow("O2 destroy a whole table (the previous one)", m_O2Destroy));
            add("");
            add("P0 the 24 stages of the pipeline, mean per tick, every record:");
            for (std::size_t index{ 0 }; index < k_StageCount; ++index) add(TimeRow(std::format("P0 {:02} {}", index, k_PipelineStageNames[index]), m_Stages[index]));
            add("");
            add(CounterRow("Alive objects of the Snapshot (every record)", m_Alive));

            return lines;
        }

        // return: The closing lines: the heaviest, and what S2.C and S2.O could save at most.
        auto Summary() const -> std::vector<std::string>
        {
            std::vector<std::string> lines{};
            std::vector<std::pair<std::string, double>> rows{};
            rows.emplace_back("Capture (C0)", m_C0.Summary().Mean);
            for (std::size_t index{ 0 }; index < k_StageCount; ++index) rows.emplace_back(std::format("stage {}", k_PipelineStageNames[index]), m_Stages[index].Summary().Mean);
            std::ranges::sort(rows, [](const auto& a, const auto& b) { return a.second > b.second; });

            lines.push_back("Heaviest per tick (Capture and the 24 stages), mean us:");
            for (std::size_t index{ 0 }; index < (std::min)(rows.size(), std::size_t{ 5 }); ++index) lines.push_back(std::format("  {:<28} {:>9.1f}", rows[index].first, rows[index].second));

            double total{ m_C0.Summary().Mean };
            for (const Series& stage : m_Stages) total += stage.Summary().Mean;
            lines.push_back(std::format("Capture + 24 stages: {:.1f} us. Capture is {:.1f} %, stage ObjectTable {:.1f} %.", total,
                100.0 * m_C0.Summary().Mean / total, 100.0 * m_Stages[0].Summary().Mean / total));

            const double c0{ m_C0.Summary().Mean };
            const double c2{ m_C2.Summary().Mean };
            lines.push_back(std::format("C0 - C2 (the most S2.C can save): {:.1f} - {:.1f} = {:.1f} us per tick ({:.1f} % of Capture).", c0, c2, c0 - c2, 100.0 * (c0 - c2) / c0));

            const double o0{ m_Stages[0].Summary().Mean };
            const double publish{ m_O1Publish.Summary().Mean };
            const double pointer{ m_O2Pointer.Summary().Mean };
            lines.push_back(std::format("O0 - O2 (the most S2.O can save in Publish): Publish {:.1f} - pointer {:.1f} = {:.1f} us per tick, of O0 {:.1f} us ({:.1f} %).",
                publish, pointer, publish - pointer, o0, 100.0 * (publish - pointer) / o0));
            return lines;
        }

    private:
        CollectingSink m_Sink{};
        Log m_Log{ m_Sink };
        LifecycleStore m_Lifecycle{};
        MemoryReaderService m_Reader{};
        CaptureStore m_Store{};
        TelemetryStore m_Telemetry{};
        CaptureService m_Service{ m_Log, m_Lifecycle, m_Reader, m_Store, m_Telemetry };
        FakeEngineMemory m_Memory{};

        const Snapshot* m_Snapshot{};
        steady_clock::time_point m_Mark{};
        std::size_t m_StageIndex{};
        std::array<Series, k_StageCount> m_Stages{};
        Series m_Alive{};

        // Capture.
        std::shared_ptr<const Snapshot> m_PreviousCaptured{};
        std::shared_ptr<Snapshot> m_PreviousReplica{};
        std::vector<std::uint8_t> m_ObjectBuffer{};
        std::vector<std::uint8_t> m_RegionBuffer{};
        std::size_t m_CaptureCompared{};
        std::size_t m_CaptureDiffs{};
        Series m_C0{}, m_C0Destroy{};
        Series m_C1Make{}, m_C1Objects{}, m_C1Walk{}, m_C1Alloc{}, m_C1ReadObjects{}, m_C1Regions{}, m_C1ReadRegions{}, m_C1Destroy{}, m_C1Sum{};
        Series m_C2Objects{}, m_C2Regions{}, m_C2{};
        Series m_CObjects{}, m_CObjectBytes{}, m_CRegionBytes{};

        // ObjectTable.
        BoneMatricesStore m_ReplicaBones{};
        DamageSectionsStore m_ReplicaDamage{};
        ObjectTable m_O2Target{};
        std::atomic<std::shared_ptr<const ObjectTable>> m_O2Sink{};
        bool m_IsReplicaPending{};
        std::shared_ptr<const ObjectTable> m_ReplicaPrevious{};
        std::shared_ptr<const ObjectTable> m_ReplicaPublished{};
        HandleAudit m_ReplicaAudit{};
        std::vector<std::uint32_t> m_ReplicaList{};
        std::size_t m_ReplicaBoneSets{};
        std::size_t m_ReplicaDamageSets{};
        std::size_t m_ObjectCompared{};
        std::size_t m_ObjectDiffs{};
        Series m_O1Clear{}, m_O1Discover{}, m_O1Audit{}, m_O1Loop{}, m_O1Remove{}, m_O1Publish{}, m_O1Bones{}, m_O1Damage{}, m_O1Sum{}, m_O1Rest{};
        Series m_OTable{}, m_ONew{}, m_ODead{}, m_OBoneSets{}, m_ODamageSets{};
        Series m_O2Assign{}, m_O2Pointer{}, m_O2Destroy{};
        std::vector<std::string> m_Notes{};

        auto Note(std::string text) -> void
        {
            if (m_Notes.size() < 20) m_Notes.push_back(std::move(text));
        }

        // Slots of the table to walk, as ObjectSlotLimit of CaptureService.cpp.
        auto SlotLimit() -> std::uint32_t
        {
            const std::uintptr_t objectArray{ m_Reader.Read<std::uintptr_t>(m_Store.GetThreadContext(), k_ThreadContextObjectDataOffset) };
            const std::int32_t capacity{ m_Reader.Read<std::int32_t>(objectArray, k_DataArrayCapacity) };
            const std::int32_t firstUnused{ m_Reader.Read<std::int32_t>(objectArray, k_DataArrayFirstUnusedIndex) };

            std::uint32_t limit{ k_MaxObjects };
            if (capacity > 0) limit = (std::min)(limit, static_cast<std::uint32_t>(capacity));
            if (firstUnused > 0) limit = (std::min)(limit, static_cast<std::uint32_t>(firstUnused));
            return limit;
        }

        // Walks the table as CopyObjects does and calls visit(handle, address, datumSize, kindId) for each live entry.
        template <typename Visit>
        auto WalkTable(Visit&& visit) -> void
        {
            const std::uintptr_t tableBase{ m_Store.GetObjectTableBase() };
            const std::uint32_t slotLimit{ this->SlotLimit() };
            for (std::uint32_t index{ 0 }; index < slotLimit; ++index)
            {
                const std::uintptr_t entryAddress{ tableBase + (index * k_EntrySize) };

                const std::uint16_t salt{ m_Reader.Read<std::uint16_t>(entryAddress, k_EntrySaltOffset) };
                if (salt == 0) continue;

                const std::uintptr_t address{ m_Reader.Read<std::uintptr_t>(entryAddress, k_EntryAddressOffset) };
                if (address == 0) continue;

                const std::uint16_t datumSize{ m_Reader.Read<std::uint16_t>(entryAddress, k_EntryDatumSizeOffset) };
                if (datumSize < k_MinDatumSize) continue;

                const std::uint8_t kindId{ m_Reader.Read<std::uint8_t>(entryAddress, k_EntryKindOffset) };
                visit((static_cast<std::uint32_t>(salt) << 16) | index, address, datumSize, kindId);
            }
        }

        auto VerifyCapture(const Snapshot& record, const Snapshot& captured) -> void
        {
            ++m_CaptureCompared;
            std::size_t before{ m_CaptureDiffs };

            if (captured.HasThreadContext != record.HasThreadContext) ++m_CaptureDiffs;
            if (captured.HasObjectTable != record.HasObjectTable) ++m_CaptureDiffs;
            if (captured.RawObjects.size() != record.RawObjects.size()) ++m_CaptureDiffs;
            for (const auto& [handle, raw] : record.RawObjects)
            {
                const auto it{ captured.RawObjects.find(handle) };
                if (it == captured.RawObjects.end() || it->second.Handle != handle || it->second.KindId != raw.KindId || it->second.Bytes != raw.Bytes) ++m_CaptureDiffs;
            }
            for (std::size_t id{ 0 }; id < k_RegionCount; ++id)
            {
                if (captured.Regions[id].Bytes != record.Regions[id].Bytes) ++m_CaptureDiffs;
            }

            if (m_CaptureDiffs != before) this->Note(std::format("Capture differs from the record in {} things (record {}).", m_CaptureDiffs - before, m_CaptureCompared));
        }

        // C0, C1 and C2 for the Snapshot of this record.
        auto MeasureCapture(const Snapshot& snapshot) -> void
        {
            m_Memory.Load(snapshot, m_Store);

            std::size_t objectBytes{};
            for (const auto& object : m_Memory.m_Live) objectBytes += object.Size;
            std::size_t regionBytes{};
            for (const auto& source : m_Memory.m_Sources) regionBytes += source.Size;
            m_CObjects.Add(static_cast<double>(m_Memory.m_Live.size()));
            m_CObjectBytes.Add(static_cast<double>(objectBytes));
            m_CRegionBytes.Add(static_cast<double>(regionBytes));
            m_ObjectBuffer.resize(objectBytes);
            m_RegionBuffer.resize(regionBytes);

            // C0: the production call.
            auto start{ steady_clock::now() };
            std::shared_ptr<const Snapshot> captured{ m_Service.Capture() };
            auto end{ steady_clock::now() };
            m_C0.Add(Micros(start, end));
            this->VerifyCapture(snapshot, *captured);

            // The store held the previous one until Capture published: this reference is the last, so this is its destruction.
            const double c0Us{ Micros(start, end) };
            start = steady_clock::now();
            m_PreviousCaptured = std::move(captured);
            end = steady_clock::now();
            m_C0Destroy.Add(c0Us + Micros(start, end));

            // C1: the steps of Capture.
            start = steady_clock::now();
            auto replica{ std::make_shared<Snapshot>() };
            end = steady_clock::now();
            const double makeUs{ Micros(start, end) };
            m_C1Make.Add(makeUs);

            start = steady_clock::now();
            this->CopyObjectsReplica(*replica);
            end = steady_clock::now();
            const double objectsUs{ Micros(start, end) };
            m_C1Objects.Add(objectsUs);

            start = steady_clock::now();
            this->CopyRegionsReplica(*replica);
            end = steady_clock::now();
            const double regionsUs{ Micros(start, end) };
            m_C1Regions.Add(regionsUs);

            start = steady_clock::now();
            m_PreviousReplica = std::move(replica);
            end = steady_clock::now();
            m_C1Destroy.Add(Micros(start, end));
            m_C1Sum.Add(makeUs + objectsUs + regionsUs);

            // The parts of the objects.
            std::size_t live{};
            start = steady_clock::now();
            this->WalkTable([&](std::uint32_t, std::uintptr_t, std::uint16_t, std::uint8_t) { ++live; });
            end = steady_clock::now();
            m_C1Walk.Add(Micros(start, end));

            {
                std::vector<std::vector<std::uint8_t>> vectors(m_Memory.m_Live.size());
                start = steady_clock::now();
                for (std::size_t index{ 0 }; index < vectors.size(); ++index) vectors[index].resize(m_Memory.m_Live[index].Size);
                end = steady_clock::now();
                m_C1Alloc.Add(Micros(start, end));
            }

            std::size_t used{};
            start = steady_clock::now();
            for (const auto& object : m_Memory.m_Live)
            {
                m_Reader.ReadRaw(object.Address, m_ObjectBuffer.data() + used, object.Size);
                used += object.Size;
            }
            end = steady_clock::now();
            m_C1ReadObjects.Add(Micros(start, end));

            used = 0;
            start = steady_clock::now();
            for (const auto& source : m_Memory.m_Sources)
            {
                if (source.Address == 0) continue;
                m_Reader.ReadRaw(source.Address, m_RegionBuffer.data() + used, source.Size);
                used += source.Size;
            }
            end = steady_clock::now();
            m_C1ReadRegions.Add(Micros(start, end));

            // C2: memcpy into the same prepared buffers.
            used = 0;
            start = steady_clock::now();
            for (const auto& object : m_Memory.m_Live)
            {
                std::memcpy(m_ObjectBuffer.data() + used, reinterpret_cast<const void*>(object.Address), object.Size);
                used += object.Size;
            }
            end = steady_clock::now();
            m_C2Objects.Add(Micros(start, end));

            used = 0;
            const auto startRegions{ steady_clock::now() };
            for (const auto& source : m_Memory.m_Sources)
            {
                if (source.Address == 0) continue;
                std::memcpy(m_RegionBuffer.data() + used, reinterpret_cast<const void*>(source.Address), source.Size);
                used += source.Size;
            }
            const auto endRegions{ steady_clock::now() };
            m_C2Regions.Add(Micros(startRegions, endRegions));
            m_C2.Add(Micros(start, endRegions));
        }

        // C1 objects: a copy of CopyObjects of CaptureService.cpp.
        auto CopyObjectsReplica(Snapshot& snapshot) -> void
        {
            this->WalkTable([&](std::uint32_t handle, std::uintptr_t address, std::uint16_t datumSize, std::uint8_t kindId) {
                RawObject object{};
                object.Handle = handle;
                object.Address = address;
                object.KindId = kindId;

                object.Bytes.resize(datumSize);
                if (!m_Reader.ReadRaw(address, object.Bytes.data(), datumSize)) return;

                snapshot.RawObjects.emplace(object.Handle, std::move(object));
            });
        }

        // C1 regions: a copy of CopyRegion of CaptureService.cpp for each region the fake memory holds.
        auto CopyRegionsReplica(Snapshot& snapshot) -> void
        {
            for (std::size_t id{ 0 }; id < k_RegionCount; ++id)
            {
                const std::uintptr_t address{ m_Memory.m_Sources[id].Address };
                if (address == 0) continue;

                Region& region{ snapshot.Regions[id] };
                region.OriginalAddress = address;
                region.Bytes.resize(RegionSize(static_cast<RegionId>(id)));

                if (!m_Reader.ReadRaw(address, region.Bytes.data(), region.Bytes.size()))
                {
                    region.Bytes.clear();
                    region.OriginalAddress = 0;
                }
            }
        }

        // O1 and O2 on a copy of the table the previous record left, for the Snapshot of this record. The chain is not touched.
        auto MeasureObjectTable(const Chain& chain, const Snapshot& snapshot) -> void
        {
            m_ReplicaPrevious = chain.m_Tables.m_ObjectStore.Acquire();
            if (!m_ReplicaPrevious || m_ReplicaPrevious->empty() || !snapshot.HasObjectTable) return;

            // The store as it is when UpdateObjectTable begins.
            ObjectTableStore store{};
            for (const auto& [handle, object] : *m_ReplicaPrevious) store.AddObject(handle, object);

            auto start{ steady_clock::now() };
            m_ReplicaBones.Clear();
            m_ReplicaDamage.Clear();
            auto end{ steady_clock::now() };
            const double clearUs{ Micros(start, end) };
            m_O1Clear.Add(clearUs);

            std::size_t added{};
            start = steady_clock::now();
            for (const auto& [handle, datum] : snapshot.RawObjects)
            {
                if (!store.HasObject(handle)) ++added;
            }
            end = steady_clock::now();
            const double discoverUs{ Micros(start, end) };
            m_O1Discover.Add(discoverUs);

            start = steady_clock::now();
            m_ReplicaAudit = AuditHandles(snapshot);
            end = steady_clock::now();
            const double auditUs{ Micros(start, end) };
            m_O1Audit.Add(auditUs);

            std::vector<std::uint32_t> dead{};
            start = steady_clock::now();
            store.UpdateObjects([&](std::uint32_t handle, AliveObject& object) {
                const auto it{ snapshot.RawObjects.find(handle) };
                if (it == snapshot.RawObjects.end())
                {
                    dead.push_back(handle);
                    return;
                }

                object.WorldOrigin = ResolveWorldOrigin(snapshot.RawObjects, it->second);
            });
            end = steady_clock::now();
            const double loopUs{ Micros(start, end) };
            m_O1Loop.Add(loopUs);

            start = steady_clock::now();
            for (const std::uint32_t handle : dead) store.RemoveObject(handle);
            end = steady_clock::now();
            const double removeUs{ Micros(start, end) };
            m_O1Remove.Add(removeUs);

            start = steady_clock::now();
            store.Publish();
            end = steady_clock::now();
            const double publishUs{ Micros(start, end) };
            m_O1Publish.Add(publishUs);
            m_ReplicaPublished = store.Acquire();

            // The objects the stores of bones and damage get tables for: the ones of the table that the Snapshot still has.
            std::vector<const RawObject*> raws{};
            m_ReplicaList.clear();
            for (const auto& [handle, object] : *m_ReplicaPublished)
            {
                const auto it{ snapshot.RawObjects.find(handle) };
                if (it == snapshot.RawObjects.end()) continue;

                raws.push_back(&it->second);
                m_ReplicaList.push_back(handle);
            }

            m_ReplicaBoneSets = 0;
            start = steady_clock::now();
            for (std::size_t index{ 0 }; index < raws.size(); ++index) m_ReplicaBoneSets += FillBones(*raws[index], m_ReplicaList[index], m_ReplicaBones) ? 1 : 0;
            end = steady_clock::now();
            const double bonesUs{ Micros(start, end) };
            m_O1Bones.Add(bonesUs);

            m_ReplicaDamageSets = 0;
            start = steady_clock::now();
            for (std::size_t index{ 0 }; index < raws.size(); ++index) m_ReplicaDamageSets += FillDamage(*raws[index], m_ReplicaList[index], m_ReplicaDamage) ? 1 : 0;
            end = steady_clock::now();
            const double damageUs{ Micros(start, end) };
            m_O1Damage.Add(damageUs);

            // The bones and damage sets happen inside the loop in production: the sum counts them once, with the loop of the replica.
            m_O1Sum.Add(clearUs + discoverUs + auditUs + loopUs + removeUs + publishUs + bonesUs + damageUs);
            m_O1Rest.Add(m_Stages[0].Summary().N == 0 ? 0.0 : m_Stages[0].Summary().Mean - (clearUs + discoverUs + auditUs + loopUs + removeUs + publishUs + bonesUs + damageUs));

            m_OTable.Add(static_cast<double>(m_ReplicaPrevious->size()));
            m_ONew.Add(static_cast<double>(added));
            m_ODead.Add(static_cast<double>(dead.size()));
            m_OBoneSets.Add(static_cast<double>(m_ReplicaBoneSets));
            m_ODamageSets.Add(static_cast<double>(m_ReplicaDamageSets));

            // O2: the same table copied onto a map that already exists, and published as a pointer.
            start = steady_clock::now();
            m_O2Target = *m_ReplicaPrevious;
            end = steady_clock::now();
            m_O2Assign.Add(Micros(start, end));

            // Dropping the table of the last measure is a cost of its own (the destruction of a whole table): it is timed apart.
            start = steady_clock::now();
            m_O2Sink.store(nullptr, std::memory_order_release);
            end = steady_clock::now();
            m_O2Destroy.Add(Micros(start, end));

            start = steady_clock::now();
            m_O2Sink.store(m_ReplicaPrevious, std::memory_order_release);
            end = steady_clock::now();
            m_O2Pointer.Add(Micros(start, end));

            m_IsReplicaPending = true;
        }

        // Compares what the replica of the ObjectTable stage got with what the pipeline published for this record.
        auto VerifyObjectTable(const Chain& chain) -> void
        {
            ++m_ObjectCompared;
            const std::size_t before{ m_ObjectDiffs };
            const auto published{ chain.m_Tables.m_ObjectStore.Acquire() };
            if (!published)
            {
                ++m_ObjectDiffs;
                return;
            }

            const HandleAudit audit{ chain.m_Tables.m_ObjectService.GetHandleAudit() };
            if (audit.Checked != m_ReplicaAudit.Checked || audit.Matching != m_ReplicaAudit.Matching || audit.Stale != m_ReplicaAudit.Stale ||
                audit.Free != m_ReplicaAudit.Free || audit.ZeroSalt != m_ReplicaAudit.ZeroSalt) ++m_ObjectDiffs;

            for (const auto& [handle, object] : *m_ReplicaPublished)
            {
                const auto it{ published->find(handle) };
                if (it == published->end()) ++m_ObjectDiffs;
                else if (it->second.WorldOrigin.X != object.WorldOrigin.X || it->second.WorldOrigin.Y != object.WorldOrigin.Y ||
                    it->second.WorldOrigin.Z != object.WorldOrigin.Z) ++m_ObjectDiffs;
            }

            // Who is alive: what the Snapshot has stays, what it lost goes, and the rest of the published table is new.
            for (const auto& [handle, object] : *m_ReplicaPrevious)
            {
                if (published->contains(handle) != m_Snapshot->RawObjects.contains(handle)) ++m_ObjectDiffs;
            }
            for (const auto& [handle, object] : *published)
            {
                if (!m_ReplicaPrevious->contains(handle) && !m_Snapshot->RawObjects.contains(handle)) ++m_ObjectDiffs;
            }

            std::size_t bones{};
            std::size_t damage{};
            for (const std::uint32_t handle : m_ReplicaList)
            {
                if (chain.m_Tables.m_BoneMatricesStore.Get(handle) != nullptr) ++bones;
                if (chain.m_Tables.m_DamageSectionsStore.Get(handle) != nullptr) ++damage;
            }
            if (bones != m_ReplicaBoneSets || damage != m_ReplicaDamageSets) ++m_ObjectDiffs;

            if (m_ObjectDiffs != before) this->Note(std::format("The ObjectTable replica differs from the pipeline in {} things (record {}).", m_ObjectDiffs - before, m_ObjectCompared));
        }
    };

    // Replays the film, measures, and writes the report to build/bench/snapshot_<film>.txt of the repository.
    auto RunBench(std::string_view film) -> void
    {
        SnapshotBench bench{};
        const FilmOutcome outcome{ RunFilmPass(film, false,
            [&](const Chain& chain, std::size_t) { bench.After(chain); },
            [&](const Chain& chain, const Snapshot& snapshot, std::size_t record) { return bench.Before(chain, snapshot, record); }) };
        if (!outcome.Skip.empty()) GTEST_SKIP() << outcome.Skip;
        ASSERT_TRUE(outcome.Error.empty()) << outcome.Error;

        const Stats alive{ bench.AliveStats() };
        const auto now{ std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()) };
        std::vector<std::string> report{};
        report.push_back(std::format("Snapshot bench: film {}, {} records ({} measured with every part, K = {}), {:%Y-%m-%d %H:%M:%S} UTC, {}.",
            film, outcome.Records, bench.Measured(), k_EveryRecords, now, Tests::Support::Bench::Configuration()));
        report.push_back(std::format("Pass: {:.1f} s. Alive objects per Snapshot: mean {:.1f}, max {:.0f}. Times in microseconds per tick.",
            outcome.Seconds, alive.Mean, alive.Max));
        report.push_back("The fake memory is built outside the measured time and is warm in the cache: C0 is a lower bound of the capture of the game.");
        report.push_back("O1 Clear includes destroying what the previous measure left (the old tables), as production does when it replaces them.");
        report.push_back("");
        for (std::string& line : bench.Lines()) report.push_back(std::move(line));
        report.push_back("");
        for (std::string& line : bench.Summary()) report.push_back(std::move(line));
        report.push_back(std::format("Differences found in the checks: {}.", bench.Differences()));
        for (const std::string& note : bench.Notes()) report.push_back("  " + note);

        const std::filesystem::path file{ Tests::Support::Bench::WriteReport(std::format("snapshot_{}.txt", film), report) };
        ASSERT_FALSE(file.empty()) << "Cannot write the report of " << film;

        ::testing::Test::RecordProperty("ReportFile", file.string());
        for (std::size_t index{ 0 }; index < report.size(); ++index)
        {
            if (!report[index].empty()) ::testing::Test::RecordProperty(std::format("Line{:02}", index), report[index]);
        }
        EXPECT_GT(outcome.Records, 0u);
        EXPECT_EQ(bench.Differences(), 0u);
    }
}

TEST(DISABLED_SnapshotBench, CtfD)
{
    RunBench("ctf_d");
}

TEST(DISABLED_SnapshotBench, CtfC)
{
    RunBench("ctf_c");
}