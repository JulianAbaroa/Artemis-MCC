module;

#include <d3d11.h>
#include <wrl/client.h>

module Viewer.Render.Common;

namespace Viewer::Render::Common
{
    auto CreateDynamicVertexBuffer(ID3D11Device* device, UINT vertexCapacity,
        ComPtr<ID3D11Buffer>& buffer, UINT& capacity, const char* tag,
        LogsService& logsService) -> bool
    {
        buffer.Reset();
        capacity = 0;

        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = vertexCapacity * Platform::Render::System::GpuPipeline::k_VertexStride;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        if (FAILED(device->CreateBuffer(&desc, nullptr, buffer.GetAddressOf())))
        {
            logsService.Message("{} ERROR: The vertex buffer failed.", tag);
            return false;
        }

        capacity = vertexCapacity;
        return true;
    }

    auto GrowDynamicVertexBuffer(ID3D11Device* device, UINT needed,
        UINT initialCapacity, ComPtr<ID3D11Buffer>& buffer, UINT& capacity,
        const char* tag, LogsService& logsService) -> bool
    {
        if (needed <= capacity) return true;

        UINT newCapacity = capacity ? capacity : initialCapacity;
        while (newCapacity < needed) newCapacity *= 2;

        return CreateDynamicVertexBuffer(device, newCapacity, buffer, capacity, tag, logsService);
    }

    auto UploadDynamicVertices(ID3D11DeviceContext* context, ID3D11Buffer* buffer,
        std::span<const Vertex> vertices, const char* tag,
        LogsService& logsService) -> bool
    {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            logsService.Message("{} ERROR: Map failed.", tag);
            return false;
        }

        std::memcpy(mapped.pData, vertices.data(), vertices.size() * sizeof(Vertex));
        context->Unmap(buffer, 0);

        return true;
    }

    auto DrawVertexBuffer(ID3D11DeviceContext* context, ID3D11Buffer* buffer,
        UINT vertexCount, D3D11_PRIMITIVE_TOPOLOGY topology) -> void
    {
        const UINT stride = Platform::Render::System::GpuPipeline::k_VertexStride;
        const UINT offset = 0;

        context->IASetPrimitiveTopology(topology);
        context->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
        context->Draw(vertexCount, 0);
    }
}