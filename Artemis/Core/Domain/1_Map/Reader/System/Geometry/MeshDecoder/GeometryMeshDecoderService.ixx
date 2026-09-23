export module Map.Reader.System:Geometry.MeshDecoder;

import :DataStream;
import :Formula;
import :Geometry.ResourcePage;
import :Geometry.Info;
import Service.Logs.System;
import Common.Math.Type;
import Map.Reader.Type;
import Map.Reader.State;
import Map.Tag.Type;
import std;

export namespace Map::Reader::System
{
	class GeometryMeshDecoderService
	{
	private:
		using Vec3 = Common::Math::Type::Vec3;
		using Triangle = Common::Math::Type::Triangle;
		using SbspObject = Map::Tag::Type::Sbsp::Object::SbspObject;
		using LbspObject = Map::Tag::Type::Lbsp::Object::LbspObject;
		using ZoneObject = Map::Tag::Type::Zone::Object::ZoneObject;
		using MeshesObject = Map::Tag::Type::Lbsp::Object::Lbsp_MeshesObject;
		using CompressionInfoEntry3 = Map::Tag::Type::Sbsp::Structure::Sbsp_CompressionInfoEntry_3;
		using TagResourcesObject = Map::Tag::Type::Zone::Object::Zone_TagResourcesObject;
		using VertexDecodeContext = Map::Reader::Type::Geometry::VertexDecodeContext;
		using BufferInfo = Map::Reader::Type::Geometry::BufferInfo;

		using LogsService = Service::Logs::System::LogsService;
		using FileStore = Map::Reader::State::FileStore;
		using DataStreamService = Map::Reader::System::DataStreamService;
		using FormulaService = Map::Reader::System::FormulaService;

		static constexpr std::uint32_t m_kFixupMask{ 0x0FFFFFFF };
		static constexpr std::uint32_t m_kResourceDatumMask{ 0xFFFFu };

		static constexpr std::uint32_t m_kVertexBufferStride{ 0x24 };
		static constexpr std::int32_t m_kXOffset{ 0 };
		static constexpr std::int32_t m_kYOffset{ 4 };
		static constexpr std::int32_t m_kZOffset{ 8 };
		static constexpr std::uint16_t m_kMeshFlagUnindexed{ (1u << 4) };

		static constexpr std::int32_t m_kInstanceStride{ 156 };
		static constexpr int m_kInstanceFixupFromEnd{ 10 };
		static constexpr int m_kInstanceMatrixFloats{ 12 };
		static constexpr int m_kInstanceScaledFloats{ 9 };
		static constexpr int m_kInstanceScaleOffset{ 0 };
		static constexpr int m_kInstanceMatrixOffset{ 4 };
		static constexpr int m_kInstanceSectionOffset{ 58 };

	public:
		GeometryMeshDecoderService(LogsService& logsService, FileStore& fileStore,
			DataStreamService& dataStreamService, FormulaService& m_FormulaService) :
			m_LogsService(logsService), m_FileStore(fileStore), 
			m_DataStreamService(dataStreamService), m_FormulaService(m_FormulaService) {}
		~GeometryMeshDecoderService() = default;

		auto EmitSection(const std::vector<std::uint8_t>& pageData,
			const TagResourcesObject& entry, std::int32_t vbCount,
			const std::vector<BufferInfo>& vbInfo, 
			const std::vector<BufferInfo>& ibInfo,
			const MeshesObject& section,
			const CompressionInfoEntry3* compressionInfo,
			const char* tagName, const float* transform,
			std::vector<Triangle>& out) const -> std::uint32_t;

		auto EmitInstancedGeometry(const std::vector<std::uint8_t>& pageData,
			const TagResourcesObject& lbspEntry, std::int32_t vbCount,
			const std::vector<BufferInfo>& vbInfo, 
			const std::vector<BufferInfo>& ibInfo,
			const SbspObject* sbsp, const LbspObject* lbsp,
			const ZoneObject* zone, std::int64_t fixupDataBase,
			const char* tagName, std::vector<Triangle>& out) const -> std::uint32_t;

		auto GetCompressionInfo(int sectionIdx, 
			const SbspObject* sbsp) const -> const CompressionInfoEntry3*;

	private:
		LogsService& m_LogsService;
		FileStore& m_FileStore;
		DataStreamService& m_DataStreamService;
		FormulaService& m_FormulaService;

		auto EmitRange(std::uint32_t start, std::uint32_t count,
			const VertexDecodeContext& context,
			const std::uint8_t* ibPointer, bool wide,
			std::uint32_t idxCount, bool isStrip,
			std::uint32_t& emitted, std::vector<Triangle>& out) const -> void;

		auto PushTriangle(std::uint32_t a, std::uint32_t b, std::uint32_t c,
			const VertexDecodeContext& context,
			std::uint32_t& emitted, std::vector<Triangle>& out) const -> void;

		auto ReadVertex(std::uint32_t idx,
			const VertexDecodeContext& context) const -> Vec3;

		auto ReadIndex(std::uint32_t i, bool wide,
			const std::uint8_t* ibPointer) const -> std::uint32_t;
	};
}