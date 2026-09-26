module;

#include <windows.h>
#include "External/minhook/include/MinHook.h"

module Platform.Input.Hook;
import :GetRawInputData;

import Platform.Input.Type;
import Platform.Hook.Common;
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
}

namespace Platform::Input::Hook
{
	GetRawInputDataDetour* GetRawInputDataDetour::s_Instance = nullptr;

	auto WINAPI GetRawInputDataDetour::HookedGetRawInputData(HRAWINPUT hRawInput,
		UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader) -> UINT
	{
		auto* self = s_Instance;

		if (self && uiCommand == RID_INPUT && 
			self->m_MouseDeltaStore.IsAIControlActive())
		{
			std::int32_t injectX = 0;
			std::int32_t injectY = 0;

			if (self->m_MouseDeltaStore.ConsumePendingDelta(injectX, injectY))
			{
				Platform::Hook::Common::InFlightScope scope(s_InFlight);

				const UINT requiredSize = sizeof(RAWINPUT);

				if (!pData)
				{
					if (pcbSize) *pcbSize = requiredSize;
					return 0;
				}

				if (!pcbSize || *pcbSize < requiredSize)
				{
					return static_cast<UINT>(-1);
				}

				auto* raw = static_cast<RAWINPUT*>(pData);
				raw->header.dwType = RIM_TYPEMOUSE;
				raw->header.dwSize = requiredSize;	
				raw->header.hDevice = self->m_MouseDeltaStore.GetKnownDeviceHandle();
				raw->header.wParam = RIM_INPUT;

				raw->data.mouse = {};
				raw->data.mouse.usFlags = MOUSE_MOVE_RELATIVE;
				raw->data.mouse.lLastX = injectX;
				raw->data.mouse.lLastY = injectY;

				return requiredSize;
			}
		}

		const UINT size = m_OriginalFunction(
			hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

		if (!self) return size;

		Platform::Hook::Common::InFlightScope scope(s_InFlight);

		if (size == static_cast<UINT>(-1) || !pData || uiCommand != RID_INPUT) return size;

		auto* raw = static_cast<RAWINPUT*>(pData);
		if (raw->header.dwType != RIM_TYPEMOUSE) return size;

		RAWMOUSE& mouse = raw->data.mouse;

		self->m_MouseDeltaStore.SetKnownDeviceHandle(raw->header.hDevice);

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
		m_FunctionAddress.store(functionAddress);

		if (!Platform::Hook::Common::InstallDetour(functionAddress,
			reinterpret_cast<void*>(&HookedGetRawInputData),
			reinterpret_cast<void**>(&m_OriginalFunction),
			"[GetRawInputDataDetour]", m_LogsService))
		{
			return;
		}

		m_IsHookInstalled.store(true);
	}

	void GetRawInputDataDetour::Uninstall()
	{
		if (!m_IsHookInstalled.load()) return;

		Platform::Hook::Common::DisableDetour(m_FunctionAddress.load());
		Platform::Hook::Common::WaitForDrain(s_InFlight, std::chrono::milliseconds(500));
		Platform::Hook::Common::RemoveDetour(m_FunctionAddress.load(),
			"[GetRawInputDataDetour]", m_LogsService);

		m_IsHookInstalled.store(false);
		s_Instance = nullptr;
	}
}