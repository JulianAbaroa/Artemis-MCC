#include "pch.h"
#include "Core/States/Infrastructure/Engine/State_Render.h"

ID3D11Device* State_Render::GetDevice() const { return m_pDevice; }
void State_Render::SetDevice(ID3D11Device* device) { m_pDevice = device; }

ID3D11DeviceContext* State_Render::GetContext() const { return m_pContext; }
void State_Render::SetContext(ID3D11DeviceContext* context) { m_pContext = context; }

HWND State_Render::GetHWND() const { return m_GameHWND; }
void State_Render::SetHWND(HWND hwnd) { m_GameHWND = hwnd; }

ID3D11RenderTargetView* State_Render::GetRTV() const { return m_pMainRenderTargetView; }
void State_Render::SetRTV(ID3D11RenderTargetView* rtv) { m_pMainRenderTargetView = rtv; }

bool State_Render::IsImGuiInitialized() const { return m_IsImGuiInitialized.load(); }
void State_Render::SetImGuiInitialized(bool initialized) { m_IsImGuiInitialized.store(initialized); }

bool State_Render::IsResizing() const { return m_IsResizing.load(); }
void State_Render::SetResizing(bool state) { m_IsResizing.store(state); }

int State_Render::GetWidth() const { return m_Width.load(); }
void State_Render::SetWidth(int width) { m_Width.store(width); }

int State_Render::GetHeight() const { return m_Height.load(); }
void State_Render::SetHeight(int height) { m_Height.store(height); }

int State_Render::GetFramerate() const { return m_Framerate.load(); }
void State_Render::SetFramerate(int framerate) { m_Framerate.store(framerate); }

float State_Render::GetUIScale() const { return m_UIScale.load(); }
void State_Render::SetUIScale(float scale) { m_UIScale.store(scale); m_NeedFontRebuild.store(true); }

bool State_Render::ShouldRebuildFonts() const { return m_NeedFontRebuild.load(); }
void State_Render::ResetFontRebuild() { m_NeedFontRebuild.store(false); }

void State_Render::CleanupRTV()
{
	if (m_pMainRenderTargetView)
	{
		m_pMainRenderTargetView->Release();
		m_pMainRenderTargetView = nullptr;
	}
}

void State_Render::FullCleanup()
{
	CleanupRTV();

	m_pDevice = nullptr;
	m_pContext = nullptr;
	m_GameHWND = nullptr;
}