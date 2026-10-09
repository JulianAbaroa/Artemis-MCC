module;

#include <d3d11.h>

export module Viewer.Map.System:LimitsPass;

import Service.Logs.System;
import Platform.Render.Type;
import Platform.Render.System;
import Export.Tick.Type;
import Viewer.Map.Type;
import std;

export namespace Viewer::Map::System
{
    // Draws the kill and safe zone volumes of the scenario as translucent solids with an outline.
    // The volumes never change while the map is loaded, so the vertices are rebuilt only when an option that shapes them changes.
    class LimitsPass
    {
    private:
        template <typename T>
        using ComPtr = Viewer::Map::Type::ComPtr<T>;

        using LogsService = Service::Logs::System::LogsService;

        using Vertex = Platform::Render::Type::Vertex;
        using GpuPipeline = Platform::Render::System::GpuPipeline;

        using MapScnrs = Export::Tick::Type::MapScnrs;

        using LimitsPassOptions = Viewer::Map::Type::LimitsPassOptions;

        // Vertices of one kind of primitive and the buffer that holds them.
        struct Batch
        {
            ComPtr<ID3D11Buffer> Buffer{};
            UINT Capacity{ 0 };
            UINT VertexCount{ 0 };
            std::vector<Vertex> Scratch{};
        };

    public:
        explicit LimitsPass(LogsService& logsService) : m_LogsService(logsService) {}
        ~LimitsPass() = default;

        LimitsPass(const LimitsPass&) = delete;
        auto operator=(const LimitsPass&) -> LimitsPass& = delete;

        // Rebuilds the vertices when an option that shapes them changes or when the limits appear.
        // note: The opacity is only remembered, so moving it never rebuilds.
        auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
            const std::shared_ptr<const MapScnrs>& scnrs, const LimitsPassOptions& options) -> void;

        auto Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void;

        auto Release() -> void;

    private:
        LogsService& m_LogsService;

        Batch m_Faces{};
        Batch m_Edges{};

        float m_Opacity{ 0.20f };

        Platform::Render::System::GpuBuffer::UploadGate m_Gate{};

        // return: False if the batch could not be uploaded.
        auto UploadBatch(ID3D11Device* device, ID3D11DeviceContext* context, Batch& batch, const char* tag) -> bool;
    };
}