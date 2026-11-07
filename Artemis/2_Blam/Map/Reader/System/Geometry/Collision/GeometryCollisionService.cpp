module Blam.Map.Reader.System;
import :Geometry.Collision;

import Common.Math.Type;
import Common.Math.System;
import Common.Engine.Type;
import Common.Geometry.System;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Math::Type::Triangle;
    using Common::Math::System::Cross;
    using Common::Math::System::Length;
    using Common::Math::System::Subtract;
    using Common::Geometry::System::AppendCollisionSurface;
    using Blam::Map::Tag::Type::Zone::Object::Zone_TagResourcesObject;
    using Blam::Map::Reader::Type::Geometry::CollisionInstance;

    using Blam::Map::Reader::Type::Constant::k_FixupLocationDefinition;
    using Blam::Map::Reader::Type::Constant::k_FixupLocationMask;
    using Blam::Map::Reader::Type::Constant::k_FixupLocationPage;
    using Blam::Map::Reader::Type::Constant::k_TagBlockSize;
    using Blam::Map::Reader::Type::Constant::k_TagBlockAddressOffset;
    using Blam::Map::Reader::Type::Constant::k_CollisionRootSize;
    using Blam::Map::Reader::Type::Constant::k_CollisionRootNormalBlock;
    using Blam::Map::Reader::Type::Constant::k_CollisionRootLargeBlock;
    using Blam::Map::Reader::Type::Constant::k_CollisionRootInstancedBlock;
    using Blam::Map::Reader::Type::Constant::k_CollisionSurfacesBlock;
    using Blam::Map::Reader::Type::Constant::k_CollisionEdgesBlock;
    using Blam::Map::Reader::Type::Constant::k_CollisionVerticesBlock;
    using Blam::Map::Reader::Type::Constant::k_CollisionNodesBlock;
    using Blam::Map::Reader::Type::Constant::k_InstanceDefinitionStride;
    using Blam::Map::Reader::Type::Constant::k_InstanceDefinitionCollisionOffset;
    using Blam::Map::Reader::Type::Constant::k_InstanceDefinitionLargeOffset;
    using Blam::Map::Reader::Type::Constant::k_InstanceStride;
    using Blam::Map::Reader::Type::Constant::k_InstanceMatrixFloats;
    using Blam::Map::Reader::Type::Constant::k_InstanceScaledFloats;
    using Blam::Map::Reader::Type::Constant::k_InstanceScaleOffset;
    using Blam::Map::Reader::Type::Constant::k_InstanceMatrixOffset;
    using Blam::Map::Reader::Type::Constant::k_InstanceDefinitionOffset;
    using Blam::Map::Reader::Type::Constant::k_InstanceFlagsOffset;
    using Common::Engine::Type::k_NoHandle;

    // Address of the elements of an empty block, so that an empty block is not mistaken for a missing one.
    constexpr std::uint8_t k_NoBytes{};

    // None, as a 16 bit index of the normal layout.
    constexpr std::uint16_t k_NoIndex16{ (std::numeric_limits<std::uint16_t>::max)() };

    // Below this length of the cross product (twice the area, in square meters) a triangle is degenerate and dropped.
    constexpr float k_MinDoubleArea{ 1e-8f };

    // Element layout of a collision geometry: the normal one uses 16 bit indices, the large one 32 bit indices.
    struct Layout
    {
        std::int32_t SurfaceStride{};
        std::int32_t SurfaceFirstEdge{};
        std::int32_t IndexSize{};
        std::int32_t SurfaceMaterial{};
        std::int32_t SurfaceFlags{};
        std::int32_t EdgeStride{};
        std::int32_t VertexStride{};
    };

    constexpr Layout k_Normal{ .SurfaceStride = 12, .SurfaceFirstEdge = 2, .IndexSize = 2, .SurfaceMaterial = 4, .SurfaceFlags = 10, .EdgeStride = 12, .VertexStride = 16 };
    constexpr Layout k_Large{ .SurfaceStride = 16, .SurfaceFirstEdge = 4, .IndexSize = 4, .SurfaceMaterial = 8, .SurfaceFlags = 14, .EdgeStride = 24, .VertexStride = 20 };

    // Elements as the shared surface walk reads them.
    struct RawSurface
    {
        std::int32_t FirstEdge{};
        std::uint8_t Flags{};
        std::int16_t Material{ -1 };
    };

    struct RawEdge
    {
        std::int32_t StartVertex{};
        std::int32_t EndVertex{};
        std::int32_t ForwardEdge{};
        std::int32_t ReverseEdge{};
        std::int32_t LeftSurface{};
        std::int32_t RightSurface{};
    };

    struct RawVertex
    {
        Vec3 Point{};
    };

    // Where the address of a block points to.
    struct Target
    {
        std::uint32_t Offset{};
        std::uint8_t Location{};
    };

    // A tag block of the definition, resolved to its elements.
    struct BlockView
    {
        bool IsValid{ true };
        std::int32_t Count{};
        std::uint8_t Location{};
        std::uint32_t Offset{};
    };

    // The definition and the raw page of a collision resource, with the fixups by position of the address they patch.
    struct Resource
    {
        std::vector<std::uint8_t> Definition{};
        std::vector<std::uint8_t> Page{};
        std::unordered_map<std::int32_t, Target> Targets{};
    };

    // What one BSP gave besides its triangles.
    struct Counters
    {
        std::uint32_t OpenSurfaces{};
        std::uint32_t Degenerate{};
    };

    template <typename T>
    auto ReadAs(const std::uint8_t* pointer) -> T
    {
        T value{};
        std::memcpy(&value, pointer, sizeof(value));
        return value;
    }

    // return: The index at pointer. The 16 bit indices of the normal layout are unsigned, with 0xFFFF as none.
    auto ReadIndex(const std::uint8_t* pointer, std::int32_t size) -> std::int32_t
    {
        if (size == 2)
        {
            const std::uint16_t index{ ReadAs<std::uint16_t>(pointer) };
            return index == k_NoIndex16 ? -1 : index;
        }

        return ReadAs<std::int32_t>(pointer);
    }

    auto BuildTargets(const Zone_TagResourcesObject& entry,
        const Blam::Map::Reader::System::FormulaService& formula) -> std::unordered_map<std::int32_t, Target>
    {
        std::unordered_map<std::int32_t, Target> targets{};
        targets.reserve(entry.ResourceFixups.size());

        for (const auto& fixup : entry.ResourceFixups)
        {
            targets[fixup.BlockOffset] = {
                .Offset = formula.ResolveFixupOffset(fixup.Address, fixup.AddressUpperBits, fixup.AddressLocationHighBits),
                .Location = static_cast<std::uint8_t>(fixup.AddressLocationHighBits & k_FixupLocationMask) };
        }

        return targets;
    }

    // param position: Offset in the definition of the tag block (count, address, definition).
    auto ResolveBlock(const Resource& resource, std::int64_t position) -> BlockView
    {
        BlockView view{};

        if (position < 0 || position + k_TagBlockSize > static_cast<std::int64_t>(resource.Definition.size()))
        {
            view.IsValid = false;
            return view;
        }

        view.Count = ReadAs<std::int32_t>(resource.Definition.data() + position);
        if (view.Count <= 0)
        {
            view.Count = 0;
            return view;
        }

        const auto it = resource.Targets.find(static_cast<std::int32_t>(position + k_TagBlockAddressOffset));
        if (it == resource.Targets.end())
        {
            view.IsValid = false;
            return view;
        }

        view.Location = it->second.Location;
        view.Offset = it->second.Offset;
        return view;
    }

    // return: The bytes of the elements of a block, or null if its address or its size leave the resource.
    auto ElementsOf(const Resource& resource, const BlockView& view, std::int32_t stride) -> const std::uint8_t*
    {
        if (!view.IsValid) return nullptr;
        if (view.Count == 0) return &k_NoBytes;

        const std::vector<std::uint8_t>& bytes = view.Location == k_FixupLocationPage ? resource.Page : resource.Definition;
        if (view.Location != k_FixupLocationPage && view.Location != k_FixupLocationDefinition) return nullptr;

        const std::uint64_t end = static_cast<std::uint64_t>(view.Offset) + static_cast<std::uint64_t>(view.Count) * static_cast<std::uint64_t>(stride);
        if (end > bytes.size()) return nullptr;

        return bytes.data() + view.Offset;
    }

    // Reads one collision geometry (nine tag blocks) and appends its surfaces as triangles.
    // param position: Offset in the definition of the first block of the geometry.
    // return: False if a block is missing or leaves the resource. Triangles may have been appended then.
    auto ReadGeometry(const Resource& resource, std::int64_t position, const Layout& layout,
        std::vector<Triangle>& out, Counters& counters) -> bool
    {
        const BlockView surfaceBlock{ ResolveBlock(resource, position + k_CollisionSurfacesBlock) };
        const BlockView edgeBlock{ ResolveBlock(resource, position + k_CollisionEdgesBlock) };
        const BlockView vertexBlock{ ResolveBlock(resource, position + k_CollisionVerticesBlock) };

        const std::uint8_t* surfaceBytes{ ElementsOf(resource, surfaceBlock, layout.SurfaceStride) };
        const std::uint8_t* edgeBytes{ ElementsOf(resource, edgeBlock, layout.EdgeStride) };
        const std::uint8_t* vertexBytes{ ElementsOf(resource, vertexBlock, layout.VertexStride) };
        if (!surfaceBytes || !edgeBytes || !vertexBytes) return false;

        std::vector<RawSurface> surfaces(static_cast<std::size_t>(surfaceBlock.Count));
        for (std::size_t i = 0; i < surfaces.size(); ++i)
        {
            const std::uint8_t* pointer = surfaceBytes + i * static_cast<std::size_t>(layout.SurfaceStride);

            // The first edge is unsigned in the normal layout.
            surfaces[i].FirstEdge = layout.IndexSize == 2 ? ReadAs<std::uint16_t>(pointer + layout.SurfaceFirstEdge) : ReadAs<std::int32_t>(pointer + layout.SurfaceFirstEdge);
            surfaces[i].Material = ReadAs<std::int16_t>(pointer + layout.SurfaceMaterial);
            surfaces[i].Flags = ReadAs<std::uint8_t>(pointer + layout.SurfaceFlags);
        }

        std::vector<RawEdge> edges(static_cast<std::size_t>(edgeBlock.Count));
        for (std::size_t i = 0; i < edges.size(); ++i)
        {
            const std::uint8_t* pointer = edgeBytes + i * static_cast<std::size_t>(layout.EdgeStride);

            edges[i].StartVertex = ReadIndex(pointer, layout.IndexSize);
            edges[i].EndVertex = ReadIndex(pointer + layout.IndexSize, layout.IndexSize);
            edges[i].ForwardEdge = ReadIndex(pointer + 2 * layout.IndexSize, layout.IndexSize);
            edges[i].ReverseEdge = ReadIndex(pointer + 3 * layout.IndexSize, layout.IndexSize);
            edges[i].LeftSurface = ReadIndex(pointer + 4 * layout.IndexSize, layout.IndexSize);
            edges[i].RightSurface = ReadIndex(pointer + 5 * layout.IndexSize, layout.IndexSize);
        }

        std::vector<RawVertex> vertices(static_cast<std::size_t>(vertexBlock.Count));
        for (std::size_t i = 0; i < vertices.size(); ++i)
        {
            vertices[i].Point = ReadAs<Vec3>(vertexBytes + i * static_cast<std::size_t>(layout.VertexStride));
        }

        const std::size_t first{ out.size() };
        out.reserve(out.size() + surfaces.size() * 2);

        for (std::int32_t surface = 0; surface < static_cast<std::int32_t>(surfaces.size()); ++surface)
        {
            if (AppendCollisionSurface(surfaces, edges, vertices, surface, out) == 0) ++counters.OpenSurfaces;
        }

        // Surfaces whose polygon has no area cannot be hit by a ray.
        const auto degenerate = [](const Triangle& triangle)
        {
            return Length(Cross(Subtract(triangle.B, triangle.A), Subtract(triangle.C, triangle.A))) < k_MinDoubleArea;
        };

        const auto removed = std::remove_if(out.begin() + static_cast<std::ptrdiff_t>(first), out.end(), degenerate);
        counters.Degenerate += static_cast<std::uint32_t>(out.end() - removed);
        out.erase(removed, out.end());

        return true;
    }
}

