export module Blam.Layout.Game.Type:VariantPreset;

import :Offset;

import std;

// Pure logic of the gametype presets (D264, D265): parse the compiled files, compose the final value of every field of the catalog from the
// saved base and a preset, apply it to a game variant with masks and decide whether a request is rejected. It reads and writes the bytes of a
// variant it is given and never touches the game.
export namespace Blam::Layout::Game::Type::VariantPreset
{
    // When the engine reads a field of the variant. Load fields can only change when the map loads.
    enum class PresetWhen : std::uint8_t
    {
        Tick,
        Spawn,
        Pickup,
        RoundStart,
        RoundEnd,
        Load,
    };

    // Moment a preset is planned for. At load every field may change, between rounds the fields of Load stay as they are.
    enum class PresetPhase : std::uint8_t
    {
        Load,
        BetweenRounds,
    };

    // A writable field of the variant. Mask holds Size bytes: the bits of the field.
    struct PresetField
    {
        std::size_t Offset{};
        std::size_t Size{};
        std::vector<std::uint8_t> Mask{};
        PresetWhen When{};
    };

    // Every writable field, in the order of the file (the order of the saved base).
    struct PresetCatalog
    {
        std::vector<PresetField> Fields{};
    };

    // Bytes to write at an offset of the variant. Only the bits of the mask are written. Bytes and Mask have the same length.
    struct PresetWrite
    {
        std::size_t Offset{};
        PresetWhen When{};
        std::vector<std::uint8_t> Bytes{};
        std::vector<std::uint8_t> Mask{};
    };

    struct PresetWrites : std::vector<PresetWrite>
    {
        using vector::vector;
    };

    // A preset as the Python compiler wrote it, with its inheritance already resolved.
    struct CompiledPreset
    {
        std::string Name{};
        std::string Source{};
        PresetWrites Writes{};
        // Indices of the trait sets of the script that the preset writes. They have to exist in the variant.
        std::vector<std::uint32_t> TraitSetChecks{};
    };

    // The bytes of every field of the catalog (Size bytes each, in catalog order) as the variant had them when it was loaded.
    struct PresetBase
    {
        std::vector<std::uint8_t> Bytes{};
    };

    // Why a request is not applied. Nothing is written.
    struct PresetRejection
    {
        std::string Reason{};
    };

    // Longest field the files may declare, in bytes.
    constexpr std::size_t k_MaxFieldSize{ 64 };

    // Names of PresetWhen in the compiled files, in enum order.
    constexpr std::array<std::string_view, 6> k_WhenNames{ "tick", "spawn", "pickup", "round_start", "round_end", "load" };
}

namespace
{
    using Blam::Layout::Game::Type::VariantPreset::PresetWhen;
    using Blam::Layout::Game::Type::VariantPreset::PresetWrite;
    using Blam::Layout::Game::Type::VariantPreset::k_MaxFieldSize;
    using Blam::Layout::Game::Type::VariantPreset::k_WhenNames;
    using Blam::Layout::Game::Type::Offset::k_VariantEngineType;
    using Blam::Layout::Game::Type::Offset::k_VariantEngineTypeMegalo;
    using Blam::Layout::Game::Type::Offset::k_VariantMegaloTraitSetCount;
    using Blam::Layout::Game::Type::Offset::k_VariantScoreToWin;

    constexpr int k_HexBase{ 16 };
    constexpr std::size_t k_NibblesPerByte{ 2 };

    struct SourceLine
    {
        std::size_t Number{};
        std::vector<std::string_view> Words{};
    };

    // return: The lines of the text that are not empty, split into words. Accepts LF and CRLF.
    auto SplitLines(std::string_view text) -> std::vector<SourceLine>
    {
        std::vector<SourceLine> lines{};
        std::size_t number{};

        for (const auto part : std::views::split(text, '\n'))
        {
            ++number;
            SourceLine line{ .Number = number };
            for (const auto word : std::views::split(std::string_view{ part.begin(), part.end() }, ' '))
            {
                std::string_view view{ word.begin(), word.end() };
                while (!view.empty() && (view.back() == '\r' || view.back() == '\t')) view.remove_suffix(1);
                if (!view.empty()) line.Words.push_back(view);
            }
            if (!line.Words.empty()) lines.push_back(std::move(line));
        }

        return lines;
    }

