#include "pch.h"

#include "ZonePass.h"
#include "ZoneGeometry.h"

#include "Core/Types/Tick/Tick.h"
#include "Core/Types/Environment/Fixtures/Fixtures.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

#include <vector>

void ZonePass::Upload(ID3D11Device* device, ID3D11DeviceContext* ctx,
    const std::shared_ptr<const Tick>& tick,
    const CollidableClassifier& classifier,
    uint64_t generation, System_Logs& logs)
{
    if (!device || !ctx) return;

    const bool sameTick = m_HasGeneration && generation == m_LastGeneration;
    if (sameTick) return;

    m_LastGeneration = generation;
    m_HasGeneration = true;

    m_SolidCount = 0;
    m_WireCount = 0;

    if (!tick || !tick->Fixtures) return;

    const Fixtures& fx = *tick->Fixtures;

    std::vector<GpuPipeline::Vertex> solid;
    std::vector<GpuPipeline::Vertex> wire;

    auto emit = [&](uint32_t handle,
        const std::array<float, 3>& pos,
        const std::array<float, 3>& fwd,
        const std::array<float, 3>& up,
        const ZoneShape& zone)
        {
            if (zone.ShapeType == ShapeType::None) return;
            const RgbColor col = classifier.ColorFor(handle);
            ZoneGeometry::AppendSolid(solid, pos, fwd, up, zone, col);
            ZoneGeometry::AppendWire(wire, pos, fwd, up, zone, col);
        };

    for (const auto& t : fx.Teleporters)
        emit(t.Handle, t.Position, t.Forward, t.Up, t.ZoneShape);

    for (const auto& o : fx.ObjectiveSpawns)
        emit(o.Handle, o.Position, o.Forward, o.Up, o.ZoneShape);

    auto upload = [&](ID3D11Buffer*& buf, UINT& cap, UINT& count,
        const std::vector<GpuPipeline::Vertex>& verts)
        {
            if (verts.empty()) { count = 0; return; }
            const UINT needed = static_cast<UINT>(verts.size());
            if (needed > cap)
            {
                UINT newCap = cap ? cap : 2048;
                while (newCap < needed) newCap *= 2;
                if (!Ensure(device, buf, cap, newCap, logs)) { count = 0; return; }
            }
            D3D11_MAPPED_SUBRESOURCE mapped;
            if (FAILED(ctx->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            {
                count = 0; return;
            }
            std::memcpy(mapped.pData, verts.data(),
                verts.size() * sizeof(GpuPipeline::Vertex));
            ctx->Unmap(buf, 0);
            count = needed;
        };

    upload(m_SolidBuffer, m_SolidCap, m_SolidCount, solid);
    upload(m_WireBuffer, m_WireCap, m_WireCount, wire);
}

bool ZonePass::Ensure(ID3D11Device* device, ID3D11Buffer*& buf, UINT& cap,
    UINT needed, System_Logs& logs)
{
    if (buf) { buf->Release(); buf = nullptr; }
    cap = 0;

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = needed * GpuPipeline::kVertexStride;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(device->CreateBuffer(&bd, nullptr, &buf)))
    {
        logs.Log("[ZonePass] ERROR: vertex buffer failed.");
        return false;
    }
    cap = needed;
    return true;
}

void ZonePass::DrawSolid(ID3D11DeviceContext* ctx)
{
    if (!ctx || !m_SolidBuffer || m_SolidCount == 0) return;
    UINT stride = GpuPipeline::kVertexStride, offset = 0;
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetVertexBuffers(0, 1, &m_SolidBuffer, &stride, &offset);
    ctx->Draw(m_SolidCount, 0);
}

void ZonePass::DrawWire(ID3D11DeviceContext* ctx)
{
    if (!ctx || !m_WireBuffer || m_WireCount == 0) return;
    UINT stride = GpuPipeline::kVertexStride, offset = 0;
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    ctx->IASetVertexBuffers(0, 1, &m_WireBuffer, &stride, &offset);
    ctx->Draw(m_WireCount, 0);
}

void ZonePass::Release()
{
    if (m_SolidBuffer) { m_SolidBuffer->Release(); m_SolidBuffer = nullptr; }
    if (m_WireBuffer) { m_WireBuffer->Release();  m_WireBuffer = nullptr; }
    m_SolidCap = m_WireCap = 0;
    m_SolidCount = m_WireCount = 0;
    m_LastGeneration = 0;
    m_HasGeneration = false;
}