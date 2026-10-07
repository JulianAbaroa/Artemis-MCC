module;

#include <d3d11.h>

module Viewer.Map.System;
import :DynamicPass;

import Platform.Render.Type;
import Platform.Render.System;
import Viewer.Selection.State;
import Viewer.Style.Type;
import std;

namespace
{
    using MeshInstance = Platform::Render::Type::MeshInstance;
    using Color = Viewer::Style::Type::Color;

    using Platform::Render::Type::SurfaceMode;
    using Platform::Render::Type::VertexLayout;
    using Viewer::Selection::State::k_NoSelection;

    using Viewer::Style::Type::k_Selected;

    constexpr UINT k_InitialGeometryVertices{ 1u << 16 };
    constexpr UINT k_InitialInstances{ 512 };
    constexpr std::uint32_t k_Invalid{ (std::numeric_limits<std::uint32_t>::max)() };
}

namespace Viewer::Map::System
{
    auto DynamicPass::Upload(ID3D11Device* device, ID3D11DeviceContext* context,
        const std::shared_ptr<const Collidables>& collidables,
        const PaletteService& palette, std::uint32_t selectedHandle,
        const std::unordered_set<std::uint32_t>& translucentHandles,
        std::uint64_t generation) -> void
    {
        if (!device || !context) return;

        const std::uint32_t gateKey = selectedHandle ^ (translucentHandles.empty() ? 0u : 0x5A5A5A5Au);

        if (m_Gate.IsCurrent(generation, gateKey)) return;

        m_Gate.Mark(generation, gateKey);

        for (const std::uint32_t index : m_Active) m_Geometries[index].Bucket.clear();
        for (const std::uint32_t index : m_ActiveTranslucent) m_Geometries[index].TranslucentBucket.clear();
        m_Active.clear();
        m_ActiveTranslucent.clear();
        m_Draws.clear();
        m_TranslucentDraws.clear();
        m_InstanceScratch.clear();
        m_InstanceCount = 0;

        if (!collidables || collidables->empty()) return;

        auto resolve = [&](const auto* mesh) -> std::uint32_t
        {
            const auto [it, inserted] = m_GeometryIndex.try_emplace(
                static_cast<const void*>(mesh), k_Invalid);
            if (!inserted) return it->second;

            const auto& triangles = mesh->Triangles;
            if (triangles.empty()) return k_Invalid;

            Geometry geometry{};
            geometry.FirstVertex = m_GeometryTotal;
            geometry.VertexCount = static_cast<UINT>(triangles.size() * 3);

            m_PendingVertices.reserve(m_PendingVertices.size() + triangles.size() * 9);

            auto add = [&](const auto& point)
            {
                m_PendingVertices.push_back(point.X);
                m_PendingVertices.push_back(point.Y);
                m_PendingVertices.push_back(point.Z);
            };

            for (const auto& triangle : triangles)
            {
                add(triangle.A);
                add(triangle.B);
                add(triangle.C);
            }

            m_GeometryTotal += geometry.VertexCount;

            const auto index = static_cast<std::uint32_t>(m_Geometries.size());
            m_Geometries.push_back(std::move(geometry));
            it->second = index;
            return index;
        };

        for (const auto& collidable : *collidables)
        {
            if (collidable.Parts.empty()) continue;

            const bool isSelected = selectedHandle != k_NoSelection &&
                collidable.Handle == selectedHandle;

            const bool isTranslucent = translucentHandles.contains(collidable.Handle);

            const Color color = isSelected ? k_Selected : palette.ColorOf(collidable.Handle);

            for (const auto& part : collidable.Parts)
            {
                if (!part.Source) continue;

                const std::uint32_t index = resolve(part.Source);
                if (index == k_Invalid) continue;

                MeshInstance instance{};
                instance.Transform = part.Transform;
                instance.R = color.R;
                instance.G = color.G;
                instance.B = color.B;

                Geometry& geometry = m_Geometries[index];
                auto& bucket = isTranslucent ? geometry.TranslucentBucket : geometry.Bucket;

                if (bucket.empty()) (isTranslucent ? m_ActiveTranslucent : m_Active).push_back(index);
                bucket.push_back(instance);
            }
        }

        if (m_Active.empty() && m_ActiveTranslucent.empty()) return;

        if (!this->FlushGeometry(device, context)) return;

        auto emit = [&](const std::vector<std::uint32_t>& active, bool translucent,
            std::vector<DrawRange>& draws)
        {
            for (const std::uint32_t index : active)
            {
                const Geometry& geometry = m_Geometries[index];
                const auto& bucket = translucent ? geometry.TranslucentBucket : geometry.Bucket;

                DrawRange range{};
                range.FirstVertex = geometry.FirstVertex;
                range.VertexCount = geometry.VertexCount;
                range.FirstInstance = static_cast<UINT>(m_InstanceScratch.size());
                range.InstanceCount = static_cast<UINT>(bucket.size());

                m_InstanceScratch.insert(m_InstanceScratch.end(), bucket.begin(), bucket.end());
                draws.push_back(range);
            }
        };

        emit(m_Active, false, m_Draws);
        emit(m_ActiveTranslucent, true, m_TranslucentDraws);

        this->UploadInstances(device, context);

        if (m_InstanceCount == 0)
        {
            m_Draws.clear();
            m_TranslucentDraws.clear();
        }
    }