    auto ParseNumber(std::string_view text, std::size_t& value, int base) -> bool
    {
        const auto result{ std::from_chars(text.data(), text.data() + text.size(), value, base) };
        return result.ec == std::errc{} && result.ptr == text.data() + text.size();
    }

    // Parses a string of hex digit pairs (memory order) into bytes.
    auto ParseBytes(std::string_view text, std::vector<std::uint8_t>& bytes) -> bool
    {
        if (text.empty() || text.size() % k_NibblesPerByte != 0 || text.size() / k_NibblesPerByte > k_MaxFieldSize) return false;

        bytes.clear();
        for (std::size_t i{}; i < text.size(); i += k_NibblesPerByte)
        {
            std::uint8_t value{};
            const auto result{ std::from_chars(text.data() + i, text.data() + i + k_NibblesPerByte, value, k_HexBase) };
            if (result.ec != std::errc{} || result.ptr != text.data() + i + k_NibblesPerByte) return false;
            bytes.push_back(value);
        }

        return true;
    }

    auto ParseWhen(std::string_view text, PresetWhen& when) -> bool
    {
        const auto found{ std::ranges::find(k_WhenNames, text) };
        if (found == k_WhenNames.end()) return false;

        when = static_cast<PresetWhen>(found - k_WhenNames.begin());
        return true;
    }

    auto LineError(std::size_t number, std::string_view expected) -> std::string
    {
        return std::format("Line {}: expected {}.", number, expected);
    }

    auto ReadU32(std::span<const std::uint8_t> bytes, std::size_t offset) -> std::uint32_t
    {
        std::uint32_t value{};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }

    // return: Whether the write covers bytes inside the variant.
    auto IsInside(const PresetWrite& write, std::size_t size) -> bool
    {
        return write.Bytes.size() == write.Mask.size() && write.Offset <= size && write.Bytes.size() <= size - write.Offset;
    }

    // return: Whether the bits of the mask in the variant already hold the bytes of the write.
    auto IsApplied(const PresetWrite& write, std::span<const std::uint8_t> variant) -> bool
    {
        for (std::size_t i{}; i < write.Bytes.size(); ++i)
        {
            if (((variant[write.Offset + i] ^ write.Bytes[i]) & write.Mask[i]) != 0) return false;
        }
        return true;
    }
}

export namespace Blam::Layout::Game::Type::VariantPreset
{
    // Parses _catalog.txt: one line "field <offset hex> <size> <mask hex> <when>" per field.
    // return: The catalog, or the first line that is wrong.
    auto ParseCatalog(std::string_view text) -> std::expected<PresetCatalog, std::string>
    {
        PresetCatalog catalog{};

        for (const SourceLine& line : SplitLines(text))
        {
            PresetField field{};
            std::size_t offset{};
            std::size_t size{};

            if (line.Words.size() != 5 || line.Words[0] != "field" || !ParseNumber(line.Words[1], offset, k_HexBase) ||
                !ParseNumber(line.Words[2], size, 10) || size == 0 || size > k_MaxFieldSize || !ParseBytes(line.Words[3], field.Mask) ||
                field.Mask.size() != size || !ParseWhen(line.Words[4], field.When))
            {
                return std::unexpected(LineError(line.Number, "'field <offset hex> <size> <mask hex> <when>'"));
            }

            field.Offset = offset;
            field.Size = size;
            catalog.Fields.push_back(std::move(field));
        }

        return catalog;
    }

