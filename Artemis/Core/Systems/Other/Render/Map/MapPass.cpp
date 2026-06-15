#include "pch.h"

#include "MapPass.h"

#include "../GpuPipeline.h"

#include "Core/Types/Sources/Static/World/SbspGeometry.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

#include <cstdio>
#include <vector>

namespace
{
    constexpr float kMapR = 0.45f;
    constexpr float kMapG = 0.78f;
    constexpr float kMapB = 0.96f;
}

void MapPass::Upload(ID3D11Device* device,
    const std::vector<SbspGeometry>& sbsps, System_Logs& logs)
{
    if (m_Uploaded) return;
    if (!device) return;

    size_t totalTris = 0;
    for (const auto& s : sbsps) totalTris += s.RenderGeometry.size();

    if (totalTris == 0)
    {
        logs.Log("[MapPass] WARN: map has no render geometry.");
        m_Uploaded = true;
        return;
    }

    std::vector<GpuPipeline::Vertex> verts;
    verts.reserve(totalTris * 3);

    auto push = [&](const SbspVec3& v) {
        verts.push_back({ v.X, v.Y, v.Z, kMapR, kMapG, kMapB });
        };

    for (const auto& s : sbsps)
        for (const auto& t : s.RenderGeometry)
        {
            push(t.V0);
            push(t.V1);
            push(t.V2);
        }

    m_VertexCount = static_cast<UINT>(verts.size());

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = m_VertexCount * GpuPipeline::kVertexStride;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = verts.data();

    if (FAILED(device->CreateBuffer(&bd, &init, &m_VertexBuffer)))
    {
        logs.Log("[MapPass] ERROR: map vertex buffer failed.");
        m_VertexCount = 0;
        return;
    }

    m_Uploaded = true;

    char buf[128];
    sprintf_s(buf, "[MapPass] INFO: uploaded map mesh, %u vertices (%zu tris).",
        m_VertexCount, totalTris);
    logs.Log(buf);
}

void MapPass::Draw(ID3D11DeviceContext* ctx)
{
    if (!ctx || !m_VertexBuffer || m_VertexCount == 0) return;

    UINT stride = GpuPipeline::kVertexStride;
    UINT offset = 0;

    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
    ctx->Draw(m_VertexCount, 0);
}

void MapPass::Release()
{
    if (m_VertexBuffer) { m_VertexBuffer->Release(); m_VertexBuffer = nullptr; }
    m_VertexCount = 0;
    m_Uploaded = false;
}