#include "pch.h"

#include "DynamicPass.h"

#include "../GpuPipeline.h"
#include "CollidableClassifier.h"

#include "Core/Types/Environment/Collidable/Collidable.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

#include <array>
#include <cstring>

void DynamicPass::Upload(ID3D11Device* device, ID3D11DeviceContext* ctx,
    const std::shared_ptr<const Collidables>& collidables,
    const CollidableClassifier& classifier,
    uint32_t selectedHandle,
    uint64_t generation, System_Logs& logs)
{
    if (!device || !ctx) return;

    const bool sameTick = m_HasGeneration && generation == m_LastGeneration;
    const bool sameSelection = selectedHandle == m_LastSelected;
    if (sameTick && sameSelection) return;

    m_LastGeneration = generation;
    m_HasGeneration = true;
    m_LastSelected = selectedHandle;

    m_AABBs.clear();

    if (!collidables || collidables->empty())
    {
        m_VertexCount = 0;
        return;
    }

    constexpr RgbColor kSelected = { 1.0f, 1.0f, 1.0f };

    std::vector<GpuPipeline::Vertex> verts;

    size_t totalTris = 0;
    for (const auto& c : *collidables)
        totalTris += c.CollidableMesh.Triangles.size();

    verts.reserve(totalTris * 3);
    m_AABBs.reserve(collidables->size());

    for (const auto& c : *collidables)
    {
        const bool selected = (selectedHandle != kNoSelection)
            && (c.Handle == selectedHandle);

        const RgbColor col = selected ? kSelected : classifier.ColorFor(c.Handle);

        const auto& tris = c.CollidableMesh.Triangles;
        if (tris.empty()) continue;

        float minX = tris[0].A[0], minY = tris[0].A[1], minZ = tris[0].A[2];
        float maxX = minX, maxY = minY, maxZ = minZ;

        auto acc = [&](const std::array<float, 3>& p) {
            if (p[0] < minX) minX = p[0]; if (p[0] > maxX) maxX = p[0];
            if (p[1] < minY) minY = p[1]; if (p[1] > maxY) maxY = p[1];
            if (p[2] < minZ) minZ = p[2]; if (p[2] > maxZ) maxZ = p[2];
            };

        for (const auto& t : tris)
        {
            verts.push_back({ t.A[0], t.A[1], t.A[2], col.r, col.g, col.b });
            verts.push_back({ t.B[0], t.B[1], t.B[2], col.r, col.g, col.b });
            verts.push_back({ t.C[0], t.C[1], t.C[2], col.r, col.g, col.b });
            acc(t.A); acc(t.B); acc(t.C);
        }

        m_AABBs.push_back({ c.Handle, minX, minY, minZ, maxX, maxY, maxZ });
    }

    if (verts.empty())
    {
        m_VertexCount = 0;
        return;
    }

    const UINT needed = static_cast<UINT>(verts.size());

    if (needed > m_Capacity)
    {
        UINT newCap = m_Capacity ? m_Capacity : 4096;
        while (newCap < needed) newCap *= 2;

        if (!EnsureCapacity(device, newCap, logs))
        {
            m_VertexCount = 0;
            return;
        }
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (FAILED(ctx->Map(m_VertexBuffer, 0,
        D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        logs.Log("[DynamicPass] ERROR: Map failed.");
        m_VertexCount = 0;
        return;
    }

    std::memcpy(mapped.pData, verts.data(),
        verts.size() * sizeof(GpuPipeline::Vertex));
    ctx->Unmap(m_VertexBuffer, 0);

    m_VertexCount = needed;
}

bool DynamicPass::EnsureCapacity(ID3D11Device* device, UINT vertexCapacity,
    System_Logs& logs)
{
    if (m_VertexBuffer) { m_VertexBuffer->Release(); m_VertexBuffer = nullptr; }
    m_Capacity = 0;

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = vertexCapacity * GpuPipeline::kVertexStride;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(device->CreateBuffer(&bd, nullptr, &m_VertexBuffer)))
    {
        logs.Log("[DynamicPass] ERROR: dynamic vertex buffer failed.");
        return false;
    }

    m_Capacity = vertexCapacity;
    return true;
}

void DynamicPass::Draw(ID3D11DeviceContext* ctx)
{
    if (!ctx || !m_VertexBuffer || m_VertexCount == 0) return;

    UINT stride = GpuPipeline::kVertexStride;
    UINT offset = 0;

    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
    ctx->Draw(m_VertexCount, 0);
}

void DynamicPass::Release()
{
    if (m_VertexBuffer) { m_VertexBuffer->Release(); m_VertexBuffer = nullptr; }
    m_Capacity = 0;
    m_VertexCount = 0;
    m_LastGeneration = 0;
    m_HasGeneration = false;
    m_LastSelected = kNoSelection;
    m_AABBs.clear();
}