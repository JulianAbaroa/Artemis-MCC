module;

#include <d3d11.h>
#include <wrl/client.h>

module Viewer.Map.System;
import :AimPass;

import Platform.Render.Type;
import Platform.Render.System;
import Common.Math.Type;
import Resolved.World.Type;
import Environment.Aim.Type;
import Viewer.Selection.State;
import Viewer.Style.Type;
import std;

namespace
{
    using SphereInstance = Platform::Render::Type::SphereInstance;
    using AnchorSource = Resolved::World::Type::ModelLink::AnchorSource;
    using SectionAim = Environment::Aim::Type::SectionAim;
    using Color = Viewer::Style::Type::Color;

    using Platform::Render::Type::SurfaceMode;
    using Platform::Render::Type::VertexLayout;
    using Viewer::Selection::State::k_NoSelection;

    using Viewer::Style::Type::k_AimModelTarget;
    using Viewer::Style::Type::k_AimHeadshotTarget;
    using Viewer::Style::Type::k_AimCollRegion;
    using Viewer::Style::Type::k_AimObjectCenter;
    using Viewer::Style::Type::k_TranslucentAlpha;

    constexpr UINT k_InitialInstances{ 256 };

    // Circles of the outline.
    constexpr int k_Segments{ 16 };

    // Segments around the sphere fill.
    constexpr int k_FillSegments{ 12 };

    // Rings of the sphere fill, from pole to pole.
    constexpr int k_FillRings{ 8 };

    constexpr float k_MinRadius{ 0.01f };

    // The object center is imprecise, so it only gets a small marker.
    constexpr float k_CenterMarkerRadius{ 0.05f };

    constexpr float k_Pi{ 3.14159265359f };
    constexpr float k_TwoPi{ 6.28318530718f };

    auto ColorOf(AnchorSource source) -> Color
    {
        switch (source)
        {
        case AnchorSource::ModelTarget:    return k_AimModelTarget;
        case AnchorSource::HeadshotTarget: return k_AimHeadshotTarget;
        case AnchorSource::CollRegion:     return k_AimCollRegion;
        default:                           return k_AimObjectCenter;
        }
    }

    auto RadiusOf(const SectionAim& aim) -> float
    {
        if (aim.AimSource == AnchorSource::ObjectCenter) return k_CenterMarkerRadius;

        return (std::max)(aim.Radius, k_MinRadius);
    }

    // Unit sphere meshes as lists of xyz floats: line segments for the outline and triangles for the fill.
    struct UnitMeshes
    {
        std::vector<float> Outline{};
        std::vector<float> Fill{};

        UnitMeshes()
        {
            auto push = [](std::vector<float>& out, float x, float y, float z)
            {
                out.push_back(x);
                out.push_back(y);
                out.push_back(z);
            };

            for (int plane = 0; plane < 3; ++plane)
            {
                for (int i = 0; i < k_Segments; ++i)
                {
                    for (int end = 0; end < 2; ++end)
                    {
                        const float angle = k_TwoPi * static_cast<float>(i + end) / k_Segments;
                        const float a = std::cos(angle);
                        const float b = std::sin(angle);

                        switch (plane)
                        {
                        case 0:  push(Outline, a, b, 0.0f); break;
                        case 1:  push(Outline, a, 0.0f, b); break;
                        default: push(Outline, 0.0f, a, b); break;
                        }
                    }
                }
            }

            struct P
            {
                float X{};
                float Y{};
                float Z{};
            };

            auto point = [](int ring, int segment)
            {
                const float theta = k_Pi * static_cast<float>(ring) / k_FillRings;
                const float phi = k_TwoPi * static_cast<float>(segment) / k_FillSegments;

                return P{
                    std::sin(theta) * std::cos(phi),
                    std::sin(theta) * std::sin(phi),
                    std::cos(theta) };
            };

            auto pushPoint = [&](const P& p) { push(Fill, p.X, p.Y, p.Z); };

            for (int ring = 0; ring < k_FillRings; ++ring)
            {
                for (int segment = 0; segment < k_FillSegments; ++segment)
                {
                    const P v00 = point(ring, segment);
                    const P v01 = point(ring, segment + 1);
                    const P v10 = point(ring + 1, segment);
                    const P v11 = point(ring + 1, segment + 1);

                    pushPoint(v00); pushPoint(v10); pushPoint(v11);
                    pushPoint(v00); pushPoint(v11); pushPoint(v01);
                }
            }
        }
    };

    auto CreateImmutableBuffer(ID3D11Device* device, const std::vector<float>& data,
        Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer) -> bool
    {
        D3D11_BUFFER_DESC desc{};
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.ByteWidth = static_cast<UINT>(data.size() * sizeof(float));
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = data.data();

        return SUCCEEDED(device->CreateBuffer(&desc, &init, buffer.ReleaseAndGetAddressOf()));
    }
}

namespace Viewer::Map::System
{
    auto AimPass::CreateUnitMeshes(ID3D11Device* device) -> bool
    {
        if (m_UnitFillBuffer && m_UnitWireBuffer) return true;

        const UnitMeshes meshes{};

        if (!CreateImmutableBuffer(device, meshes.Fill, m_UnitFillBuffer) ||
            !CreateImmutableBuffer(device, meshes.Outline, m_UnitWireBuffer))
        {
            m_UnitFillBuffer.Reset();
            m_UnitWireBuffer.Reset();
            m_LogsService.Message("[AimPass] ERROR: Failed to create the unit mesh buffers.");
            return false;
        }

        m_UnitFillCount = static_cast<UINT>(meshes.Fill.size() / 3);
        m_UnitWireCount = static_cast<UINT>(meshes.Outline.size() / 3);
        return true;
    }

