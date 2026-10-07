module;

#include <d3d11.h>

export module Viewer.Map.System:AimPass;

import Service.Logs.System;
import Platform.Render.Type;
import Platform.Render.System;
import Export.Tick.Type;
import Viewer.Map.Type;
import std;

export namespace Viewer::Map::System
{
    // Draws a sphere at each aim section of the objects: a translucent fill and a wire outline.
    // The color comes from the anchor source of the section, or from its vitality if tinting by health is on.
    class AimPass
    {
    private:
        template <typename T>
        using ComPtr = Viewer::Map::Type::ComPtr<T>;

        using LogsService = Service::Logs::System::LogsService;

        using SphereInstance = Platform::Render::Type::SphereInstance;
        using GpuPipeline = Platform::Render::System::GpuPipeline;

        using Health = Export::Tick::Type::Health;
        using Healths = Export::Tick::Type::Healths;
        using Aim = Export::Tick::Type::Aim;
        using Aims = Export::Tick::Type::Aims;

    public:
        explicit AimPass(LogsService& logsService) : m_LogsService(logsService) {}
        ~AimPass() = default;

        AimPass(const AimPass&) = delete;
        auto operator=(const AimPass&) -> AimPass& = delete;

        // Rebuilds the spheres when the generation, the selection or the tint option change.
        // param selectedHandle: If set, only the aim of that object is drawn.
        // param tintByHealth: Colors each sphere by the vitality of its section.
        auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
            const std::shared_ptr<const Aims>& aims,
            const std::shared_ptr<const Healths>& healths, bool tintByHealth,
            std::uint32_t selectedHandle, std::uint64_t generation) -> void;

        auto Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void;

        auto GetInstanceCount() const -> UINT;

        auto Release() -> void;

    private:
        LogsService& m_LogsService;

        ComPtr<ID3D11Buffer> m_UnitFillBuffer{};
        ComPtr<ID3D11Buffer> m_UnitWireBuffer{};
        UINT m_UnitFillCount{ 0 };
        UINT m_UnitWireCount{ 0 };

        Platform::Render::System::GpuBuffer::InstanceBuffer m_InstanceBuffer{};
        UINT m_InstanceCount{ 0 };
        UINT m_FillInstanceCount{ 0 };
        bool m_IsTintByHealth{ false };

        Platform::Render::System::GpuBuffer::UploadGate m_Gate{};

        std::vector<SphereInstance> m_FillScratch{};
        std::vector<SphereInstance> m_CenterScratch{};

        // Creates the unit sphere buffers once.
        // return: False if a buffer could not be created.
        auto CreateUnitMeshes(ID3D11Device* device) -> bool;

        // Uploads the fill spheres followed by the object-center markers. The markers draw only the outline.
        auto UploadInstances(ID3D11Device* device, ID3D11DeviceContext* context) -> void;
    };
}