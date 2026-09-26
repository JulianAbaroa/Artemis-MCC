module Map.Reader.System;
import :Geometry.Loader;

namespace
{
	using MeshesObject = Map::Tag::Type::Lbsp::Object::Lbsp_MeshesObject;
	using TagResourcesObject = Map::Tag::Type::Zone::Object::Zone_TagResourcesObject;
	using BufferInfo = Map::Reader::Type::Geometry::BufferInfo;
}

namespace Map::Reader::System
{
	auto GeometryLoaderService::ReadRenderGeometry(
		std::vector<std::string>& sbspTagNames) const -> std::vector<SbspGeometry>
	{
		std::vector<SbspGeometry> result;

		const ZoneObject* zone = m_TagCatalog.Zone.Get("__zone__");
		const PlayObject* play = m_TagCatalog.Play.Get("__play__");
		if (!zone || !play) return result;

		const std::int64_t fixupDataBase = m_FormulaService.ToFileOffset(
			m_FormulaService.Expand(zone->Data.FixupInformation.Pointer));

		std::int32_t processed = 0;

		result.reserve(sbspTagNames.size());

		for (const std::string& tagName : sbspTagNames)
		{
			const SbspObject* sbsp = m_TagCatalog.Sbsp.Get(tagName);
			if (!sbsp) continue;

			const LbspObject* lbsp = m_TagCatalog.Lbsp.Get(tagName);
			if (!lbsp || lbsp->Meshes.empty()) continue;

			const TagResourcesObject* lbspEntry = nullptr;

			std::vector<std::uint8_t> pageData = m_GeometryResourcePageService.Read(
				lbsp->Data.ZoneAssetDatum, &lbspEntry, zone, play);

			if (pageData.empty() || !lbspEntry)
			{
				m_LogsService.Message("[GeometryLoaderService] INFO: '{}':"
					" LBSP page empty/omitted", tagName);
				continue;
			}

			std::int32_t vbCount = 0, ibCount = 0;
			if (!m_GeometryInfoService.ReadFooter(*lbspEntry, vbCount, ibCount,
				fixupDataBase) || vbCount <= 0)
			{
				m_LogsService.Message("[GeometryLoaderService] WARNING:"
					" '{}': invalid footer", tagName);
				continue;
			}

			std::vector<BufferInfo> vbInfo, ibInfo;
			if (!m_GeometryInfoService.ReadBufferInfoTables(*lbspEntry, vbCount,
				ibCount, vbInfo, ibInfo, fixupDataBase))
			{
				m_LogsService.Message("[GeometryLoaderService] WARNING:"
					" '{}': invalid info-arrays", tagName);
				continue;
			}

			SbspGeometry geometry{};
			geometry.TagName = tagName;

			const char* name = tagName.c_str();

			std::uint32_t trianglesWorld = 0;

			for (const auto& cluster : sbsp->Clusters)
			{
				int sectionIdx = cluster.MeshIndex;
				if (sectionIdx < 0 || sectionIdx >= (int)lbsp->Meshes.size())
				{
					continue;
				}

				const MeshesObject& section = lbsp->Meshes[sectionIdx];

				trianglesWorld += m_GeometryMeshDecoderService.EmitSection(
					pageData, *lbspEntry, vbCount, vbInfo, ibInfo, section, 
					nullptr, name, nullptr, geometry.RenderGeometry);
			}

			std::uint32_t trianglesInstances = m_GeometryMeshDecoderService.
				EmitInstancedGeometry(pageData, *lbspEntry, vbCount, vbInfo, ibInfo, 
					sbsp, lbsp, zone, fixupDataBase, name, geometry.RenderGeometry);

			m_LogsService.Message("[GeometryLoaderService] INFO:"
				" '{}': {} total tris ({} clusters + {} instances)",
				tagName, trianglesWorld + trianglesInstances,
				trianglesWorld, trianglesInstances);

			result.push_back(std::move(geometry));

			++processed;
		}

		m_LogsService.Message("[GeometryLoaderService] INFO:"
			" processed SBSPs: {}", processed);

		return result;
	}
}