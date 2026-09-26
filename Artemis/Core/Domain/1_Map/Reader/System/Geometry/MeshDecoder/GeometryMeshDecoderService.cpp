module Map.Reader.System;
import :Geometry.MeshDecoder;

namespace
{
	using Map::Reader::Type::Constant::k_FixupMask;
	using Map::Reader::Type::Constant::k_ResourceDatumMask;
	using Map::Reader::Type::Constant::k_VertexBufferStride;
	using Map::Reader::Type::Constant::k_XOffset;
	using Map::Reader::Type::Constant::k_YOffset;
	using Map::Reader::Type::Constant::k_ZOffset;
	using Map::Reader::Type::Constant::k_MeshFlagUnindexed;
	using Map::Reader::Type::Constant::k_InstanceStride;
	using Map::Reader::Type::Constant::k_InstanceFixupFromEnd;
	using Map::Reader::Type::Constant::k_InstanceMatrixFloats;
	using Map::Reader::Type::Constant::k_InstanceScaledFloats;
	using Map::Reader::Type::Constant::k_InstanceScaleOffset;
	using Map::Reader::Type::Constant::k_InstanceMatrixOffset;
	using Map::Reader::Type::Constant::k_InstanceSectionOffset;
}

namespace Map::Reader::System
{
	auto GeometryMeshDecoderService::EmitSection(const std::vector<std::uint8_t>& pageData,
		const TagResourcesObject& entry, std::int32_t vbCount, 
		const std::vector<BufferInfo>& vbInfo,
		const std::vector<BufferInfo>& ibInfo, const MeshesObject& sec,
		const CompressionInfoEntry3* compressionInfo,
		const char* tagName, const float* transformMatrix,
		std::vector<Triangle>& out) const -> std::uint32_t
	{
		int vbIdx = sec.VertexBufferIndex1;
		int ibIdx = sec.IndexBufferIndex;

		if (vbIdx < 0 || vbIdx >= vbCount) return 0;
		if (vbIdx >= (int)vbInfo.size()) return 0;
		if (vbIdx >= (int)entry.ResourceFixups.size()) return 0;

		const auto& vbFixup = entry.ResourceFixups[vbIdx];
		std::uint32_t vbOffset = m_FormulaService.ResolveFixupOffset(
			vbFixup.Address, vbFixup.AddressUpperBits, vbFixup.AddressLocationHighBits);

		std::uint32_t vbLength = vbInfo[vbIdx].DataLength;

		if ((std::int64_t)vbOffset + vbLength > (std::int64_t)pageData.size())
		{
			return 0;
		}

		const std::uint8_t* vertexBuffer = pageData.data() + vbOffset;

		std::uint32_t vCountInfo = vbInfo[vbIdx].Aux;
		std::uint32_t stride = k_VertexBufferStride;

		if (vCountInfo > 0 && (vbLength % vCountInfo) == 0)
		{
			stride = vbLength / vCountInfo;
		}
		else if (vCountInfo > 0)
		{
			m_LogsService.Message("[GeometryMeshDecoderService] WARNING: '{}':"
				" VB {} DataLength={} not divisible by VertexCount={},"
				" stride fallback 0x{:X} (VertexType={}).", tagName ? 
				tagName : "?", vbIdx, vbLength, vCountInfo, 
				k_VertexBufferStride, (unsigned)sec.VertexType);
		}

		if (stride == 0) return 0;
		std::uint32_t vertexCount = vbLength / stride;
		if (vertexCount == 0) return 0;

		bool hasBounds = false;

		float minX = 0, minY = 0, minZ = 0,
			lengthX = 1, lengthY = 1, lengthZ = 1;

		if (compressionInfo != nullptr)
		{
			const auto& bx = compressionInfo->PositionBoundsX;
			const auto& by = compressionInfo->PositionBoundsY;
			const auto& bz = compressionInfo->PositionBoundsZ;

			hasBounds = !(bx.Min == bx.Max &&
				by.Min == by.Max && bz.Min == bz.Max);

			if (hasBounds)
			{
				minX = bx.Min; lengthX = bx.Max - bx.Min;
				minY = by.Min; lengthY = by.Max - by.Min;
				minZ = bz.Min; lengthZ = bz.Max - bz.Min;
			}
		}

		VertexDecodeContext context{};
		context.Buffer = vertexBuffer;
		context.Count = vertexCount;
		context.Stride = stride;
		context.HasBounds = hasBounds;
		context.MinX = minX;
		context.MinY = minY;
		context.MinZ = minZ;
		context.LengthX = lengthX;
		context.LengthY = lengthY;
		context.LengthZ = lengthZ;
		context.TransformMatrix = transformMatrix;

		std::uint32_t emitted = 0;

		bool unindexed = (ibIdx < 0) ||
			(sec.MeshFlags & k_MeshFlagUnindexed);

		if (unindexed)
		{
			for (std::uint32_t i = 0; i + 2 < vertexCount; ++i)
			{
				if (i & 1)
				{
					this->PushTriangle(i, i + 2, i + 1, context, emitted, out);
				}
				else
				{
					this->PushTriangle(i, i + 1, i + 2, context, emitted, out);
				}
			}

			return emitted;
		}

		if (ibIdx >= (int)ibInfo.size()) return emitted;

		int fixupIB = vbCount * 2 + ibIdx;
		if (fixupIB < 0 || fixupIB >= (int)entry.ResourceFixups.size())
		{
			return emitted;
		}

		const auto& ibFixup = entry.ResourceFixups[fixupIB];
		std::uint32_t ibOffset = m_FormulaService.ResolveFixupOffset(
			ibFixup.Address, ibFixup.AddressUpperBits, ibFixup.AddressLocationHighBits);

		std::uint32_t ibLength = ibInfo[ibIdx].DataLength;

		if ((std::int64_t)ibOffset + ibLength > (std::int64_t)pageData.size())
		{
			return emitted;
		}

		bool wide = (vertexCount > (std::numeric_limits<std::uint16_t>::max)());
		std::uint32_t idxStride = wide ? sizeof(std::uint32_t) : sizeof(std::uint16_t);
		std::uint32_t idxCount = ibLength / idxStride;
		const std::uint8_t* ibPointer = pageData.data() + ibOffset;

		bool isStrip = (sec.IndexBufferType == 5) || (sec.IndexBufferType == 0);

		if (!sec.Parts.empty())
		{
			for (const auto& part : sec.Parts)
			{
				this->EmitRange(part.IndexStart, part.IndexCount,
					context, ibPointer, wide, idxCount, isStrip, emitted, out);
			}
		}
		else
		{
			this->EmitRange(0, idxCount, context, ibPointer,
				wide, idxCount, isStrip, emitted, out);
		}

		return emitted;
	}

