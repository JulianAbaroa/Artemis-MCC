module;

#include <d3d11.h>
#include <wrl/client.h>

module Viewer.Map.System;
import :AimPass;

import Common.Math.Type;
import Platform.Render.System;
import Resolved.World.Type;
import Viewer.Render.Common;
import Viewer.Selection.State;
import std;

namespace
{
	using Vertex = Platform::Render::System::GpuPipeline::Vertex;
	using Vec3 = Common::Math::Type::Vec3;
	using AimSource = Resolved::World::Type::ModelLink::AnchorSource;
	using SectionAim = Environment::Aim::Type::SectionAim;

	using Viewer::Selection::State::k_NoSelection;

	constexpr UINT k_InitialCapacity = 2048;

	constexpr int k_Segments = 16;      // outline circles
	constexpr int k_FillSegments = 12;  // sphere fill, around
	constexpr int k_FillRings = 8;      // sphere fill, pole to pole

	constexpr float k_MinRadius = 0.01f;
	constexpr float k_CenterMarkerRadius = 0.05f; // ObjectCenter is imprecise: small marker only
	constexpr float k_FillAlpha = 0.35f;
	constexpr float k_Pi = 3.14159265359f;
	constexpr float k_TwoPi = 6.28318530718f;

	struct Color { float R{}, G{}, B{}; };

	constexpr Color k_ModelTarget{ 0.3f, 1.0f, 0.3f };     // green:  marker of the hlmt target
	constexpr Color k_HeadshotTarget{ 1.0f, 0.85f, 0.2f }; // yellow: headshot lock-on marker
	constexpr Color k_CollRegion{ 0.2f, 0.6f, 1.0f };      // blue:   bounds of the coll region
	constexpr Color k_ObjectCenter{ 0.6f, 0.6f, 0.6f };    // gray:   center of the model (imprecise)

	auto ColorOf(AimSource source) -> Color
	{
		switch (source)
		{
		case AimSource::ModelTarget:    return k_ModelTarget;
		case AimSource::HeadshotTarget: return k_HeadshotTarget;
		case AimSource::CollRegion:     return k_CollRegion;
		default:                        return k_ObjectCenter;
		}
	}

	auto RadiusOf(const SectionAim& aim) -> float
	{
		if (aim.Source == AimSource::ObjectCenter) return k_CenterMarkerRadius;

		return (std::max)(aim.Radius, k_MinRadius);
	}

	auto AppendOutline(std::vector<Vertex>& vertices, const SectionAim& aim) -> void
	{
		const Color c = ColorOf(aim.Source);
		const float r = RadiusOf(aim);
		const Vec3& p = aim.Position;

		auto point = [&](int plane, float angle) {
			const float a = std::cos(angle) * r;
			const float b = std::sin(angle) * r;

			switch (plane)
			{
			case 0:  return Vertex{ p.X + a, p.Y + b, p.Z, c.R, c.G, c.B };
			case 1:  return Vertex{ p.X + a, p.Y, p.Z + b, c.R, c.G, c.B };
			default: return Vertex{ p.X, p.Y + a, p.Z + b, c.R, c.G, c.B };
			}
			};

		for (int plane = 0; plane < 3; ++plane)
		{
			for (int i = 0; i < k_Segments; ++i)
			{
				const float a0 = k_TwoPi * static_cast<float>(i) / k_Segments;
				const float a1 = k_TwoPi * static_cast<float>(i + 1) / k_Segments;

				vertices.push_back(point(plane, a0));
				vertices.push_back(point(plane, a1));
			}
		}
	}

	// UV sphere as a triangle list.
	auto AppendFill(std::vector<Vertex>& vertices, const SectionAim& aim) -> void
	{
		const Color c = ColorOf(aim.Source);
		const float r = RadiusOf(aim);
		const Vec3& p = aim.Position;

		auto point = [&](int ring, int segment) {
			const float theta = k_Pi * static_cast<float>(ring) / k_FillRings;
			const float phi = k_TwoPi * static_cast<float>(segment) / k_FillSegments;

			return Vertex{
				p.X + r * std::sin(theta) * std::cos(phi),
				p.Y + r * std::sin(theta) * std::sin(phi),
				p.Z + r * std::cos(theta),
				c.R, c.G, c.B };
			};

		for (int ring = 0; ring < k_FillRings; ++ring)
		{
			for (int segment = 0; segment < k_FillSegments; ++segment)
			{
				const Vertex v00 = point(ring, segment);
				const Vertex v01 = point(ring, segment + 1);
				const Vertex v10 = point(ring + 1, segment);
				const Vertex v11 = point(ring + 1, segment + 1);

				vertices.push_back(v00);
				vertices.push_back(v10);
				vertices.push_back(v11);

				vertices.push_back(v00);
				vertices.push_back(v11);
				vertices.push_back(v01);
			}
		}
	}

