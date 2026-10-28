export module Resolved.Tag.System;

import Common.Log.System;
import Map.Reader.State;
import Resolved.Tag.Type;
import std;

export namespace Resolved::Tag::System
{
    // Turns the handles and names of the tags of the loaded map into tag information: names, groups and group membership.
    class TagResolverService
    {
    private:
        using TagInfo = Resolved::Tag::Type::TagInfo;

        using Log = Common::Log::System::Log;
        using FileStore = Map::Reader::State::FileStore;
        using TagIndexStore = Map::Reader::State::TagIndexStore;

    public:
        TagResolverService(Log& log, FileStore& fileStore, TagIndexStore& tagIndexStore) :
            m_Log(log), m_FileStore(fileStore), m_TagIndexStore(tagIndexStore) {}
        ~TagResolverService() = default;

        // Gets the group and the name of the tag that a datum handle points to.
        // param handle: Datum handle. The upper 16 bits are the salt and the lower 16 bits the tag index.
        // return: Info with IsValid false if no map is loaded, the handle is invalid or its salt does not match.
        auto ResolveHandle(std::uint32_t handle) const -> TagInfo;

        // Checks if a tag of that group exists with that name.
        // note: Only the groups of the collision, model and render model tags are tracked.
        // The groups of each name are indexed on the first call.
        auto HasColl(const std::string& tagName) const -> bool;
        auto HasHlmt(const std::string& tagName) const -> bool;
        auto HasMode(const std::string& tagName) const -> bool;

        // Forgets the groups indexed by name, so the next map is indexed again.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        FileStore& m_FileStore;
        TagIndexStore& m_TagIndexStore;

        mutable std::unordered_map<std::string, std::uint32_t> m_GroupMaskByName{};
        mutable bool m_IsGroupIndexBuilt{ false };
        mutable std::mutex m_GroupIndexMutex{};

        auto MagicToString(std::int32_t magic) const -> std::string;

        auto EnsureGroupIndexBuilt() const -> void;
        auto HasGroup(const std::string& tagName, std::uint32_t magic) const -> bool;
    };
}