    // Parses <name>.txt of a compiled preset. A line that is wrong invalidates the whole preset.
    // return: The preset, or the first line that is wrong.
    auto ParsePreset(std::string_view text) -> std::expected<CompiledPreset, std::string>
    {
        CompiledPreset preset{};
        bool isNamed{};

        for (const SourceLine& line : SplitLines(text))
        {
            const std::string_view keyword{ line.Words[0] };

            if (keyword == "preset" && line.Words.size() == 2 && !isNamed)
            {
                preset.Name = std::string{ line.Words[1] };
                isNamed = true;
            }
            else if (keyword == "source" && line.Words.size() == 2) preset.Source = std::string{ line.Words[1] };
            else if (keyword == "write")
            {
                PresetWrite write{};
                std::size_t offset{};

                if (line.Words.size() != 5 || !ParseNumber(line.Words[1], offset, k_HexBase) || !ParseWhen(line.Words[2], write.When) ||
                    !ParseBytes(line.Words[3], write.Bytes) || !ParseBytes(line.Words[4], write.Mask) || write.Bytes.size() != write.Mask.size())
                {
                    return std::unexpected(LineError(line.Number, "'write <offset hex> <when> <bytes hex> <mask hex>' with bytes and mask of the same length"));
                }

                write.Offset = offset;
                preset.Writes.push_back(std::move(write));
            }
            else if (keyword == "check")
            {
                std::size_t index{};
                if (line.Words.size() != 3 || line.Words[1] != "megalo_trait_set" || !ParseNumber(line.Words[2], index, 10))
                {
                    return std::unexpected(LineError(line.Number, "'check megalo_trait_set <index>'"));
                }
                preset.TraitSetChecks.push_back(static_cast<std::uint32_t>(index));
            }
            else return std::unexpected(LineError(line.Number, "'preset <name>' once, 'source <hash>', 'write ...' or 'check ...'"));
        }

        if (!isNamed) return std::unexpected(std::string{ "Line 1: expected 'preset <name>'." });
        return preset;
    }

    // return: Whether the bytes are a Megalo variant big enough to hold every field of the catalog.
    auto IsMegaloVariant(std::span<const std::uint8_t> variant) -> bool
    {
        return variant.size() >= k_VariantScoreToWin + sizeof(std::uint16_t) &&
            ReadU32(variant, k_VariantEngineType) == k_VariantEngineTypeMegalo;
    }

    // Saves the bytes of every field of the catalog from a variant, to restore them later.
    // return: The base, or why a field is outside the variant.
    auto SaveBase(const PresetCatalog& catalog, std::span<const std::uint8_t> variant) -> std::expected<PresetBase, std::string>
    {
        PresetBase base{};

        for (const PresetField& field : catalog.Fields)
        {
            if (field.Offset > variant.size() || field.Size > variant.size() - field.Offset)
            {
                return std::unexpected(std::format("The field at offset 0x{:X} is outside the variant.", field.Offset));
            }
            base.Bytes.insert(base.Bytes.end(), variant.begin() + field.Offset, variant.begin() + field.Offset + field.Size);
        }

        return base;
    }

    // Composes the final value of every field of the catalog: the one of the preset when it writes the field, the saved base otherwise.
    // param preset: The preset to put, or null for the base alone (a request with no preset restores everything).
    // return: One write per field with the mask of the field, or why the preset does not fit the catalog.
    auto Compose(const PresetCatalog& catalog, const PresetBase& base, const CompiledPreset* preset) -> std::expected<PresetWrites, std::string>
    {
        std::map<std::pair<std::size_t, std::vector<std::uint8_t>>, const PresetWrite*> chosen{};

        if (preset != nullptr)
        {
            std::set<std::pair<std::size_t, std::vector<std::uint8_t>>> known{};
            for (const PresetField& field : catalog.Fields) known.emplace(field.Offset, field.Mask);

            for (const PresetWrite& write : preset->Writes)
            {
                const std::pair key{ write.Offset, write.Mask };
                if (!known.contains(key)) return std::unexpected(std::format("The write at offset 0x{:X} is not a field of the catalog.", write.Offset));
                chosen[key] = &write;
            }
        }

        PresetWrites writes{};
        std::size_t cursor{};

        for (const PresetField& field : catalog.Fields)
        {
            if (cursor + field.Size > base.Bytes.size()) return std::unexpected(std::string{ "The saved base does not match the catalog." });

            PresetWrite write{ .Offset = field.Offset, .When = field.When, .Bytes = {}, .Mask = field.Mask };
            const auto found{ chosen.find({ field.Offset, field.Mask }) };
            const bool isWritten{ found != chosen.end() };

            if (isWritten && found->second->Bytes.size() != field.Size)
            {
                return std::unexpected(std::format("The write at offset 0x{:X} has another size than its field.", field.Offset));
            }

            for (std::size_t i{}; i < field.Size; ++i)
            {
                const std::uint8_t value{ isWritten ? found->second->Bytes[i] : base.Bytes[cursor + i] };
                write.Bytes.push_back(static_cast<std::uint8_t>(value & field.Mask[i]));
            }

            cursor += field.Size;
            writes.push_back(std::move(write));
        }

        return writes;
    }

