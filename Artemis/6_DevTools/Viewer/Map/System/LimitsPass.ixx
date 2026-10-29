module;

#include <d3d11.h>
#include <wrl/client.h>

export module DevTools.Viewer.Map.System:LimitsPass;

import Common.Log.System;
import Engine.Render.Type;
import Engine.Render.System;
import Blam.MapModel.Definitions.Type;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Map.Type;
import std;

export namespace DevTools::Viewer::Map::System
{
    // Draws the kill and safe zone volumes of the scenario as translucent solids with an outline,
    // and the soft ceilings of the structure design as translucent surfaces.
    // The volumes never change while the map is loaded, so the vertices are rebuilt only when an option that shapes them changes.
    class LimitsPass
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        using Log = Common::Log::System::Log;

        using Vertex = Engine::Render::Type::Vertex;
        using GpuPipeline = Engine::Render::System::GpuPipeline;

        using Scnr = Blam::MapModel::Definitions::Type::Scnr::Scnr;
        using Sddt = Blam::MapModel::Definitions::Type::Sddt::Sddt;

        using LimitsPassOptions = DevTools::Viewer::Map::Type::LimitsPassOptions;
        using VertexBatch = DevTools::Viewer::Map::Type::VertexBatch;

    public:
        explicit LimitsPass(Log& log) : m_Log(log) {}
        ~LimitsPass() = default;

        LimitsPass(const LimitsPass&) = delete;
        auto operator=(const LimitsPass&) -> LimitsPass& = delete;

        // Rebuilds the vertices when an option that shapes them changes or when the limits appear.
        // note: The opacity is only remembered, so moving it never rebuilds.
        auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
            const std::shared_ptr<const std::unordered_map<std::string, Scnr>>& scnrs, const std::shared_ptr<const std::unordered_map<std::string, Sddt>>& sddts,
            const LimitsPassOptions& options) -> void;

        auto Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void;

        auto Release() -> void;

    private:
        Log& m_Log;

        VertexBatch m_Faces{};
        VertexBatch m_Edges{};

        float m_Opacity{ 0.20f };

        Engine::Render::System::UploadGate m_Gate{};

        // return: False if the batch could not be uploaded.
        auto UploadBatch(ID3D11Device* device, ID3D11DeviceContext* context, VertexBatch& batch, const char* tag) -> bool;
    };
}