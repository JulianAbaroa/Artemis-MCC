module;

#include <d3d11.h>

module Viewer.Map.System;
import :MapService;

import Viewer.Camera.Type;
import std;

namespace
{
	using Viewport = Viewer::Camera::Type::Viewport;

	constexpr float k_Background[4] = { 0.05f, 0.06f, 0.08f, 1.0f };
}

namespace Viewer::Map::System
{
	auto MapService::Render(const FrameContext& frame) -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		if (m_IsSuspended) return;

		if (!frame.Device || !frame.Context ||
			!frame.RenderTarget || !frame.DepthStencil) return;

		if (frame.Device != m_Device)
		{
			this->ReleaseAll();
			m_Device = frame.Device;
		}

		if (m_IsMapResetPending)
		{
			this->ReleaseMap();
			m_IsMapResetPending = false;
		}

		if (!m_CameraStore.IsActive())
		{
			m_CameraService.Deactivate();
			return;
		}

		const auto tick = m_TickStore.Acquire();
		if (!tick) return;

		if (!m_GpuPipeline.Initialize(frame.Device)) return;

		if (m_WorldStore.IsFrozen() && !m_MapPass.IsUploaded())
		{
			m_MapPass.Upload(frame.Device, m_WorldStore.GetAllResolvedSbsps());
		}

		if (!m_MapPass.HasBuffer()) return;

		Viewport viewport;
		viewport.Size.X = static_cast<float>(frame.Width);
		viewport.Size.Y = static_cast<float>(frame.Height);

		const bool acceptInput = !m_SettingsStore.IsMenuVisible();
		m_CameraService.Update(tick, viewport, acceptInput);

		m_PaletteService.Build(tick);

		m_SelectionService.Pick(m_CameraService.GetCenterRay(), m_DynamicPass.GetBounds());

		m_DynamicPass.Upload(frame.Device, frame.Context, tick->Collidables,
			m_PaletteService, m_SelectionStore.GetSelected(), tick->Generation);

		m_ZonePass.Upload(frame.Device, frame.Context, tick, m_PaletteService);

		this->Draw(frame);
	}

	auto MapService::Draw(const FrameContext& frame) -> void
	{
		ID3D11DeviceContext* context = frame.Context;

		GpuStateGuard guard(context);

		context->ClearRenderTargetView(frame.RenderTarget, k_Background);
		context->ClearDepthStencilView(frame.DepthStencil,
			D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		D3D11_VIEWPORT viewport = {};
		viewport.Width = static_cast<float>(frame.Width);
		viewport.Height = static_cast<float>(frame.Height);
		viewport.MaxDepth = 1.0f;

		ID3D11RenderTargetView* renderTarget = frame.RenderTarget;
		context->OMSetRenderTargets(1, &renderTarget, frame.DepthStencil);
		context->RSSetViewports(1, &viewport);

		m_GpuPipeline.UpdateCamera(context, m_CameraService.GetViewProjection());
		m_GpuPipeline.Bind(context);

		m_MapPass.Draw(context);
		m_DynamicPass.Draw(context);
		m_ZonePass.Draw(context);
	}

	auto MapService::Suspend() -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		m_IsSuspended = true;
		m_IsMapResetPending = true;

		m_SelectionStore.Clear();
	}

	auto MapService::Resume() -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		m_IsSuspended = false;
	}

	auto MapService::Release() -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		this->ReleaseAll();
		m_Device = nullptr;
	}

	auto MapService::ReleaseMap() -> void
	{
		m_MapPass.Release();
		m_DynamicPass.Release();
		m_ZonePass.Release();
		m_PaletteService.Reset();
	}

	auto MapService::ReleaseAll() -> void
	{
		this->ReleaseMap();
		m_GpuPipeline.Release();
	}
}