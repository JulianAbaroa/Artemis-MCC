module;

#include <d3d11.h>
#include <wrl/client.h>

export module Platform.Render.System:GpuBuffer;

import Service.Logs.System;
import Platform.Render.Type;
import std;

namespace
{
    using LogsService = Service::Logs::System::LogsService;

    using Vertex = Platform::Render::Type::Vertex;
}

export namespace Platform::Render::System::GpuBuffer
{
    // Creates a dynamic vertex buffer for vertexCapacity vertices, replacing the previous one.
    // param capacity: Receives the new capacity, 0 if it fails.
    // param tag: Log prefix of the caller, e.g. "[MapPass]".
    // return: False if the buffer could not be created.
    auto CreateDynamicVertexBuffer(ID3D11Device* device, UINT vertexCapacity,
        Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, UINT& capacity, const char* tag,
        LogsService& logsService) -> bool;

    // Makes the buffer hold at least needed vertices, doubling the capacity from initialCapacity.
    // The content is lost when it grows.
    // return: False if the buffer could not be created.
    auto GrowDynamicVertexBuffer(ID3D11Device* device, UINT needed,
        UINT initialCapacity, Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, UINT& capacity,
        const char* tag, LogsService& logsService) -> bool;

    // Replaces the whole content of the buffer with the vertices.
    // note: The vertices must fit in the buffer.
    // return: False if the buffer could not be mapped.
    auto UploadDynamicVertices(ID3D11DeviceContext* context, ID3D11Buffer* buffer,
        std::span<const Vertex> vertices, const char* tag,
        LogsService& logsService) -> bool;

    // Binds the buffer to slot 0 and draws it.
    auto DrawVertexBuffer(ID3D11DeviceContext* context, ID3D11Buffer* buffer,
        UINT vertexCount, D3D11_PRIMITIVE_TOPOLOGY topology) -> void;

    // Remembers the last upload so callers skip uploading the same data again.
    class UploadGate
    {
    public:
        // return: True if the last Mark had the same generation and key.
        auto IsCurrent(std::uint64_t generation, std::uint32_t key = 0) const -> bool
        {
            return m_IsMarked && generation == m_Generation && key == m_Key;
        }

        // Records an upload of the generation and key.
        auto Mark(std::uint64_t generation, std::uint32_t key = 0) -> void
        {
            m_Generation = generation;
            m_Key = key;
            m_IsMarked = true;
        }

        auto Reset() -> void
        {
            m_Generation = 0;
            m_Key = 0;
            m_IsMarked = false;
        }

    private:
        std::uint64_t m_Generation{ 0 };
        std::uint32_t m_Key{ 0 };
        bool m_IsMarked{ false };
    };

    // Dynamic buffer of per-instance data. Grows by doubling and never shrinks.
    class InstanceBuffer
    {
    public:
        // Uploads count instances of stride bytes, growing the buffer if needed.
        // param initialCapacity: Instances reserved the first time.
        // param tag: Log prefix of the caller.
        // return: False if the input is invalid or a GPU call fails.
        auto Update(ID3D11Device* device, ID3D11DeviceContext* context,
            const void* data, UINT count, UINT stride, UINT initialCapacity,
            const char* tag, LogsService& logsService) -> bool;

        auto Get() const -> ID3D11Buffer*
        {
            return m_Buffer.Get();
        }

        auto Release() -> void
        {
            m_Buffer.Reset();
            m_Capacity = 0;
        }

    private:
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_Buffer{};
        UINT m_Capacity{ 0 };
    };
}