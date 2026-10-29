module;

#include <d3d11.h>
#include <wrl/client.h>

export module DevTools.Viewer.Map.System:AimPass;

import Common.Log.System;
import Engine.Render.Type;
import Engine.Render.System;
import Perception.Aim.Type;
import Perception.Details.Type;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Map.Type;
import std;

export namespace DevTools::Viewer::Map::System
{
    // Draws a sphere at each aim section of the objects: a translucent fill and a wire outline.
    // The color comes from the anchor source of the section, or from its vitality if tinting by health is on.
    class AimPass
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        using Log = Common::Log::System::Log;

        using SphereInstance = Engine::Render::Type::SphereInstance;
        using GpuPipeline = Engine::Render::System::GpuPipeline;

        using VitalityDetail = Perception::Details::Type::VitalityDetail;
        using VitalityDetails = Perception::Details::Type::VitalityDetails;
        using Aim = Perception::Aim::Type::Aim;
        using Aims = Perception::Aim::Type::Aims;

    public:
        explicit AimPass(Log& log) : m_Log(log) {}
        ~AimPass() = default;

        AimPass(const AimPass&) = delete;
        auto operator=(const AimPass&) -> AimPass& = delete;

        // Rebuilds the spheres when the generation, the selection or the tint option change.
        // param selectedHandle: If set, only the aim of that object is drawn.
        // param tintByHealth: Colors each sphere by the vitality of its section.
        auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
            const std::shared_ptr<const Aims>& aims,
            const std::shared_ptr<const VitalityDetails>& vitalities, bool tintByHealth,
            std::uint32_t selectedHandle, std::uint64_t generation) -> void;

        auto Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void;

        auto GetInstanceCount() const -> UINT;

        auto Release() -> void;

    private:
        Log& m_Log;

        ComPtr<ID3D11Buffer> m_UnitFillBuffer{};
        ComPtr<ID3D11Buffer> m_UnitWireBuffer{};
        UINT m_UnitFillCount{ 0 };
        UINT m_UnitWireCount{ 0 };

        Engine::Render::System::InstanceBuffer m_InstanceBuffer{};
        UINT m_InstanceCount{ 0 };
        UINT m_FillInstanceCount{ 0 };
        bool m_IsTintByHealth{ false };

        Engine::Render::System::UploadGate m_Gate{};

        std::vector<SphereInstance> m_FillScratch{};
        std::vector<SphereInstance> m_CenterScratch{};

        // Creates the unit sphere buffers once.
        // return: False if a buffer could not be created.
        auto CreateUnitMeshes(ID3D11Device* device) -> bool;

        // Uploads the fill spheres followed by the object-center markers. The markers draw only the outline.
        auto UploadInstances(ID3D11Device* device, ID3D11DeviceContext* context) -> void;
    };
}