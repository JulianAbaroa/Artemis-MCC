export module Map.Reader.System:TagResolver;

import :Formula;

import Service.Logs.System;
import Map.Reader.Type;
import Map.Reader.State;
import std;

export namespace Map::Reader::System
{
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
			TagIndexStore& m_TagIndexStore, FormulaService& m_FormulaService) :
			m_LogsService(logsService), m_FileStore(fileStore),
			m_TagIndexStore(m_TagIndexStore), m_FormulaService(m_FormulaService) {}
		~TagResolverService() = default;

		auto ResolveHandle(std::uint32_t handle) const -> TagInfo;
		auto GetTagOffset(std::int32_t tagIndex) const -> std::int64_t;
		auto ResolveTagOffset(const TagTableEntry& tag) const -> std::int64_t;
		auto ResolveTagReferenceName(const TagReference& reference) const -> std::string;
		auto ResolveBlockOffset(const TagBlock& block) const -> std::int64_t;

		auto HasBipd(const std::string& tagName) const -> bool;
		auto HasBloc(const std::string& tagName) const -> bool;
		auto HasColl(const std::string& tagName) const -> bool;
		auto HasCtrl(const std::string& tagName) const -> bool;
		auto HasEqip(const std::string& tagName) const -> bool;
		auto HasHlmt(const std::string& tagName) const -> bool;
		auto HasMach(const std::string& tagName) const -> bool;
		auto HasMode(const std::string& tagName) const -> bool;
		auto HasPhmo(const std::string& tagName) const -> bool;
		auto HasProj(const std::string& tagName) const -> bool;
		auto HasScen(const std::string& tagName) const -> bool;
		auto HasScnr(const std::string& tagName) const -> bool;
		auto HasVehi(const std::string& tagName) const -> bool;
		auto HasWeap(const std::string& tagName) const -> bool;

	private:
		LogsService& m_LogsService;
		FileStore& m_FileStore;
		TagIndexStore& m_TagIndexStore;
		FormulaService& m_FormulaService;

		auto MagicToString(std::int32_t magic) const -> std::string;

		auto EnsureGroupIndexBuilt() const -> void;
		auto HasGroup(const std::string& tagName, std::uint32_t magic) const -> bool;

		mutable std::unordered_map<std::string, std::uint32_t> m_GroupMaskByName;
		mutable bool m_GroupIndexBuilt{ false };
		mutable std::mutex m_GroupIndexMutex;
	};
}