#pragma once

#include <d3d11.h>
#include <atomic>

class RenderState
{
public:
	ID3D11Device* GetDevice() const;
	void SetDevice(ID3D11Device* device);

	ID3D11DeviceContext* GetContext() const;
	void SetContext(ID3D11DeviceContext* context);

	HWND GetHWND() const;
	void SetHWND(HWND hwnd);

	ID3D11RenderTargetView* GetRTV() const;
	void SetRTV(ID3D11RenderTargetView* rtv);

	bool IsImGuiInitialized() const;
	void SetImGuiInitialized(bool initialized);

	bool IsResizing() const;
	void SetResizing(bool state);

	int GetWidth() const;
	void SetWidth(int width);

	int GetHeight() const;
	void SetHeight(int height);

	int GetFramerate() const;
	void SetFramerate(int framerate);

	float GetUIScale() const;
	void SetUIScale(float scale);

	bool ShouldRebuildFonts() const;
	void ResetFontRebuild();

	void CleanupRTV();
	void FullCleanup();

private:
	ID3D11Device* m_pDevice{ nullptr };
	ID3D11DeviceContext* m_pContext{ nullptr };
	HWND m_GameHWND{ nullptr };
	ID3D11RenderTargetView* m_pMainRenderTargetView{ nullptr };

	std::atomic<bool> m_IsImGuiInitialized{ false };
	std::atomic<bool> m_IsResizing{ false };
	std::atomic<int> m_Width{ 0 };
	std::atomic<int> m_Height{ 0 };
	std::atomic<int> m_Framerate{ 0 };
	std::atomic<float> m_UIScale{ 1.0f };
	std::atomic<bool> m_NeedFontRebuild{ false };
};