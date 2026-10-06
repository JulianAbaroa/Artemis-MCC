module Viewer.Scene.System;

import Viewer.Selection.State;
import std;

namespace
{
    using Viewer::Selection::State::k_NoSelection;
}

namespace Viewer::Scene::System
{
    auto SceneService::IsActive() const -> bool
    {
        return m_CameraStore.IsActive();
    }

    auto SceneService::Deactivate() -> void
    {
        m_CameraService.Deactivate();
    }

    auto SceneService::Update(const std::shared_ptr<const Tick>& tick, const Viewport& viewport) -> void
    {
        const bool acceptInput = !m_SettingsStore.IsMenuVisible();
        const std::uint32_t selected = m_SelectionService.GetSelected();
        const std::optional<std::uint32_t> followHandle =
            (selected == k_NoSelection)
            ? std::nullopt : std::optional<std::uint32_t>{ selected };

        m_CameraService.Update(tick, viewport, acceptInput, followHandle);

        m_PaletteService.Build(tick);

        m_SelectionService.Update(tick, m_CameraService.GetCenterRay());
    }

    auto SceneService::ClearSelection() -> void
    {
        m_SelectionService.Clear();
    }

    auto SceneService::Reset() -> void
    {
        m_PaletteService.Reset();
        m_SelectionService.Reset();
    }

    auto SceneService::GetViewProjection() const -> const Matrix&
    {
        return m_CameraService.GetViewProjection();
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