	auto GeometryMeshDecoderService::EmitInstancedGeometry(
		const std::vector<std::uint8_t>& pageData, 
		const TagResourcesObject& lbspEntry,
		std::int32_t vbCount, const std::vector<BufferInfo>& vbInfo,
		const std::vector<BufferInfo>& ibInfo, const SbspObject* sbsp,
		const LbspObject* lbsp, const ZoneObject* zone,
		std::int64_t fixupDataBase, const char* tagName,
		std::vector<Triangle>& out) const -> std::uint32_t
	{
		if (sbsp->InstancedGeometryInstances.empty()) return 0;

		int instResIdx = (int)(sbsp->Data.ZoneAssetDatum5 & k_ResourceDatumMask);
		if (instResIdx < 0 || instResIdx >= (int)zone->TagResources.size()) return 0;

		const TagResourcesObject& instEntry = zone->TagResources[instResIdx];

		int fixupIdx = (int)instEntry.ResourceFixups.size() - k_InstanceFixupFromEnd;
		if (fixupIdx < 0 || fixupIdx >= (int)instEntry.ResourceFixups.size())
		{
			m_LogsService.Message("[GeometryMeshDecoderService] WARNING: '{}':"
				" InstancesEntry.ResourceFixups too short (Count={}, need >={})", 
				tagName, (int)instEntry.ResourceFixups.size(), k_InstanceFixupFromEnd);
			return 0;
		}

		const auto& fixup = instEntry.ResourceFixups[fixupIdx];

		// TODO: NOTE: This address does NOT use Formula::ResolveFixupOffset intentionally.
		// It's a different formula than EmitSection: it sums
		// instEntry.FixupInformationOffset and does not use AddressLocationHighBits.
		// Preserved as is from the original — do not merge with the one above.
		std::uint32_t rawOffset = ((std::uint32_t)fixup.AddressUpperBits << 16) | fixup.Address;
		std::uint32_t address = ((std::uint32_t)instEntry.FixupInformationOffset + rawOffset) & k_FixupMask;
		std::int64_t transformsBase = fixupDataBase + address;

		int instCount = (int)sbsp->InstancedGeometryInstances.size();

		auto blob = m_DataStreamService.ReadData(
			m_FileStore.GetMapFilePath(), transformsBase,
			k_InstanceStride * instCount);

		std::uint32_t emitted = 0;

		for (int i = 0; i < instCount; ++i)
		{
			const std::int64_t base = (std::int64_t)i * k_InstanceStride;
			if (base + k_InstanceStride > (std::int64_t)blob.size()) break;

			const std::uint8_t* pointer = blob.data() + base;

			float scale;
			std::memcpy(&scale, pointer + k_InstanceScaleOffset, sizeof(scale));

			float m[k_InstanceMatrixFloats]{};
			for (int k = 0; k < k_InstanceMatrixFloats; ++k)
			{
				std::memcpy(&m[k], pointer + k_InstanceMatrixOffset + k * (int)sizeof(float), sizeof(float));
			}

			for (int k = 0; k < k_InstanceScaledFloats; ++k)
			{
				m[k] *= scale;
			}

			std::int16_t sectionIdx16;
			std::memcpy(&sectionIdx16, pointer + k_InstanceSectionOffset, sizeof(sectionIdx16));

			int sectionIdx = (int)sectionIdx16;
			if (sectionIdx < 0 || sectionIdx >= (int)lbsp->Meshes.size()) continue;

			const MeshesObject& section = lbsp->Meshes[sectionIdx];

			emitted += this->EmitSection(pageData, lbspEntry, vbCount,
				vbInfo, ibInfo, section, this->GetCompressionInfo(sectionIdx, sbsp),
				tagName, m, out);
		}

		return emitted;
	}

