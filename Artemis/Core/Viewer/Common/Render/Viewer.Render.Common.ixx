module;

#include <d3d11.h>
#include <wrl/client.h>

export module Viewer.Render.Common;

import Service.Logs.System;
import Platform.Render.System;
import std;

export namespace Viewer::Render::Common
{
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    using Vertex = Platform::Render::System::GpuPipeline::Vertex;
    using LogsService = Service::Logs::System::LogsService;

    auto CreateDynamicVertexBuffer(ID3D11Device* device, UINT vertexCapacity,
        ComPtr<ID3D11Buffer>& buffer, UINT& capacity, const char* tag,
        LogsService& logsService) -> bool;

    auto GrowDynamicVertexBuffer(ID3D11Device* device, UINT needed,
        UINT initialCapacity, ComPtr<ID3D11Buffer>& buffer, UINT& capacity,
        const char* tag, LogsService& logsService) -> bool;

    auto UploadDynamicVertices(ID3D11DeviceContext* context, ID3D11Buffer* buffer,
        std::span<const Vertex> vertices, const char* tag,
        LogsService& logsService) -> bool;

    auto DrawVertexBuffer(ID3D11DeviceContext* context, ID3D11Buffer* buffer,
        UINT vertexCount, D3D11_PRIMITIVE_TOPOLOGY topology) -> void;
}