    auto AimPass::UploadInstances(ID3D11Device* device, ID3D11DeviceContext* context) -> void
    {
        m_InstanceCount = 0;
        m_FillInstanceCount = 0;

        const UINT fillCount = static_cast<UINT>(m_FillScratch.size());
        m_FillScratch.insert(m_FillScratch.end(), m_CenterScratch.begin(), m_CenterScratch.end());

        const UINT total = static_cast<UINT>(m_FillScratch.size());
        if (total == 0) return;

        if (!m_InstanceBuffer.Update(device, context, m_FillScratch.data(), total,
            GpuPipeline::k_InstanceStride, k_InitialInstances, "[AimPass]", m_LogsService))
        {
            return;
        }

        m_InstanceCount = total;
        m_FillInstanceCount = fillCount;
    }

    auto AimPass::Upload(ID3D11Device* device, ID3D11DeviceContext* context,
        const std::shared_ptr<const Aims>& aims,
        const std::shared_ptr<const Healths>& healths, bool tintByHealth,
        std::uint32_t selectedHandle, std::uint64_t generation) -> void
    {
        if (!device || !context) return;

        if (m_Gate.IsCurrent(generation, selectedHandle) && tintByHealth == m_IsTintByHealth) return;

        m_IsTintByHealth = tintByHealth;
        m_Gate.Mark(generation, selectedHandle);
        m_InstanceCount = 0;
        m_FillInstanceCount = 0;

        if (!this->CreateUnitMeshes(device)) return;

        if (!aims) return;

        m_FillScratch.clear();
        m_CenterScratch.clear();

        auto emit = [&](const Aim& aim)
        {
            const Health* health{ nullptr };
            if (tintByHealth && healths)
            {
                const auto healthIt = healths->find(aim.Handle);
                if (healthIt != healths->end()) health = &healthIt->second;
            }

            for (std::size_t i = 0; i < aim.Sections.size(); ++i)
            {
                const SectionAim& section = aim.Sections[i];
                if (!section.Valid) continue;

                Color c = ColorOf(section.AimSource);
                if (health && section.AimSource != AnchorSource::ObjectCenter && i < health->SectionVitalities.size())
                {
                    c = Viewer::Style::Type::ColorOfVitality(health->SectionVitalities[i]);
                }

                const SphereInstance instance{
                    section.Position.X, section.Position.Y, section.Position.Z,
                    RadiusOf(section), c.R, c.G, c.B };

                if (section.AimSource == AnchorSource::ObjectCenter)
                {
                    m_CenterScratch.push_back(instance);
                }
                else
                {
                    m_FillScratch.push_back(instance);
                }
            }
        };

        if (selectedHandle != k_NoSelection)
        {
            auto it = aims->find(selectedHandle);
            if (it != aims->end()) emit(it->second);
        }
        else
        {
            for (const auto& [handle, aim] : *aims)
            {
                emit(aim);
            }
        }

        this->UploadInstances(device, context);
    }

    auto AimPass::Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void
    {
        if (!context || !m_InstanceBuffer.Get()) return;
        if (m_InstanceCount == 0 || !m_UnitFillBuffer || !m_UnitWireBuffer) return;

        const UINT strides[2] = { GpuPipeline::k_UnitVertexStride, GpuPipeline::k_InstanceStride };
        const UINT offsets[2] = { 0, 0 };

        if (m_FillInstanceCount > 0)
        {
            pipeline.Bind(context, VertexLayout::SphereInstanced, SurfaceMode::Translucent,
                k_TranslucentAlpha);

            ID3D11Buffer* buffers[2] = { m_UnitFillBuffer.Get(), m_InstanceBuffer.Get() };
            context->IASetVertexBuffers(0, 2, buffers, strides, offsets);
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context->DrawInstanced(m_UnitFillCount, m_FillInstanceCount, 0, 0);
        }

        {
            pipeline.Bind(context, VertexLayout::SphereInstanced, SurfaceMode::Overlay);

            ID3D11Buffer* buffers[2] = { m_UnitWireBuffer.Get(), m_InstanceBuffer.Get() };
            context->IASetVertexBuffers(0, 2, buffers, strides, offsets);
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
            context->DrawInstanced(m_UnitWireCount, m_InstanceCount, 0, 0);
        }
    }

    auto AimPass::GetInstanceCount() const -> UINT
    {
        return m_InstanceCount;
    }

    auto AimPass::Release() -> void
    {
        m_UnitFillBuffer.Reset();
        m_UnitWireBuffer.Reset();
        m_InstanceBuffer.Release();

        m_UnitFillCount = 0;
        m_UnitWireCount = 0;
        m_InstanceCount = 0;
        m_FillInstanceCount = 0;

        m_Gate.Reset();

        std::vector<SphereInstance>().swap(m_FillScratch);
        std::vector<SphereInstance>().swap(m_CenterScratch);
    }
}