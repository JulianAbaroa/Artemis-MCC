module;

#include <windows.h>
#include "External/minhook/include/MinHook.h"

module Platform.Input.Hook;
import :GetRawInputData;

import Platform.Input.Type;
import std;

namespace
{
	using RawMouse = Platform::Input::Type::RawMouse;

	inline auto SwallowMouse(RAWMOUSE& mouse) -> void
	{
		mouse.lLastX = 0;
		mouse.lLastY = 0;
		mouse.ulButtons = 0;
	}

	struct InFlightScope
	{
		explicit InFlightScope(std::atomic<int>& counter) : m_Counter(counter)
		{
			m_Counter.fetch_add(1, std::memory_order_acq_rel);
		}
		~InFlightScope()
		{
			m_Counter.fetch_sub(1, std::memory_order_acq_rel);
		}

		InFlightScope(const InFlightScope&) = delete;
		InFlightScope& operator=(const InFlightScope&) = delete;

		std::atomic<int>& m_Counter;
	};
}

namespace Platform::Input::Hook
{
	GetRawInputDataDetour* GetRawInputDataDetour::s_Instance = nullptr;

	auto WINAPI GetRawInputDataDetour::HookedGetRawInputData(HRAWINPUT hRawInput,
		UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader) -> UINT
	{
		const UINT size = m_OriginalFunction(
			hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

		auto* self = s_Instance;
		if (!self) return size;

		InFlightScope scope(s_InFlight);

		if (size == static_cast<UINT>(-1) || !pData || uiCommand != RID_INPUT) return size;

		auto* raw = static_cast<RAWINPUT*>(pData);
		if (raw->header.dwType != RIM_TYPEMOUSE) return size;

		RAWMOUSE& mouse = raw->data.mouse;

		if (self->m_SettingsStore.IsMenuVisible())
		{
			if (self->m_SettingsStore.ShouldFreezeMouse()) SwallowMouse(mouse);
			return size;
		}

		if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE) return size;

		RawMouse input{};
		input.DeltaX = mouse.lLastX;
		input.DeltaY = mouse.lLastY;
		input.ButtonFlags = mouse.usButtonFlags;
		input.ButtonData = mouse.usButtonData;

		if (self->m_InputService.RaiseRawMouse(input)) SwallowMouse(mouse);

		return size;
	}

	void GetRawInputDataDetour::Install()
	{
		if (m_IsHookInstalled.load()) return;
		s_Instance = this;

		HMODULE user32 = GetModuleHandleW(L"user32.dll");
		if (!user32)
		{
			m_LogsService.Message("[GetRawInputDataDetour] ERROR:"
				" Could not get the handle of user32.dll.");
			return;
		}

		void* functionAddress = reinterpret_cast<void*>(
			GetProcAddress(user32, "GetRawInputData"));

		if (!functionAddress)
		{
			m_LogsService.Message("[GetRawInputDataDetour] ERROR:"
				" Failed to obtain the function address.");
			return;
		}

		m_FunctionAddress.store(functionAddress);
		MH_RemoveHook(m_FunctionAddress.load());

		if (MH_CreateHook(m_FunctionAddress.load(), reinterpret_cast<LPVOID>(&HookedGetRawInputData),
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
		{
			m_LogsService.Message("[GetRawInputDataDetour] ERROR:"
				" Failed to create the hook.");
			return;
		}

		if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
		{
			m_LogsService.Message("[GetRawInputDataDetour] ERROR:"
				" Failed to enable the hook.");

			MH_RemoveHook(m_FunctionAddress.load());
			return;
		}

		m_IsHookInstalled.store(true);
		m_LogsService.Message("[GetRawInputDataDetour] INFO: Hook installed.");
	}

	void GetRawInputDataDetour::Uninstall()
	{
		if (!m_IsHookInstalled.load()) return;

		MH_DisableHook(m_FunctionAddress.load());

		const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
		while (s_InFlight.load(std::memory_order_acquire) > 0 &&
			std::chrono::steady_clock::now() < deadline)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}

		MH_RemoveHook(m_FunctionAddress.load());

		m_IsHookInstalled.store(false);
		m_LogsService.Message("[GetRawInputDataDetour] INFO: Hook uninstalled.");

		s_Instance = nullptr;
	}
}