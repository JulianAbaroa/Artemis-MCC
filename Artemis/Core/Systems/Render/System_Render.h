#pragma once

#include <d3d11.h>
#include <atomic>
#include <chrono>

struct DX11Addresses
{
	void* Present;
	void* ResizeBuffers;
};

class State_Render;
class State_Settings;
class System_Logs;

struct System_Render_Dependencies
{
	State_Render& State_Render;
	State_Settings& State_Settings;
	System_Logs& System_Logs;
};

class System_Render
{
public:
	System_Render(System_Render_Dependencies dependencies) : 
		m_Deps(dependencies) {}
	~System_Render() = default;

	DX11Addresses GetVtableAddresses() const;

	void Initialize(IDXGISwapChain* pSwapChain);
	bool IsInitialized();
	void Shutdown();

	void UpdateFramerate();

	void BeginFrame(IDXGISwapChain* pSwapChain);
	void EndFrame();

	void UpdateUIScale();

private:
	System_Render_Dependencies m_Deps;

	void ApplyCustomStyle();

	const int m_PresentVMTIndex = 8;
	const int m_ResizeBuffersVMTIndex = 13;

	std::atomic<int> m_FrameCount{ 0 };
	std::chrono::steady_clock::time_point m_LastFramerateTime;
	std::chrono::steady_clock::time_point m_LastCaptureTime;

	ID3D11Texture2D* m_pStagingTextures[2] = { nullptr, nullptr };
	int m_currentBufferIndex = 0;
};