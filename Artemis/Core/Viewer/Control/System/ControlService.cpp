module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

module Viewer.Control.System;

import Viewer.Camera.Type;
import Viewer.Options.Type;
import std;

namespace
{
    using Key = Viewer::Camera::Type::Key;
    using Mode = Viewer::Camera::Type::Mode;
    using Follow = Viewer::Camera::Type::Follow;
    using Scalar = Viewer::Options::Type::Scalar;

    // Maps a virtual key to a camera key.
    // return: False if the key is not a camera key.
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
        case VK_CONTROL: outKey = Key::Custom; return true;
        default:         return false;
        }
    }

    auto IsRepeat(std::intptr_t lParam) -> bool
    {
        return (lParam & 0x40000000) != 0;
    }

    // One notch of the mouse wheel is 120 units.
    constexpr float k_WheelNotch{ 120.0f };

    // Factor applied to the custom speed for each notch. The inverse is applied to the field of view.
    constexpr float k_SpeedStep{ 1.15f };
    constexpr float k_FovStep{ 1.08f };

    // Factor applied to the orbit distance for each notch.
    constexpr float k_OrbitStep{ 1.15f };
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

        // In Game mode the game keeps every key.
        if (m_CameraStore.GetMode() == Mode::Game) return false;

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

            Key key{};
            if (ToCameraKey(message.WParam, key)) m_CameraStore.SetKeyDown(key, true);

            return true;
        }

        if (isKeyUp)
        {
            Key key{};
            if (ToCameraKey(message.WParam, key)) m_CameraStore.SetKeyDown(key, false);

            return true;
        }

        if (message.Message == WM_LBUTTONDOWN)
        {
            // While following, the crosshair is on the followed object, so a pick would select it again.
            // The click releases it instead. The follow mode stays armed and the next click picks and follows a new one.
            if (m_CameraStore.GetFollow() != Follow::Off && m_SelectionStore.HasSelection())
            {
                m_SelectionStore.Clear();
            }
            else
            {
                m_SelectionStore.RequestPick();
            }

            return true;
        }

        // Like the left click, only the press is consumed. The release always goes on, so the Gui and the game never miss it.
        if (message.Message == WM_RBUTTONDOWN)
        {
            m_CameraStore.ToggleZoom();
            return true;
        }

        if (message.Message == WM_MOUSEWHEEL)
        {
            this->HandleWheel(static_cast<int>(GET_WHEEL_DELTA_WPARAM(message.WParam)));
            return true;
        }

        return false;
    }

    auto ControlService::HandleWheel(int delta) -> void
    {
        const float notches{ static_cast<float>(delta) / k_WheelNotch };

        // Wheel up raises the speed, narrows the field of view (zooms in) and brings the orbit camera closer.
        if (m_CameraStore.IsKeyDown(Key::Custom))
        {
            const float speed{ m_OptionsStore.GetScalar(Scalar::SpeedCustom) };
            m_OptionsStore.SetScalar(Scalar::SpeedCustom, speed * std::pow(k_SpeedStep, notches));
            return;
        }

        if (m_CameraStore.GetFollow() == Follow::Orbit)
        {
            const float distance{ m_CameraStore.GetOrbitDistance() };
            m_CameraStore.SetOrbitDistance(distance * std::pow(1.0f / k_OrbitStep, notches));
            return;
        }

        const Scalar fov{ m_CameraStore.IsZoomActive() ? Scalar::FovZoom : Scalar::FovNormal };
        m_OptionsStore.SetScalar(fov, m_OptionsStore.GetScalar(fov) * std::pow(1.0f / k_FovStep, notches));
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

        case VK_TAB:
        {
            // Not a viewer key while the viewer is off or the menu uses it to move between widgets.
            if (!m_CameraStore.IsActive() || m_SettingsStore.IsMenuVisible()) return false;

            if (!IsRepeat(message.LParam)) m_CameraStore.ToggleMode();
            return true;
        }

        case VK_OEM_5:
        {
            // The key left of the 1 (|). Only a Free mode key: in Game mode it goes to the game.
            if (!m_CameraStore.IsActive() || m_SettingsStore.IsMenuVisible() ||
                m_CameraStore.GetMode() != Mode::Free) return false;

            if (!IsRepeat(message.LParam)) m_CameraStore.CycleFollow();
            return true;
        }

        case VK_F6:
        {
            if (!IsRepeat(message.LParam))
            {
                m_OptionsStore.ToggleEnabled(Viewer::Options::Type::Flag::Labels);
            }
            return true;
        }

        default:
            return false;
        }
    }

    auto ControlService::HandleMouse(RawMouse& mouse) -> bool
    {
        if (m_SettingsStore.IsMenuVisible() || !m_CameraStore.IsActive()) return false;
        if (m_CameraStore.GetMode() == Mode::Game) return false;

        m_CameraStore.AccumulateMouseDelta(
            static_cast<float>(mouse.DeltaX), static_cast<float>(mouse.DeltaY));

        return true;
    }
}