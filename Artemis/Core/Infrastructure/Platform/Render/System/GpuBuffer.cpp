module;

#include <d3d11.h>
#include <wrl/client.h>

module Platform.Render.System;
import :GpuBuffer;
import :GpuPipeline;

import std;

namespace
{
    using LogsService = Service::Logs::System::LogsService;

    using Vertex = Platform::Render::Type::Vertex;
}

namespace Platform::Render::System::GpuBuffer
{
    auto CreateDynamicVertexBuffer(ID3D11Device* device, UINT vertexCapacity,
        Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, UINT& capacity, const char* tag,
        LogsService& logsService) -> bool
    {
        buffer.Reset();
        capacity = 0;

        D3D11_BUFFER_DESC desc{};
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = vertexCapacity * GpuPipeline::k_VertexStride;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        if (FAILED(device->CreateBuffer(&desc, nullptr, buffer.GetAddressOf())))
        {
            logsService.Message("{} ERROR: Failed to create the vertex buffer.", tag);
            return false;
        }

        capacity = vertexCapacity;
        return true;
    }

    auto GrowDynamicVertexBuffer(ID3D11Device* device, UINT needed,
        UINT initialCapacity, Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, UINT& capacity,
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
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            logsService.Message("{} ERROR: Failed to map the instance buffer.", tag);
            return false;
        }

        std::memcpy(mapped.pData, vertices.data(), vertices.size() * sizeof(Vertex));
        context->Unmap(buffer, 0);

        return true;
    }

    auto DrawVertexBuffer(ID3D11DeviceContext* context, ID3D11Buffer* buffer,
        UINT vertexCount, D3D11_PRIMITIVE_TOPOLOGY topology) -> void
    {
        const UINT stride = GpuPipeline::k_VertexStride;
        const UINT offset = 0;

        context->IASetPrimitiveTopology(topology);
        context->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
        context->Draw(vertexCount, 0);
    }

    auto InstanceBuffer::Update(ID3D11Device* device, ID3D11DeviceContext* context,
        const void* data, UINT count, UINT stride, UINT initialCapacity,
        const char* tag, LogsService& logsService) -> bool
    {
        if (!device || !context || !data || count == 0) return false;

        if (count > m_Capacity || !m_Buffer)
        {
            UINT capacity = m_Capacity ? m_Capacity : initialCapacity;
            while (capacity < count) capacity *= 2;

            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = capacity * stride;
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            m_Capacity = 0;
            if (FAILED(device->CreateBuffer(&desc, nullptr, m_Buffer.ReleaseAndGetAddressOf())))
            {
                m_Buffer.Reset();
                logsService.Message("{} ERROR: Failed to create the instance buffer.", tag);
                return false;
            }

            m_Capacity = capacity;
        }

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(context->Map(m_Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            logsService.Message("{} ERROR: Failed to map the instance buffer.", tag);
            return false;
        }

        std::memcpy(mapped.pData, data, static_cast<std::size_t>(count) * stride);
        context->Unmap(m_Buffer.Get(), 0);

        return true;
    }
}