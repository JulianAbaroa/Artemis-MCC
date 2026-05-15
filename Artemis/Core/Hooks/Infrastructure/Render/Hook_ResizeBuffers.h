#pragma once

#include <d3d11.h>
#include <atomic>

class State_Render;
class System_Render;
class System_Debug;

struct Hook_ResizeBuffers_Dependencies
{
	State_Render& State_Render;
	System_Render& System_Render;
	System_Debug& System_Debug;
};

class Hook_ResizeBuffers
{
public:
	Hook_ResizeBuffers(Hook_ResizeBuffers_Dependencies dependencies) :
		m_Deps(dependencies) {}
	~Hook_ResizeBuffers() = default;

	void Install();
	void Uninstall();

private:
	static Hook_ResizeBuffers* s_Instance;
	Hook_ResizeBuffers_Dependencies m_Deps;

	static HRESULT __stdcall HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount,
		UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

	typedef HRESULT(__stdcall* ResizeBuffers_t)(IDXGISwapChain* pSwapChain, UINT BufferCount,
		UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

	static inline ResizeBuffers_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};