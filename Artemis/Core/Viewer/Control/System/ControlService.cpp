module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

module Viewer.Control.System;

import Viewer.Camera.Type;
import std;

namespace
{
	using Key = Viewer::Camera::Type::Key;

	auto ToCameraKey(std::uintptr_t virtualKey, Key& outKey) -> bool
	{
		switch (virtualKey)
		{
		case 'W':        outKey = Key::Forward; return true;
		case 'S':        outKey = Key::Backward; return true;
		case 'A':        outKey = Key::Left; return true;
		case 'D':        outKey = Key::Right; return true;
		case 'R':        outKey = Key::Up; return true;
		case 'F':        outKey = Key::Down; return true;
		case VK_SHIFT:   outKey = Key::Fast; return true;
		case VK_CONTROL: outKey = Key::Slow; return true;
		default:         return false;
		}
	}

	auto IsRepeat(std::intptr_t lParam) -> bool
	{
		return (lParam & 0x40000000) != 0;
	}
}

namespace Viewer::Control::System
{
	auto ControlService::HandleMessage(WindowMessage& message) -> bool
	{
		const bool isKeyDown = message.Message == WM_KEYDOWN || message.Message == WM_SYSKEYDOWN;
		const bool isKeyUp = message.Message == WM_KEYUP || message.Message == WM_SYSKEYUP;

		if (message.Message == WM_KILLFOCUS)
		{
			m_CameraStore.ResetKeys();
			return false;
		}

		if (isKeyDown && this->HandleHotkey(message)) return true;

		if (m_SettingsStore.IsMenuVisible() || !m_CameraStore.IsActive()) return false;

		if (isKeyDown)
		{
			switch (message.WParam)
			{
			case VK_RIGHT: m_OverlayStore.NextMode(); return true;
			case VK_LEFT:  m_OverlayStore.PreviousMode(); return true;
			case VK_DOWN:  m_OverlayStore.NextPage(); return true;
			case VK_UP:    m_OverlayStore.PreviousPage(); return true;
			default: break;
			}

			Key key;
			if (ToCameraKey(message.WParam, key)) m_CameraStore.SetKey(key, true);

			return true;
		}

		if (isKeyUp)
		{
			Key key;
			if (ToCameraKey(message.WParam, key)) m_CameraStore.SetKey(key, false);

			return true;
		}

		if (message.Message == WM_LBUTTONDOWN)
		{
			m_SelectionStore.RequestPick();
			return true;
		}

		return false;
	}

	auto ControlService::HandleHotkey(const WindowMessage& message) -> bool
	{
		switch (message.WParam)
		{
		case VK_F4:
		{
			if (!IsRepeat(message.LParam))
			{
				const bool isActive = !m_CameraStore.IsActive();
				m_CameraStore.SetActive(isActive);

				if (!isActive) m_CameraStore.ResetKeys();
			}
			return true;
		}

		case VK_F5:
		{
			if (!IsRepeat(message.LParam))
			{
				m_CameraStore.SetFollowEnabled(!m_CameraStore.IsFollowEnabled());
			}
			return true;
		}

		default:
			return false;
		}
	}

	auto ControlService::HandleMouse(RawMouse& mouse) -> bool
	{
		if (!m_CameraStore.IsActive()) return false;

		m_CameraStore.AccumulateMouseDelta(
			static_cast<float>(mouse.DeltaX), static_cast<float>(mouse.DeltaY));

		return true;
	}
}