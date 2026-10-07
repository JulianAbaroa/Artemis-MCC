export module Map.Reader.System:TagResolver;

import :Formula;

import Service.Logs.System;
import Map.Reader.Type;
import Map.Reader.State;
import std;

export namespace Map::Reader::System
{
    // Turns the values stored in the map into tag information: names, offsets and group membership.
    class TagResolverService
    {
    private:
        using TagInfo = Map::Reader::Type::Info::TagInfo;
        using TagTableEntry = Map::Reader::Type::Structure::TagTable::Entry;
        using TagReference = Map::Reader::Type::Structure::Tag::Reference;
        using TagBlock = Map::Reader::Type::Structure::Tag::Block;

        using LogsService = Service::Logs::System::LogsService;
        using FileStore = Map::Reader::State::FileStore;
        using TagIndexStore = Map::Reader::State::TagIndexStore;
        using FormulaService = Map::Reader::System::FormulaService;

    public:
        TagResolverService(LogsService& logsService, FileStore& fileStore,
            TagIndexStore& tagIndexStore, FormulaService& formulaService) :
            m_LogsService(logsService), m_FileStore(fileStore),
            m_TagIndexStore(tagIndexStore), m_FormulaService(formulaService) {}
        ~TagResolverService() = default;

        // Gets the group and the name of the tag that a datum handle points to.
        // param handle: Datum handle. The upper 16 bits are the salt and the lower 16 bits the tag index.
        // return: Info with IsValid false if no map is loaded, the handle is invalid or its salt does not match.
        auto ResolveHandle(std::uint32_t handle) const -> TagInfo;

        // return: File offset of the tag data, or -1 if the index is out of range or the tag has no data.
        auto GetTagOffset(std::int32_t tagIndex) const -> std::int64_t;

        // return: File offset of the tag data, or -1 if the tag has no data.
        auto ResolveTagOffset(const TagTableEntry& tag) const -> std::int64_t;

        // return: Name of the tag the reference points to, or empty if the reference is null or points outside the tag table.
        auto ResolveTagReferenceName(const TagReference& reference) const -> std::string;

        // return: File offset of the first entry of the block, or -1 if the block is empty.
        auto ResolveBlockOffset(const TagBlock& block) const -> std::int64_t;

        // Checks if a tag of that group exists with that name.
        // note: Only the groups of objects and of the tags derived from them are tracked.
        // The groups of each name are indexed on the first call.
        auto HasBipd(const std::string& tagName) const -> bool;
        auto HasBloc(const std::string& tagName) const -> bool;
        auto HasColl(const std::string& tagName) const -> bool;
        auto HasCtrl(const std::string& tagName) const -> bool;
        auto HasEqip(const std::string& tagName) const -> bool;
        auto HasHlmt(const std::string& tagName) const -> bool;
        auto HasMach(const std::string& tagName) const -> bool;
        auto HasMode(const std::string& tagName) const -> bool;
        auto HasProj(const std::string& tagName) const -> bool;
        auto HasScen(const std::string& tagName) const -> bool;
        auto HasVehi(const std::string& tagName) const -> bool;
        auto HasWeap(const std::string& tagName) const -> bool;

        // Forgets the groups indexed by name, so the next map is indexed again.
        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        FileStore& m_FileStore;
        TagIndexStore& m_TagIndexStore;
        FormulaService& m_FormulaService;

        mutable std::unordered_map<std::string, std::uint32_t> m_GroupMaskByName{};
        mutable bool m_IsGroupIndexBuilt{ false };
        mutable std::mutex m_GroupIndexMutex{};

        auto MagicToString(std::int32_t magic) const -> std::string;

        auto EnsureGroupIndexBuilt() const -> void;
        auto HasGroup(const std::string& tagName, std::uint32_t magic) const -> bool;
    };
}