	auto UploadBuffer(ID3D11Device* device, ID3D11DeviceContext* context,
		const std::vector<Vertex>& vertices, Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer,
		UINT& capacity, UINT& count, const char* tag,
		Service::Logs::System::LogsService& logs) -> void
	{
		count = 0;
		if (vertices.empty()) return;

		const UINT needed = static_cast<UINT>(vertices.size());

		if (!Viewer::Render::Common::GrowDynamicVertexBuffer(device, needed,
			k_InitialCapacity, buffer, capacity, tag, logs))
		{
			return;
		}

		if (!Viewer::Render::Common::UploadDynamicVertices(context, buffer.Get(),
			vertices, tag, logs))
		{
			return;
		}

		count = needed;
	}
}

namespace Viewer::Map::System
{
	auto AimPass::Upload(ID3D11Device* device, ID3D11DeviceContext* context,
		const std::shared_ptr<const Aims>& aims, std::uint32_t selectedHandle,
		std::uint64_t generation) -> void
	{
		if (!device || !context) return;

		if (m_HasUpload && generation == m_LastGeneration &&
			selectedHandle == m_LastSelected)
		{
			return;
		}

		m_LastGeneration = generation;
		m_LastSelected = selectedHandle;
		m_HasUpload = true;
		m_FillCount = 0;
		m_WireCount = 0;

		if (!m_NoDepthState)
		{
			D3D11_DEPTH_STENCIL_DESC depth = {};
			depth.DepthEnable = FALSE;
			depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			depth.DepthFunc = D3D11_COMPARISON_ALWAYS;
			depth.StencilEnable = FALSE;

			if (FAILED(device->CreateDepthStencilState(&depth, m_NoDepthState.GetAddressOf())))
			{
				m_LogsService.Message("[AimPass] ERROR: Depth state failed.");
				return;
			}
		}

		if (!m_FillBlendState)
		{
			D3D11_BLEND_DESC blend = {};
			auto& target = blend.RenderTarget[0];
			target.BlendEnable = TRUE;
			target.SrcBlend = D3D11_BLEND_BLEND_FACTOR;
			target.DestBlend = D3D11_BLEND_INV_BLEND_FACTOR;
			target.BlendOp = D3D11_BLEND_OP_ADD;
			target.SrcBlendAlpha = D3D11_BLEND_ONE;
			target.DestBlendAlpha = D3D11_BLEND_ZERO;
			target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			target.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			if (FAILED(device->CreateBlendState(&blend, m_FillBlendState.GetAddressOf())))
			{
				m_LogsService.Message("[AimPass] ERROR: Blend state failed.");
				return;
			}
		}

		if (!aims) return;

		std::vector<Vertex> fill;
		std::vector<Vertex> wire;

		auto emit = [&](const Environment::Aim::Type::Aim& aim) {
			for (const SectionAim& section : aim.Sections)
			{
				if (!section.Valid) continue;

				if (section.Source != AimSource::ObjectCenter)
				{
					AppendFill(fill, section);
				}
				AppendOutline(wire, section);
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

		UploadBuffer(device, context, fill, m_FillBuffer, m_FillCapacity,
			m_FillCount, "[AimPass]", m_LogsService);
		UploadBuffer(device, context, wire, m_WireBuffer, m_WireCapacity,
			m_WireCount, "[AimPass]", m_LogsService);
	}

	auto AimPass::Draw(ID3D11DeviceContext* context) -> void
	{
		if (!context || !m_NoDepthState || !m_FillBlendState) return;
		if (m_FillCount == 0 && m_WireCount == 0) return;

		context->OMSetDepthStencilState(m_NoDepthState.Get(), 0);

		if (m_FillBuffer && m_FillCount > 0)
		{
			const float factor[4] = { k_FillAlpha, k_FillAlpha, k_FillAlpha, k_FillAlpha };
			context->OMSetBlendState(m_FillBlendState.Get(), factor, 0xffffffff);

			Viewer::Render::Common::DrawVertexBuffer(context, m_FillBuffer.Get(),
				m_FillCount, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}

		if (m_WireBuffer && m_WireCount > 0)
		{
			const float factor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			context->OMSetBlendState(nullptr, factor, 0xffffffff);

			Viewer::Render::Common::DrawVertexBuffer(context, m_WireBuffer.Get(),
				m_WireCount, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		}
	}

	auto AimPass::Release() -> void
	{
		m_FillBuffer.Reset();
		m_WireBuffer.Reset();
		m_NoDepthState.Reset();
		m_FillBlendState.Reset();

		m_FillCapacity = 0;
		m_WireCapacity = 0;
		m_FillCount = 0;
		m_WireCount = 0;

		m_LastGeneration = 0;
		m_LastSelected = 0;
		m_HasUpload = false;
	}
}