    // Decides a request: composes the preset (or the base) over the catalog and keeps the writes that change the variant.
    // A request is rejected, with nothing to write, when the variant is not Megalo, a trait set of the preset does not exist, a write does not
    // fit, or (between rounds) a field of Load would end with a value other than the one the variant has now.
    // param preset: The preset to put, or null to restore the base.
    // return: The writes to apply, or the reason of the rejection.
    auto Plan(const PresetCatalog& catalog, const PresetBase& base, const CompiledPreset* preset, std::span<const std::uint8_t> variant,
        PresetPhase phase) -> std::expected<PresetWrites, PresetRejection>
    {
        if (!IsMegaloVariant(variant)) return std::unexpected(PresetRejection{ "The game variant is not a Megalo variant." });

        if (preset != nullptr)
        {
            const std::uint32_t setCount{ ReadU32(variant, k_VariantMegaloTraitSetCount) };
            for (const std::uint32_t index : preset->TraitSetChecks)
            {
                if (index >= setCount)
                {
                    return std::unexpected(PresetRejection{ std::format("Trait set {} does not exist (the script has {}).", index, setCount) });
                }
            }
        }

        std::expected<PresetWrites, std::string> composed{ Compose(catalog, base, preset) };
        if (!composed) return std::unexpected(PresetRejection{ composed.error() });

        PresetWrites changes{};
        for (PresetWrite& write : *composed)
        {
            if (!IsInside(write, variant.size())) return std::unexpected(PresetRejection{ std::format("The field at offset 0x{:X} is outside the variant.", write.Offset) });
            if (IsApplied(write, variant)) continue;

            if (phase == PresetPhase::BetweenRounds && write.When == PresetWhen::Load)
            {
                return std::unexpected(PresetRejection{ std::format("The field at offset 0x{:X} can only change at load.", write.Offset) });
            }
            changes.push_back(std::move(write));
        }

        return changes;
    }

    // Writes the bits of the mask of every write into the variant. Nothing is written when a write is outside it.
    // return: Whether the writes were applied.
    auto Apply(std::span<std::uint8_t> variant, const PresetWrites& writes) -> bool
    {
        if (!std::ranges::all_of(writes, [&](const PresetWrite& write) { return IsInside(write, variant.size()); })) return false;

        for (const PresetWrite& write : writes)
        {
            for (std::size_t i{}; i < write.Bytes.size(); ++i)
            {
                std::uint8_t& target{ variant[write.Offset + i] };
                target = static_cast<std::uint8_t>((target & ~write.Mask[i]) | (write.Bytes[i] & write.Mask[i]));
            }
        }

        return true;
    }

    // return: Whether every write is in the variant (read back after Apply).
    auto Verify(std::span<const std::uint8_t> variant, const PresetWrites& writes) -> bool
    {
        return std::ranges::all_of(writes, [&](const PresetWrite& write) { return IsInside(write, variant.size()) && IsApplied(write, variant); });
    }
}