#include <gtest/gtest.h>

import Contract.Bridge.Type;
import Bridge.Codec.Dump.System;
import Bridge.Codec.Layer;
import Tests.Support.FilmPass;
import Tests.Support.Pipeline;
import std;

// Q101: the sparse mode against the full one over the golden films. The full pass runs every record through the pipeline. The sparse pass runs
// the decision records (the engine tick is a multiple of the decision period) and the first record after the load through the pipeline, and
// the records in between through MakeLight and RunLight, as the AI thread does. On each decision record the Tick of both passes is compared
// field by field, with the layout of the bridge: every field is equal except the ones of the tolerance list, which have a cap of differences
// and a bound on the size of each difference (the magnitude checks of TickComparer::CheckMagnitudes).

namespace
{
    using Contract::Bridge::Type::BridgeGround;
    using Contract::Bridge::Type::BridgeLift;
    using Contract::Bridge::Type::BridgePlayer;
    using Contract::Bridge::Type::BridgeTick;
    using Contract::Bridge::Type::BridgeWeapon;
    using Contract::Bridge::Type::k_BridgeLayoutJson;
    using Tests::Support::FilmPass::FilmOutcome;
    using Tests::Support::FilmPass::FilmSparse;
    using Tests::Support::FilmPass::RecordingsFolder;
    using Tests::Support::FilmPass::RunFilmPass;
    using Tests::Support::Pipeline::Chain;

    namespace TickCodec = Bridge::Codec::Dump::System;

    // Decision period of the pass, as the one Python asks for.
    constexpr std::size_t k_DecisionPeriod{ 4 };

    // Ticks a sampled value may be off: the interval between two decisions hides at most period - 1 ticks.
    constexpr std::uint32_t k_MaxTickShift{ k_DecisionPeriod - 1 };

    // Largest excess of the sparse AirborneTicks over the full one that the films gave (386, suelo), times about 1.5.
    constexpr std::uint32_t k_MaxAirborneExcess{ 600 };

    // Relative slack on the charge bound, for the rounding of the float rates.
    constexpr double k_ChargeSlack{ 1.001 };

    // Samples kept of the differences of each field.
    constexpr std::size_t k_FirstCases{ 3 };

    // A field that may differ between the passes, and how many values of it may differ in one film (all the decisions, all the elements).
    // The cap is secondary: how much each value may differ is checked by TickComparer::CheckMagnitudes.
    struct Tolerance
    {
        std::string_view Field{};
        std::size_t Cap{};
    };