    auto DynamicPass::FlushGeometry(ID3D11Device* device, ID3D11DeviceContext* context) -> bool
    {
        if (m_PendingVertices.empty()) return m_GeometryBuffer != nullptr;

        if (m_GeometryTotal > m_GeometryCapacity || !m_GeometryBuffer)
        {
            UINT capacity = m_GeometryCapacity ? m_GeometryCapacity : k_InitialGeometryVertices;
            while (capacity < m_GeometryTotal) capacity *= 2;

            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = capacity * GpuPipeline::k_UnitVertexStride;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

            ComPtr<ID3D11Buffer> grown{};
            if (FAILED(device->CreateBuffer(&desc, nullptr, grown.GetAddressOf())))
            {
                m_LogsService.Message("[DynamicPass] ERROR: Failed to create the geometry buffer.");
                return false;
            }

            if (m_GeometryBuffer && m_GeometryUploaded > 0)
            {
                D3D11_BOX box{};
                box.right = m_GeometryUploaded * GpuPipeline::k_UnitVertexStride;
                box.bottom = 1;
                box.back = 1;

                context->CopySubresourceRegion(grown.Get(), 0, 0, 0, 0,
                    m_GeometryBuffer.Get(), 0, &box);
            }

            m_GeometryBuffer = std::move(grown);
            m_GeometryCapacity = capacity;
        }

        D3D11_BOX target{};
        target.left = m_GeometryUploaded * GpuPipeline::k_UnitVertexStride;
        target.right = m_GeometryTotal * GpuPipeline::k_UnitVertexStride;
        target.bottom = 1;
        target.back = 1;

        context->UpdateSubresource(m_GeometryBuffer.Get(), 0, &target,
            m_PendingVertices.data(), 0, 0);

        m_GeometryUploaded = m_GeometryTotal;
        m_PendingVertices.clear();
        return true;
    }

    auto DynamicPass::UploadInstances(ID3D11Device* device, ID3D11DeviceContext* context) -> void
    {
        m_InstanceCount = 0;

        const UINT total = static_cast<UINT>(m_InstanceScratch.size());
        if (total == 0) return;

        if (!m_InstanceBuffer.Update(device, context, m_InstanceScratch.data(), total,
            GpuPipeline::k_MeshInstanceStride, k_InitialInstances, "[DynamicPass]", m_LogsService))
        {
            return;
        }

        m_InstanceCount = total;
    }

    auto DynamicPass::GetInstanceCount() const -> UINT
    {
        return m_InstanceCount;
    }

    auto DynamicPass::GetDrawCount() const -> UINT
    {
        return static_cast<UINT>(m_Draws.size());
    }

    auto DynamicPass::Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void
    {
        if (!context || !m_GeometryBuffer || !m_InstanceBuffer.Get() || m_Draws.empty()) return;

        pipeline.Bind(context, VertexLayout::MeshInstanced);

        ID3D11Buffer* buffers[2] = { m_GeometryBuffer.Get(), m_InstanceBuffer.Get() };
        const UINT strides[2] = { GpuPipeline::k_UnitVertexStride, GpuPipeline::k_MeshInstanceStride };
        const UINT offsets[2] = { 0, 0 };

        context->IASetVertexBuffers(0, 2, buffers, strides, offsets);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for (const DrawRange& range : m_Draws)
        {
            context->DrawInstanced(range.VertexCount, range.InstanceCount,
                range.FirstVertex, range.FirstInstance);
        }
    }

    auto DynamicPass::DrawTranslucent(ID3D11DeviceContext* context, GpuPipeline& pipeline, float alpha) -> void
    {
        if (!context || !m_GeometryBuffer || !m_InstanceBuffer.Get() || m_TranslucentDraws.empty()) return;

        pipeline.Bind(context, VertexLayout::MeshInstanced, SurfaceMode::Translucent, alpha);

        ID3D11Buffer* buffers[2] = { m_GeometryBuffer.Get(), m_InstanceBuffer.Get() };
        const UINT strides[2] = { GpuPipeline::k_UnitVertexStride, GpuPipeline::k_MeshInstanceStride };
        const UINT offsets[2] = { 0, 0 };

        context->IASetVertexBuffers(0, 2, buffers, strides, offsets);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for (const DrawRange& range : m_TranslucentDraws)
        {
            context->DrawInstanced(range.VertexCount, range.InstanceCount,
                range.FirstVertex, range.FirstInstance);
        }
    }

    auto DynamicPass::Release() -> void
    {
        m_GeometryBuffer.Reset();
        m_InstanceBuffer.Release();

        m_GeometryCapacity = 0;
        m_GeometryUploaded = 0;
        m_GeometryTotal = 0;
        m_InstanceCount = 0;

        m_Gate.Reset();

        m_GeometryIndex.clear();
        std::vector<Geometry>().swap(m_Geometries);
        std::vector<std::uint32_t>().swap(m_Active);
        std::vector<std::uint32_t>().swap(m_ActiveTranslucent);
        std::vector<float>().swap(m_PendingVertices);
        std::vector<MeshInstance>().swap(m_InstanceScratch);
        std::vector<DrawRange>().swap(m_Draws);
        std::vector<DrawRange>().swap(m_TranslucentDraws);
    }
}