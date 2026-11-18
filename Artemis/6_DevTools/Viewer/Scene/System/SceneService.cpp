module DevTools.Viewer.Scene.System;

import Common.Engine.Type;
import Perception.Players.Type;
import Views.View.System;
import DevTools.Viewer.Camera.Type;
import DevTools.Viewer.Selection.State;
import std;

namespace
{
    using DevTools::Viewer::Camera::Type::Follow;
    using DevTools::Viewer::Camera::Type::Mode;
    using Perception::Players::Type::Player;

    using Views::View::System::FindObservedView;

    using Common::Engine::Type::k_NoHandle;
}

namespace DevTools::Viewer::Scene::System
{
    auto SceneService::IsActive() const -> bool
    {
        return m_CameraStore.IsActive();
    }

    auto SceneService::GetObservedView(const Tick& tick) const -> const View*
    {
        return tick.Views ? FindObservedView(*tick.Views, m_ObservedStore.Get()) : nullptr;
    }

    auto SceneService::Deactivate() -> void
    {
        m_ObservedStore.Set(k_NoHandle);
        m_CameraService.Deactivate();
    }

    auto SceneService::Update(const std::shared_ptr<const Tick>& tick, const Viewport& viewport) -> void
    {
        const bool acceptInput = !m_SettingsStore.IsMenuVisible();
        const std::uint32_t selected = m_SelectionService.GetSelected();
        const std::optional<std::uint32_t> followHandle =
            (selected == k_NoHandle)
            ? std::nullopt : std::optional<std::uint32_t>{ selected };

        m_CameraService.Update(tick, viewport, acceptInput, followHandle);

        std::uint32_t observed{ k_NoHandle };
        if (followHandle && m_CameraStore.GetFollow() != Follow::Off && tick && tick->Players)
        {
            for (const Player& player : tick->Players->Rows)
            {
                if (player.BipedHandle == *followHandle) observed = player.Handle;
            }
        }
        m_ObservedStore.Set(observed);

        m_PaletteService.Build(tick);

        m_SelectionService.Update(m_CameraService.GetCenterRay());
    }

    auto SceneService::ClearSelection() -> void
    {
        m_SelectionService.Clear();
    }

    auto SceneService::Reset() -> void
    {
        m_ObservedStore.Set(k_NoHandle);
        m_PaletteService.Reset();
        m_SelectionService.Reset();
    }

    auto SceneService::GetViewProjection() const -> const Matrix&
    {
        return m_CameraService.GetViewProjection();
    }

    auto SceneService::IsGameCamera() const -> bool
    {
        return m_CameraStore.GetMode() == Mode::Game;
    }

    auto SceneService::GetGameCamera() const -> const std::optional<GameCamera>&
    {
        return m_CameraService.GetGameCamera();
    }

    auto SceneService::GetSelected() const -> std::uint32_t
    {
        return m_SelectionService.GetSelected();
    }

    auto SceneService::GetPalette() const -> const PaletteService&
    {
        return m_PaletteService;
    }
}