    // Fields of the tolerance list, as Struct.Field of the layout (a nested struct counts under the field that holds it). The caps are about 1.5 times
    // the most any film gave (armas_a, cgb_displace_1min, ctf_c, ctf_d, explosivos, lifts, powerups, suelo, armas_b). Why each one may differ:
    // - AirborneTicks, Ground, GroundObject, IsGrounded: the first sample without contact dates the takeoff in the sample (the contact may have
    //   ended in any tick of the interval) and the ground is held from there, so the hold ends up to period - 1 ticks off. A contact of 1 or 2
    //   ticks between two samples is never seen by the sparse pass, which then keeps counting where the full pass restarts.
    // - GroundObject: the last object seen under the feet, history the sparse pass only has in part.
    // - OwnVelocity: it is relative to the object under the feet, so it follows Ground and GroundObject.
    // - Lift ExitTick and ExitVelocity: a biped that enters and leaves a lift between two samples is dated and measured in the sample of the
    //   exit. The exit stays in the lift, so one difference is repeated in every later decision.
    // - ChargeFraction: a charge is seen first in the sample, so it under-estimates by up to period - 1 ticks.
    // Nothing of the events, the flags, the match, the entities or the header differs: they are not in the list.
    const std::vector<Tolerance> k_Tolerances{
        { "BridgePlayer.AirborneTicks", 2'700 },
        { "BridgePlayer.Ground", 45 },
        { "BridgePlayer.GroundObject", 55 },
        { "BridgePlayer.IsGrounded", 45 },
        { "BridgePlayer.OwnVelocity", 15 },
        { "BridgeLift.ExitTick", 14'200 },
        { "BridgeLift.ExitVelocity", 41'200 },
        { "BridgeWeapon.ChargeFraction", 75 } };

    // One field of the layout.
    struct LayoutField
    {
        std::string Name{};
        std::string Type{};
        std::size_t Offset{};
        std::size_t Count{};
    };

    struct LayoutStruct
    {
        std::string Name{};
        std::size_t Size{};
        std::vector<LayoutField> Fields{};
    };

    // return: The text between the quotes that start at the position, and moves the position after them.
    auto ReadQuoted(const std::string_view text, std::size_t& position) -> std::string
    {
        const std::size_t start{ text.find('"', position) + 1 };
        const std::size_t end{ text.find('"', start) };
        position = end + 1;
        return std::string{ text.substr(start, end - start) };
    }

    // return: The number that follows the key.
    auto ReadNumberAfter(const std::string_view text, std::string_view key, std::size_t position) -> std::size_t
    {
        const std::size_t start{ text.find(key, position) + key.size() };
        std::size_t value{};
        std::from_chars(text.data() + start, text.data() + text.size(), value);
        return value;
    }

    // return: The structs of the layout the contract carries (the JSON of GenerateBridge.py), by name.
    auto ParseLayout() -> std::map<std::string, LayoutStruct>
    {
        std::map<std::string, LayoutStruct> structs{};
        const std::string_view json{ k_BridgeLayoutJson };

        std::size_t position{ json.find("\"structs\":[") };
        while ((position = json.find("{\"name\":\"", position)) != std::string_view::npos)
        {
            LayoutStruct layout{};
            std::size_t cursor{ position + 8 };
            layout.Name = ReadQuoted(json, cursor);
            layout.Size = ReadNumberAfter(json, "\"size\":", cursor);

            const std::size_t fieldsStart{ json.find("\"fields\":[", cursor) + 10 };
            const std::size_t fieldsEnd{ json.find("]]}", fieldsStart) + 1 };
            std::size_t fieldPosition{ fieldsStart };
            while ((fieldPosition = json.find('[', fieldPosition)) != std::string_view::npos && fieldPosition < fieldsEnd)
            {
                LayoutField field{};
                ++fieldPosition;
                field.Name = ReadQuoted(json, fieldPosition);
                field.Type = ReadQuoted(json, fieldPosition);
                std::from_chars(json.data() + fieldPosition + 1, json.data() + json.size(), field.Offset);
                const std::size_t countStart{ json.find(',', fieldPosition + 1) + 1 };
                std::from_chars(json.data() + countStart, json.data() + json.size(), field.Count);
                fieldPosition = json.find(']', fieldPosition);
                layout.Fields.push_back(std::move(field));
            }

            structs[layout.Name] = std::move(layout);
            position = fieldsEnd;
        }

        return structs;
    }

    // return: The size of a primitive of the layout, 0 when the type is not one.
    auto PrimitiveSize(const std::string& type) -> std::size_t
    {
        if (type == "u8" || type == "i8") return 1;
        if (type == "u16" || type == "i16") return 2;
        if (type == "u32" || type == "i32" || type == "f32") return 4;
        if (type == "u64") return 8;
        return 0;
    }

    // The differences of one field over a film.
    struct FieldDifference
    {
        std::size_t Count{};
        std::vector<std::string> First{};
    };

    // What the comparison of a film found.
    struct SparseReport
    {
        std::size_t Decisions{};
        std::map<std::string, FieldDifference> Differences{};
        // Largest size of a difference by field, in the unit of the check: ticks (AirborneTicks, ExitTick, ChargeFraction in ticks of charge).
        std::map<std::string, double> MaxMagnitude{};
        // Differences that break a magnitude check: how many, and the first ones.
        std::size_t ViolationCount{};
        std::map<std::string, std::size_t> ViolationKinds{};
        std::vector<std::string> Violations{};
        // Charge differences bounded with the fastest rate of the film because the weapon had no rate of its own.
        std::size_t ChargeWithoutOwnRate{};
        std::size_t ChargeDifferences{};
    };

    // A charge difference between the passes, checked at the end of the film when the rate of its weapon is known.
    struct ChargeCase
    {
        std::uint32_t Handle{};
        std::size_t Record{};
        float Full{};
        float Sparse{};
    };

    // The charge of a weapon in the previous decision of the full pass.
    struct ChargeSample
    {
        float Fraction{};
        std::uint32_t Tick{};
    };

    // Which header field holds the number of rows of each array of the tick.
    const std::map<std::string, std::string, std::less<>> k_CountFields{
        { "Players", "PlayerCount" }, { "Entities", "EntityCount" }, { "Objectives", "ObjectiveCount" }, { "Stands", "StandCount" },
        { "Events", "EventCount" }, { "Weapons", "WeaponCount" }, { "Projectiles", "ProjectileCount" }, { "Explosives", "ExplosiveCount" },
        { "Lifts", "LiftCount" } };

    class TickComparer
    {
    public:
        TickComparer() : m_Structs(ParseLayout()), m_Full(std::make_unique<BridgeTick>()), m_Sparse(std::make_unique<BridgeTick>()) {}

        // Flattens the Tick of both chains and compares them field by field.
        auto Compare(Chain& full, Chain& sparse, std::size_t record) -> void
        {
            this->Flatten(full, *m_Full);
            this->Flatten(sparse, *m_Sparse);
            m_Record = record;
            ++m_Report.Decisions;

            this->CompareStruct("BridgeTick", reinterpret_cast<const char*>(m_Full.get()), reinterpret_cast<const char*>(m_Sparse.get()), "Tick", {}, true);
            this->CheckMagnitudes();
        }

        // Checks the charge differences of the film against the rate of charge each weapon showed in the full pass.
        auto Report() -> SparseReport&
        {
            this->CheckCharges();
            return m_Report;
        }

    private:
        static auto Flatten(Chain& chain, BridgeTick& flat) -> void
        {
            const auto tick{ chain.m_Export.m_TickStore.Acquire() };
            TickCodec::ClearHeader(flat);
            chain.m_Export.m_FlatService.Flatten(*tick, flat);
        }

        // Compares the fields of a struct at the bytes of both ticks. In the tick, the arrays only hold as many rows as the header says.
        // param key: Struct.Field the differences of a nested struct (BridgeVec3, BridgeMagazine...) count under: the field of the row that holds it.
        // Empty for the fields of the rows of the tick, which count under their own struct.
        auto CompareStruct(const std::string& name, const char* full, const char* sparse, const std::string& path, const std::string& key, bool isTick) -> void
        {
            const LayoutStruct& layout{ m_Structs.at(name) };
            for (const LayoutField& field : layout.Fields)
            {
                std::size_t count{ field.Count };
                if (isTick)
                {
                    if (const auto it{ k_CountFields.find(field.Name) }; it != k_CountFields.end()) count = std::min(count, this->HeaderCount(it->second));
                }

                const std::size_t primitive{ PrimitiveSize(field.Type) };
                for (std::size_t index{ 0 }; index < count; ++index)
                {
                    const std::string element{ std::format("{}.{}{}", path, field.Name, field.Count > 1 ? std::format("[{}]", index) : std::string{}) };
                    if (primitive == 0)
                    {
                        const LayoutStruct& inner{ m_Structs.at(field.Type) };
                        const std::size_t offset{ field.Offset + index * inner.Size };
                        const std::string innerKey{ isTick ? std::string{} : key.empty() ? std::format("{}.{}", name, field.Name) : key };
                        this->CompareStruct(field.Type, full + offset, sparse + offset, element, innerKey, false);
                        continue;
                    }

                    const std::size_t offset{ field.Offset + index * primitive };
                    if (std::memcmp(full + offset, sparse + offset, primitive) == 0) continue;

                    FieldDifference& difference{ m_Report.Differences[key.empty() ? std::format("{}.{}", name, field.Name) : key] };
                    ++difference.Count;
                    if (difference.First.size() < k_FirstCases)
                    {
                        std::uint64_t a{};
                        std::uint64_t b{};
                        std::memcpy(&a, full + offset, primitive);
                        std::memcpy(&b, sparse + offset, primitive);
                        difference.First.push_back(std::format("record {} {}: full {:#x} sparse {:#x}", m_Record, element, a, b));
                    }
                }
            }
        }

        // Records the largest size of the difference of a field.
        auto Observe(const std::string& field, double magnitude) -> void
        {
            double& largest{ m_Report.MaxMagnitude[field] };
            largest = (std::max)(largest, magnitude);
        }

        auto Violate(const std::string& kind, std::string text) -> void
        {
            ++m_Report.ViolationCount;
            ++m_Report.ViolationKinds[kind];
            if (m_Report.Violations.size() < k_FirstCases * 3) m_Report.Violations.push_back(std::format("record {}: {}", m_Record, text));
        }

        static auto Distance(std::uint32_t a, std::uint32_t b) -> std::uint32_t
        {
            return a > b ? a - b : b - a;
        }

        // Bounds the size of the differences of the tolerance list on the decision just compared (the rows are equal elsewhere, as CompareStruct checks).
        // - AirborneTicks and the lift ExitTick: at most period - 1 ticks apart.
        // - Ground and IsGrounded: only on a player whose AirborneTicks differs in this tick. GroundObject also when both stand on objects, and
        //   OwnVelocity also when GroundObject differs.
        // - ExitVelocity: only on a lift whose ExitTick differs.
        // - ChargeFraction: collected for CheckCharges.
        auto CheckMagnitudes() -> void
        {
            const BridgeTick& full{ *m_Full };
            const BridgeTick& sparse{ *m_Sparse };

            const std::size_t players{ (std::min)({ static_cast<std::size_t>(full.Header.PlayerCount), static_cast<std::size_t>(sparse.Header.PlayerCount), full.Players.size() }) };
            for (std::size_t index{ 0 }; index < players; ++index)
            {
                const BridgePlayer& a{ full.Players[index] };
                const BridgePlayer& b{ sparse.Players[index] };

                // The sparse pass is never lower than the full one by more than the ticks the interval hides. It may be higher by much more:
                // a contact of 1 or 2 ticks between two samples resets the full count and the sparse pass never sees it, so the offset lasts
                // until the next contact both see. That excess has no bound the Tick can give, so it is bounded by the largest one measured.
                const std::uint32_t lower{ a.AirborneTicks > b.AirborneTicks ? a.AirborneTicks - b.AirborneTicks : 0 };
                const std::uint32_t higher{ b.AirborneTicks > a.AirborneTicks ? b.AirborneTicks - a.AirborneTicks : 0 };
                this->Observe("BridgePlayer.AirborneTicks sparse below full", lower);
                this->Observe("BridgePlayer.AirborneTicks sparse above full", higher);
                if (lower > k_MaxTickShift) this->Violate("player AirborneTicks below", std::format("player {:#x} AirborneTicks full {} sparse {}", a.Handle, a.AirborneTicks, b.AirborneTicks));
                if (higher > k_MaxAirborneExcess) this->Violate("player AirborneTicks above", std::format("player {:#x} AirborneTicks full {} sparse {}", a.Handle, a.AirborneTicks, b.AirborneTicks));

                // The ground is the one of the sample. It may differ only where AirborneTicks does, because the hold of the last ground follows it.
                // GroundObject is the last object seen under the feet, which the sparse pass can miss when both stand on objects, and OwnVelocity
                // is relative to it.
                const bool airborneDiffers{ a.AirborneTicks != b.AirborneTicks };
                const bool bothOnObject{ a.Ground == BridgeGround::Object && b.Ground == BridgeGround::Object };
                const bool objectDiffers{ a.GroundObject != b.GroundObject };
                if (!airborneDiffers && a.Ground != b.Ground) this->Violate("player Ground", std::format("player {:#x} Ground differs with equal AirborneTicks {}", a.Handle, a.AirborneTicks));
                if (!airborneDiffers && a.IsGrounded != b.IsGrounded)
                {
                    this->Violate("player IsGrounded", std::format("player {:#x} IsGrounded differs with equal AirborneTicks {}", a.Handle, a.AirborneTicks));
                }

                if (!airborneDiffers && !bothOnObject && objectDiffers)
                {
                    this->Violate("player GroundObject", std::format("player {:#x} GroundObject differs with equal AirborneTicks {} and not on an object", a.Handle, a.AirborneTicks));
                }

                if (!airborneDiffers && !objectDiffers && std::memcmp(&a.OwnVelocity, &b.OwnVelocity, sizeof(a.OwnVelocity)) != 0)
                {
                    this->Violate("player OwnVelocity", std::format("player {:#x} OwnVelocity differs with equal AirborneTicks {} and GroundObject", a.Handle, a.AirborneTicks));
                }
            }

            const std::size_t lifts{ (std::min)({ static_cast<std::size_t>(full.Header.LiftCount), static_cast<std::size_t>(sparse.Header.LiftCount), full.Lifts.size() }) };
            for (std::size_t index{ 0 }; index < lifts; ++index)
            {
                const BridgeLift& a{ full.Lifts[index] };
                const BridgeLift& b{ sparse.Lifts[index] };

                const std::uint32_t shift{ Distance(a.ExitTick, b.ExitTick) };
                this->Observe("BridgeLift.ExitTick", shift);
                if (shift > k_MaxTickShift) this->Violate("lift ExitTick", std::format("lift {:#x} ExitTick full {} sparse {}", a.Handle, a.ExitTick, b.ExitTick));
                if (shift == 0 && std::memcmp(&a.ExitVelocity, &b.ExitVelocity, sizeof(a.ExitVelocity)) != 0)
                {
                    this->Violate("lift ExitVelocity", std::format("lift {:#x} ExitVelocity differs with equal ExitTick {}", a.Handle, a.ExitTick));
                }
            }

            this->CollectCharges(full, sparse);
        }

        // Learns the rate of charge of each weapon from the full pass (fraction gained per tick between two decisions while it charges)
        // and keeps the differences of the sparse pass.
        auto CollectCharges(const BridgeTick& full, const BridgeTick& sparse) -> void
        {
            const std::uint32_t tick{ full.Match.GameTick };
            const std::size_t weapons{ (std::min)({ static_cast<std::size_t>(full.Header.WeaponCount), static_cast<std::size_t>(sparse.Header.WeaponCount), full.Weapons.size() }) };
            for (std::size_t index{ 0 }; index < weapons; ++index)
            {
                const BridgeWeapon& a{ full.Weapons[index] };
                const BridgeWeapon& b{ sparse.Weapons[index] };

                const auto previous{ m_Charges.find(a.Handle) };
                if (previous != m_Charges.end() && std::isfinite(a.ChargeFraction) && a.ChargeFraction < 1.0f && a.ChargeFraction > previous->second.Fraction
                    && tick > previous->second.Tick)
                {
                    const double rate{ (a.ChargeFraction - previous->second.Fraction) / static_cast<double>(tick - previous->second.Tick) };
                    double& fastest{ m_Rates[a.Handle] };
                    fastest = (std::max)(fastest, rate);
                    m_FastestRate = (std::max)(m_FastestRate, rate);
                }

                if (std::isfinite(a.ChargeFraction)) m_Charges[a.Handle] = ChargeSample{ a.ChargeFraction, tick };
                else m_Charges.erase(a.Handle);

                if (std::memcmp(&a.ChargeFraction, &b.ChargeFraction, sizeof(float)) == 0) continue;

                if (!std::isfinite(a.ChargeFraction) || !std::isfinite(b.ChargeFraction))
                {
                    this->Violate("weapon charge not finite", std::format("weapon {:#x} ChargeFraction full {} sparse {}", a.Handle, a.ChargeFraction, b.ChargeFraction));
                    continue;
                }

                if (b.ChargeFraction > a.ChargeFraction) this->Violate("weapon charge sparse above full", std::format("weapon {:#x} ChargeFraction sparse {} above full {}", a.Handle, b.ChargeFraction, a.ChargeFraction));
                else m_ChargeCases.push_back(ChargeCase{ a.Handle, m_Record, a.ChargeFraction, b.ChargeFraction });
            }
        }

        // The sparse charge starts counting at the first sample that sees it, so it is lower than the full one by at most period - 1 ticks of the
        // rate of that weapon. The rate is the fastest the full pass showed for the handle, or for the film when the handle never showed one.
        auto CheckCharges() -> void
        {
            m_Report.ChargeDifferences = m_ChargeCases.size();
            for (const ChargeCase& charge : m_ChargeCases)
            {
                const auto own{ m_Rates.find(charge.Handle) };
                const bool hasOwnRate{ own != m_Rates.end() };
                const double rate{ hasOwnRate ? own->second : m_FastestRate };
                if (!hasOwnRate) ++m_Report.ChargeWithoutOwnRate;

                const double drop{ static_cast<double>(charge.Full) - charge.Sparse };
                if (rate <= 0.0)
                {
                    this->Observe("BridgeWeapon.ChargeFraction (fraction, no rate)", drop);
                    continue;
                }

                this->Observe("BridgeWeapon.ChargeFraction", drop / rate);
                if (drop > k_MaxTickShift * rate * k_ChargeSlack)
                {
                    m_Record = charge.Record;
                    this->Violate("weapon charge drop", std::format("weapon {:#x} ChargeFraction full {} sparse {}: {:.2f} ticks of charge at {:.5f} per tick", charge.Handle, charge.Full, charge.Sparse,
                        drop / rate, rate));
                }
            }
        }

        // return: The rows the header of the full tick says an array has, by the name of its count field.
        auto HeaderCount(const std::string& countField) const -> std::size_t
        {
            const LayoutStruct& header{ m_Structs.at("BridgeTickHeader") };
            const auto it{ std::ranges::find(header.Fields, countField, &LayoutField::Name) };
            const std::size_t headerOffset{ std::ranges::find(m_Structs.at("BridgeTick").Fields, "Header", &LayoutField::Name)->Offset };
            const std::size_t offset{ headerOffset + it->Offset };

            std::uint16_t full{};
            std::uint16_t sparse{};
            std::memcpy(&full, reinterpret_cast<const char*>(m_Full.get()) + offset, sizeof(full));
            std::memcpy(&sparse, reinterpret_cast<const char*>(m_Sparse.get()) + offset, sizeof(sparse));
            return std::min(full, sparse);
        }

        std::map<std::string, LayoutStruct> m_Structs{};
        std::unique_ptr<BridgeTick> m_Full{};
        std::unique_ptr<BridgeTick> m_Sparse{};
        std::size_t m_Record{};
        SparseReport m_Report{};
        std::map<std::uint32_t, ChargeSample> m_Charges{};
        std::map<std::uint32_t, double> m_Rates{};
        double m_FastestRate{};
        std::vector<ChargeCase> m_ChargeCases{};
    };

    // Replays the film with the full and the sparse pass side by side.
    // return: What the comparison found, or the reason the film could not be run in the outcome.
    auto CompareFilm(const std::string& film, FilmOutcome& outcome) -> SparseReport
    {
        TickComparer comparer{};
        outcome = RunFilmPass(film, false, {}, {}, 1, FilmSparse{ .Period = k_DecisionPeriod,
            .OnDecision = [&](Chain& full, Chain& sparse, std::size_t record) { comparer.Compare(full, sparse, record); } });
        return std::move(comparer.Report());
    }

    // return: The films of Ignore/Regresion that have golden.sdump, sorted by name.
    auto FilmsWithGolden() -> std::vector<std::string>
    {
        std::vector<std::string> films{};
        std::error_code error{};
        for (const auto& entry : std::filesystem::directory_iterator(RecordingsFolder(), error))
        {
            if (entry.is_directory() && std::filesystem::is_regular_file(entry.path() / "golden.sdump", error)) films.push_back(entry.path().filename().string());
        }

        std::ranges::sort(films);
        return films;
    }

    // Compares the films (in parallel) and checks every difference against the tolerance list.
    auto CheckFilms(const std::vector<std::string>& names) -> void
    {
        std::vector<std::future<std::pair<FilmOutcome, SparseReport>>> jobs{};
        for (const std::string& name : names)
        {
            jobs.push_back(std::async(std::launch::async, [name]
            {
                FilmOutcome outcome{};
                SparseReport report{ CompareFilm(name, outcome) };
                return std::pair<FilmOutcome, SparseReport>{ std::move(outcome), std::move(report) };
            }));
        }

        std::size_t replayed{ 0 };
        for (std::size_t index{ 0 }; index < jobs.size(); ++index)
        {
            const std::string& name{ names[index] };
            const auto [outcome, report] = jobs[index].get();
            SCOPED_TRACE(name);

            if (!outcome.Error.empty()) { ADD_FAILURE() << outcome.Error; continue; }
            if (!outcome.Skip.empty()) continue;

            ++replayed;
            EXPECT_GT(report.Decisions, 0u);
            std::cout << std::format("[  SPARSE  ] {}: {} records, {} decisions, {} fields differ, {:.1f} s\n", name, outcome.Records, report.Decisions,
                report.Differences.size(), outcome.Seconds) << std::flush;

            for (const auto& [field, magnitude] : report.MaxMagnitude)
            {
                std::cout << std::format("[  SPARSE  ]   max magnitude {}: {:.3f}{}\n", field, magnitude, field == "BridgeWeapon.ChargeFraction" ? " ticks of charge" : "");
            }

            std::cout << std::format("[  SPARSE  ]   charge differences {} ({} bounded with the film rate)\n", report.ChargeDifferences, report.ChargeWithoutOwnRate);
            for (const auto& [kind, count] : report.ViolationKinds) std::cout << std::format("[  SPARSE  ]   violations {}: {}\n", kind, count);
            for (const std::string& line : report.Violations) ADD_FAILURE() << "magnitude: " << line;
            if (report.ViolationCount > report.Violations.size()) ADD_FAILURE() << report.ViolationCount << " magnitude violations in all.";

            for (const auto& [field, difference] : report.Differences)
            {
                const auto tolerance{ std::ranges::find(k_Tolerances, field, &Tolerance::Field) };
                std::cout << std::format("[  SPARSE  ]   {}: {} values{}\n", field, difference.Count, tolerance == k_Tolerances.end() ? " (NOT in the list)" : "");
                for (const std::string& line : difference.First) std::cout << "[  SPARSE  ]     " << line << "\n";

                if (tolerance == k_Tolerances.end())
                {
                    ADD_FAILURE() << field << " differs in " << difference.Count << " values and is not in the tolerance list. First: "
                        << (difference.First.empty() ? std::string{} : difference.First.front());
                }
                else
                {
                    EXPECT_LE(difference.Count, tolerance->Cap) << field << " differs more than its cap.";
                }
            }
        }

        std::cout << std::flush;
        if (replayed == 0) GTEST_SKIP() << "No recording (golden.sdump) in " << RecordingsFolder().string();
    }
}

// The short film that stays in the suite (about 30 s). The flags and the lifts are in the test of every film.
TEST(ReplaySparseTests, ShortFilmGivesTheSameTickAsTheFullPassOutsideTheToleranceList)
{
    CheckFilms({ "explosivos" });
}

// Every film of the recordings. Slow: run it with --gtest_also_run_disabled_tests when Q101 changes.
TEST(ReplaySparseTests, DISABLED_EveryFilmGivesTheSameTickAsTheFullPassOutsideTheToleranceList)
{
    CheckFilms(FilmsWithGolden());
}