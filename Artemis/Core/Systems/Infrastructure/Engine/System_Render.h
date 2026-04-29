#pragma once

#include "Core/Types/Infrastructure/DX11Types.h"
#include <d3d11.h>
#include <atomic>
#include <chrono>

class System_Render
{
public:
	DX11Addresses GetVtableAddresses();

	void Initialize(IDXGISwapChain* pSwapChain);
	bool IsInitialized();
	void Shutdown();

	void UpdateFramerate();

	void BeginFrame(IDXGISwapChain* pSwapChain);
	void EndFrame();

	void UpdateUIScale();

private:
	void ApplyCustomStyle();

	const int m_PresentVMTIndex = 8;
	const int m_ResizeBuffersVMTIndex = 13;

	std::atomic<int> m_FrameCount{ 0 };
	std::chrono::steady_clock::time_point m_LastFramerateTime;
	std::chrono::steady_clock::time_point m_LastCaptureTime;

	ID3D11Texture2D* m_pStagingTextures[2] = { nullptr, nullptr };
	int m_currentBufferIndex = 0;
};