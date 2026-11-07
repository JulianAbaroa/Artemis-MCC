module;

#include <d3d11.h>

module DevTools.Viewer.Map.System;
import :MapPass;

import Engine.Render.Type;
import Engine.Render.System;
import DevTools.Viewer.Style.Type;
import std;

namespace
{
    using Engine::Render::Type::VertexLayout;

    using Engine::Render::Type::Vertex;

    using DevTools::Viewer::Style::Type::Color;
    using DevTools::Viewer::Style::Type::k_Map;
    using DevTools::Viewer::Style::Type::k_MapCollision;
    using DevTools::Viewer::Style::Type::k_MapCollisionInvisible;
    using DevTools::Viewer::Style::Type::k_MapCollisionTwoSided;

    // Bits of the flags of a collision surface.
    constexpr std::uint8_t k_TwoSidedFlag{ 1u << 0 };
    constexpr std::uint8_t k_InvisibleFlag{ 1u << 1 };
}

namespace DevTools::Viewer::Map::System
{
    auto MapPass::CreateBuffer(ID3D11Device* device, const std::vector<Vertex>& vertices,
        ComPtr<ID3D11Buffer>& outBuffer, UINT& outCount) -> bool
    {
        const std::size_t byteWidth = vertices.size() * sizeof(Vertex);
        if (byteWidth > (std::numeric_limits<UINT>::max)())
        {
            m_Log.Message("[MapPass] ERROR: Failed to upload the map mesh, it is too big for one buffer.");
            return false;
        }

        D3D11_BUFFER_DESC desc{};
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.ByteWidth = static_cast<UINT>(byteWidth);
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA data{};
        data.pSysMem = vertices.data();

        if (FAILED(device->CreateBuffer(&desc, &data, outBuffer.GetAddressOf())))
        {
            m_Log.Message("[MapPass] ERROR: Failed to create the map vertex buffer.");
            return false;
        }

        outCount = static_cast<UINT>(vertices.size());
        return true;
    }

    auto MapPass::Upload(ID3D11Device* device, const std::unordered_map<std::string, Sbsp>& sbsps) -> void
    {
        if (m_IsUploaded || !device) return;

        m_IsUploaded = true;

        std::size_t triangleCount{ 0 };
        std::size_t collisionCount{ 0 };
        for (const auto& [tagName, sbsp] : sbsps)
        {
            triangleCount += sbsp.RenderGeometry.size();
            collisionCount += sbsp.CollisionGeometry.size();
        }

        if (triangleCount == 0 && collisionCount == 0)
        {
            m_Log.Message("[MapPass] WARNING: The map has no render or collision geometry.");
            return;
        }

        std::vector<Vertex> vertices{};
        vertices.reserve(triangleCount * 3);

        auto push = [&vertices](const auto& point, const Color& color)
        {
            vertices.push_back(Vertex{ point.X, point.Y, point.Z,
                color.R, color.G, color.B });
        };

        for (const auto& [tagName, sbsp] : sbsps)
        {
            for (const auto& triangle : sbsp.RenderGeometry)
            {
                push(triangle.A, k_Map);
                push(triangle.B, k_Map);
                push(triangle.C, k_Map);
            }
        }

        if (!vertices.empty() && this->CreateBuffer(device, vertices, m_VertexBuffer, m_VertexCount))
        {
            m_Log.Message("[MapPass] INFO: Uploaded map mesh,"
                " {} vertices ({} triangles).", m_VertexCount, triangleCount);
        }

        vertices.clear();
        vertices.reserve(collisionCount * 3);

        for (const auto& [tagName, sbsp] : sbsps)
        {
            for (const auto& triangle : sbsp.CollisionGeometry)
            {
                // Invisible wins over two sided, so the surfaces that only the rays see stand out.
                const Color& color = (triangle.SurfaceFlags & k_InvisibleFlag) ? k_MapCollisionInvisible :
                    (triangle.SurfaceFlags & k_TwoSidedFlag) ? k_MapCollisionTwoSided : k_MapCollision;

                push(triangle.A, color);
                push(triangle.B, color);
                push(triangle.C, color);
            }
        }

        if (!vertices.empty() && this->CreateBuffer(device, vertices, m_CollisionBuffer, m_CollisionCount))
        {
            m_Log.Message("[MapPass] INFO: Uploaded collision mesh,"
                " {} vertices ({} triangles).", m_CollisionCount, collisionCount);
        }
    }

    auto MapPass::IsUploaded() const -> bool
    {
        return m_IsUploaded;
    }

    auto MapPass::HasBuffer() const -> bool
    {
        return (m_VertexBuffer != nullptr && m_VertexCount > 0) || (m_CollisionBuffer != nullptr && m_CollisionCount > 0);
    }

    auto MapPass::Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline, bool isCollision) -> void
    {
        if (!context || !this->HasBuffer()) return;

        const bool hasCollision = m_CollisionBuffer != nullptr && m_CollisionCount > 0;
        const bool hasRender = m_VertexBuffer != nullptr && m_VertexCount > 0;
        const bool useCollision = hasCollision && (isCollision || !hasRender);

        pipeline.Bind(context, VertexLayout::Colored);

        if (useCollision)
        {
            Engine::Render::System::DrawVertexBuffer(context, m_CollisionBuffer.Get(),
                m_CollisionCount, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            return;
        }

        Engine::Render::System::DrawVertexBuffer(context, m_VertexBuffer.Get(),
            m_VertexCount, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    auto MapPass::Release() -> void
    {
        m_VertexBuffer.Reset();
        m_VertexCount = 0;
        m_CollisionBuffer.Reset();
        m_CollisionCount = 0;
        m_IsUploaded = false;
    }
}