	auto GeometryMeshDecoderService::GetCompressionInfo(int sectionIdx,
		const SbspObject* sbsp) const -> const CompressionInfoEntry3*
	{
		if (sectionIdx < 0 || sectionIdx >= (int)sbsp->CompressionInfo_3.size())
		{
			return nullptr;
		}

		return &sbsp->CompressionInfo_3[sectionIdx];
	}

	void GeometryMeshDecoderService::EmitRange(
		std::uint32_t start, std::uint32_t count,
		const VertexDecodeContext& context,
		const std::uint8_t* ibPointer, bool wide,
		std::uint32_t idxCount, bool isStrip,
		std::uint32_t& emitted, std::vector<Triangle>& out) const
	{
		if (count == 0) return;

		std::uint64_t end = (std::uint64_t)start + count;
		if (end > idxCount) end = idxCount;
		if (start >= end) return;

		if (!isStrip)
		{
			for (std::uint64_t i = start; i + 2 < end; i += 3)
			{
				this->PushTriangle(
					this->ReadIndex((std::uint32_t)i, wide, ibPointer),
					this->ReadIndex((std::uint32_t)i + 1, wide, ibPointer),
					this->ReadIndex((std::uint32_t)i + 2, wide, ibPointer),
					context, emitted, out);
			}

			return;
		}

		std::uint32_t i0 = 0, i1 = 0, i2 = 0;
		std::uint64_t pos = 0;

		for (std::uint64_t i = start; i < end; ++i)
		{
			std::uint32_t idx = this->ReadIndex((std::uint32_t)i, wide, ibPointer);

			i0 = i1;
			i1 = i2;
			i2 = idx;

			if (pos++ < 2) continue;
			if (i0 == i1 || i0 == i2 || i1 == i2) continue;

			if (pos & 1)
			{
				this->PushTriangle(i0, i1, i2, context, emitted, out);
			}
			else
			{
				this->PushTriangle(i0, i2, i1, context, emitted, out);
			}
		}
	}

	auto GeometryMeshDecoderService::PushTriangle(std::uint32_t a, std::uint32_t b,
		std::uint32_t c, const VertexDecodeContext& context, 
		std::uint32_t& emitted, std::vector<Triangle>& out) const -> void
	{
		if (a == b || b == c || a == c) return;
		if (a >= context.Count || b >= context.Count || c >= context.Count) return;

		Triangle triangle{};
		triangle.A = this->ReadVertex(a, context);
		triangle.B = this->ReadVertex(b, context);
		triangle.C = this->ReadVertex(c, context);

		out.push_back(triangle);
		++emitted;
	}

	auto GeometryMeshDecoderService::ReadVertex(std::uint32_t idx,
		const VertexDecodeContext& context) const -> Vec3
	{
		if (idx >= context.Count) return Vec3{};

		const std::uint8_t* pointer = context.Buffer + (std::size_t)idx * context.Stride;

		float x{}, y{}, z{};

		std::memcpy(&x, pointer + k_XOffset, sizeof(x));
		std::memcpy(&y, pointer + k_YOffset, sizeof(y));
		std::memcpy(&z, pointer + k_ZOffset, sizeof(z));

		if (context.HasBounds)
		{
			x = context.MinX + x * context.LengthX;
			y = context.MinY + y * context.LengthY;
			z = context.MinZ + z * context.LengthZ;
		}

		if (context.TransformMatrix)
		{
			const float* m = context.TransformMatrix;
			float worldX = x * m[0] + y * m[3] + z * m[6] + m[9];
			float worldY = x * m[1] + y * m[4] + z * m[7] + m[10];
			float worldZ = x * m[2] + y * m[5] + z * m[8] + m[11];

			return { worldX, worldY, worldZ };
		}

		return { x, y, z };
	}

	auto GeometryMeshDecoderService::ReadIndex(std::uint32_t i, bool wide,
		const std::uint8_t* ibPointer) const -> std::uint32_t
	{
		if (wide)
		{
			std::uint32_t value;
			std::memcpy(&value, ibPointer + (std::size_t)i * sizeof(value), sizeof(value));
			return value;
		}

		std::uint16_t value;
		std::memcpy(&value, ibPointer + (std::size_t)i * sizeof(value), sizeof(value));
		return value;
	}
}