namespace Blam::Map::Reader::System
{
    auto GeometryCollisionService::ReadCollision(const std::vector<std::string>& sbspTagNames) const -> std::vector<SbspCollision>
    {
        std::vector<SbspCollision> result{};

        const ZoneObject* zone = m_TagCatalog.Zone.Get("__zone__");
        const PlayObject* play = m_TagCatalog.Play.Get("__play__");
        if (!zone || !play) return result;

        const std::int64_t fixupDataBase{ m_GeometryResourcePageService.FixupDataBase(zone) };

        result.reserve(sbspTagNames.size());

        for (const std::string& tagName : sbspTagNames)
        {
            const SbspObject* sbsp = m_TagCatalog.Sbsp.Get(tagName);
            if (!sbsp) continue;

            // Without resource items the collision is inline in the tag (0x50C). No known map uses it.
            std::uint32_t inlineDescriptor{};
            std::memcpy(&inlineDescriptor, sbsp->Data._pad_0x50C, sizeof(inlineDescriptor));
            if (sbsp->Data.Unknown_116 == 0)
            {
                if (inlineDescriptor != 0)
                {
                    m_Log.Message("[GeometryCollisionService] WARNING: '{}': Inline collision is not supported.", tagName);
                }

                continue;
            }

            const std::uint32_t datum{ sbsp->Data.ZoneAssetDatum4 };
            if (datum == k_NoHandle)
            {
                m_Log.Message("[GeometryCollisionService] INFO: '{}': No collision resource.", tagName);
                continue;
            }

            const Zone_TagResourcesObject* entry{ nullptr };

            Resource resource{};
            resource.Page = m_GeometryResourcePageService.Read(datum, &entry, zone, play);
            if (!entry)
            {
                m_Log.Message("[GeometryCollisionService] WARNING: '{}': Collision resource out of range.", tagName);
                continue;
            }

            resource.Definition = m_GeometryInfoService.ReadDefinition(*entry, fixupDataBase);
            resource.Targets = BuildTargets(*entry, m_FormulaService);

            const FormulaService& formula = m_FormulaService;
            const std::uint32_t rootOffset{ formula.ResolveFixupOffset(entry->RootDefinitionAddress,
                entry->RootDefinitionAddressUpperBits, entry->RootDefinitionAddressLocationHighBits) };
            if ((entry->RootDefinitionAddressLocationHighBits & k_FixupLocationMask) != k_FixupLocationDefinition ||
                static_cast<std::uint64_t>(rootOffset) + k_CollisionRootSize > resource.Definition.size())
            {
                m_Log.Message("[GeometryCollisionService] WARNING: '{}': Invalid collision root.", tagName);
                continue;
            }

            // A bsp that is not part of the loaded zone set has its definition but no page.
            if (resource.Page.empty())
            {
                m_Log.Message("[GeometryCollisionService] INFO: '{}': Collision page not loaded.", tagName);
                continue;
            }

            SbspCollision collision{};
            collision.TagName = tagName;
            Counters counters{};

            const BlockView normalBsps{ ResolveBlock(resource, rootOffset + k_CollisionRootNormalBlock) };
            const BlockView largeBsps{ ResolveBlock(resource, rootOffset + k_CollisionRootLargeBlock) };
            const BlockView definitions{ ResolveBlock(resource, rootOffset + k_CollisionRootInstancedBlock) };

            // The first bsp of the block. The element is the geometry itself.
            const auto readStructure = [&](const BlockView& block, const Layout& layout)
            {
                if (block.Count == 0) return;
                if (block.Count > 1)
                {
                    m_Log.Message("[GeometryCollisionService] WARNING: '{}': {} collision bsps, only the first one is read.", tagName, block.Count);
                }

                if (!block.IsValid || block.Location != k_FixupLocationDefinition ||
                    !ReadGeometry(resource, block.Offset, layout, collision.Structure, counters))
                {
                    m_Log.Message("[GeometryCollisionService] WARNING: '{}': Invalid structure collision.", tagName);
                }
            };

            readStructure(normalBsps, k_Normal);
            readStructure(largeBsps, k_Large);
            collision.IsLarge = largeBsps.Count > 0;

            if (definitions.Count > 0)
            {
                const std::uint64_t end{ static_cast<std::uint64_t>(definitions.Offset) + static_cast<std::uint64_t>(definitions.Count) * k_InstanceDefinitionStride };
                if (!definitions.IsValid || definitions.Location != k_FixupLocationDefinition || end > resource.Definition.size())
                {
                    m_Log.Message("[GeometryCollisionService] WARNING: '{}': Invalid instanced geometry definitions.", tagName);
                }
                else
                {
                    collision.Definitions.resize(static_cast<std::size_t>(definitions.Count));

                    for (std::int32_t i = 0; i < definitions.Count; ++i)
                    {
                        const std::int64_t entryPosition{ static_cast<std::int64_t>(definitions.Offset) + static_cast<std::int64_t>(i) * k_InstanceDefinitionStride };

                        // A second geometry with elements would be the large layout. No known map has one.
                        if (ResolveBlock(resource, entryPosition + k_InstanceDefinitionLargeOffset + k_CollisionNodesBlock).Count > 0)
                        {
                            ++collision.UnreadDefinitions;
                        }

                        if (!ReadGeometry(resource, entryPosition + k_InstanceDefinitionCollisionOffset, k_Normal, collision.Definitions[i], counters))
                        {
                            ++collision.UnreadDefinitions;
                        }
                    }
                }
            }

            if (!collision.Definitions.empty())
            {
                const std::vector<std::uint8_t> table{ m_GeometryMeshDecoderService.ReadInstanceTable(sbsp, zone, fixupDataBase, tagName.c_str()) };

                collision.Instances.reserve(table.size() / k_InstanceStride);

                for (std::size_t base = 0; base + k_InstanceStride <= table.size(); base += k_InstanceStride)
                {
                    const std::uint8_t* pointer = table.data() + base;

                    CollisionInstance instance{};
                    instance.DefinitionIndex = ReadAs<std::int16_t>(pointer + k_InstanceDefinitionOffset);
                    instance.Flags = ReadAs<std::uint16_t>(pointer + k_InstanceFlagsOffset);

                    if (instance.DefinitionIndex < 0 || instance.DefinitionIndex >= static_cast<std::int16_t>(collision.Definitions.size()))
                    {
                        ++collision.InvalidPlacements;
                        continue;
                    }

                    const float scale{ ReadAs<float>(pointer + k_InstanceScaleOffset) };
                    for (int k = 0; k < k_InstanceMatrixFloats; ++k)
                    {
                        instance.Matrix[k] = ReadAs<float>(pointer + k_InstanceMatrixOffset + k * static_cast<int>(sizeof(float)));
                        if (k < k_InstanceScaledFloats) instance.Matrix[k] *= scale;
                    }

                    collision.Instances.push_back(instance);
                }
            }

            collision.OpenSurfaces = counters.OpenSurfaces;
            collision.DegenerateTriangles = counters.Degenerate;

            std::uint64_t placedTriangles{};
            for (const CollisionInstance& instance : collision.Instances)
            {
                placedTriangles += collision.Definitions[static_cast<std::size_t>(instance.DefinitionIndex)].size();
            }

            m_Log.Message("[GeometryCollisionService] INFO: '{}': {} structure tris ({}), {} definitions, {} placements with {} tris,"
                " {} open surfaces, {} degenerate tris dropped.", tagName, collision.Structure.size(),
                collision.IsLarge ? "large" : "normal", collision.Definitions.size(), collision.Instances.size(),
                placedTriangles, collision.OpenSurfaces, collision.DegenerateTriangles);

            if (collision.UnreadDefinitions > 0 || collision.InvalidPlacements > 0)
            {
                m_Log.Message("[GeometryCollisionService] WARNING: '{}': {} definitions not read, {} placements out of range.",
                    tagName, collision.UnreadDefinitions, collision.InvalidPlacements);
            }

            result.push_back(std::move(collision));
        }

        return result;
    }
}