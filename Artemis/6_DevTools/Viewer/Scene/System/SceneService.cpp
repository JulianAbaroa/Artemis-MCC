module DevTools.Viewer.Scene.System;

import Common.Engine.Type;
import Common.Math.System;
import Perception.ObjectGraph.Type;
import Perception.Players.Type;
import Views.View.System;
import DevTools.Viewer.Camera.Type;
import DevTools.Viewer.Collidable.Type;
import DevTools.Viewer.Selection.State;
import std;

namespace
{
    using DevTools::Viewer::Camera::Type::Follow;
    using DevTools::Viewer::Camera::Type::Mode;
    using Perception::ObjectGraph::Type::ObjectNode;
    using Perception::Players::Type::Player;

    using Views::View::System::FindObservedView;
    using Views::View::System::ViewPositionOf;

    using DevTools::Viewer::Collidable::Type::ShareObjects;

    using Common::Engine::Type::k_NoHandle;

    // The game camera counts as in the eyes of the View when closer than this. On foot it measures 0.004, in third person it is meters.
    constexpr float k_FirstPersonDistance{ 0.1f };
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

    auto SceneService::GetHiddenHandles() const -> const std::unordered_set<std::uint32_t>&
    {
        return m_Hidden;
    }

    auto SceneService::GetEyeDistance(const Tick& tick) const -> std::optional<float>
    {
        const std::optional<GameCamera>& camera{ m_CameraService.GetGameCamera() };
        const std::optional<Vec3> eye{ this->GetLocalEye(tick) };
        if (!camera || !eye) return std::nullopt;

        return static_cast<float>(Common::Math::System::Distance(camera->Position, *eye));
    }

    auto SceneService::GetLocalEye(const Tick& tick) const -> std::optional<Vec3>
    {
        const View* view{ tick.Views ? FindObservedView(*tick.Views, k_NoHandle) : nullptr };
        if (!view || !view->Head.HasBody) return std::nullopt;

        return ViewPositionOf(*view);
    }

    auto SceneService::GetLocalVehiclePosition(const Tick& tick) const -> std::optional<Vec3>
    {
        const View* view{ tick.Views ? FindObservedView(*tick.Views, k_NoHandle) : nullptr };
        if (!view || view->Head.BipedHandle == k_NoHandle || !tick.ObjectGraph || !tick.ObjectTable) return std::nullopt;

        const ObjectNode* node{ tick.ObjectGraph->Find(view->Head.BipedHandle) };
        if (!node || node->ParentHandle == k_NoHandle) return std::nullopt;

        const auto parent{ tick.ObjectTable->find(node->ParentHandle) };
        if (parent == tick.ObjectTable->end()) return std::nullopt;

        return parent->second.WorldOrigin;
    }

    auto SceneService::IsGameMode() const -> bool
    {
        return m_CameraStore.GetMode() == Mode::Game;
    }

    auto SceneService::HiddenHandlesOf(const Tick& tick) const -> std::unordered_set<std::uint32_t>
    {
        std::unordered_set<std::uint32_t> hidden{};
        if (!tick.Views) return hidden;

        const View* view{ nullptr };
        if (m_CameraStore.GetMode() == Mode::Game)
        {
            // note: Without a valid game camera the camera sits on the View of the local player, so it is hidden.
            const std::optional<float> distance{ this->GetEyeDistance(tick) };
            if (!distance || *distance < k_FirstPersonDistance) view = FindObservedView(*tick.Views, k_NoHandle);
        }
        else if (m_CameraStore.GetFollow() == Follow::FirstPerson && m_ObservedStore.Get() != k_NoHandle) view = this->GetObservedView(tick);

        if (!view || view->Head.BipedHandle == k_NoHandle) return hidden;

        hidden.insert(view->Head.BipedHandle);

        const ObjectNode* node{ tick.ObjectGraph ? tick.ObjectGraph->Find(view->Head.BipedHandle) : nullptr };
        if (node)
        {
            for (const std::uint32_t child : tick.ObjectGraph->ChildrenOf(*node)) hidden.insert(child);
        }

        return hidden;
    }

    auto SceneService::Deactivate() -> void
    {
        m_Scene.reset();
        m_ObservedStore.Set(k_NoHandle);
        m_CameraService.Deactivate();
    }

    auto SceneService::GetFrameScene(const CollidableStore& store) const -> std::shared_ptr<const CollidableScene>
    {
        return m_Scene ? m_Scene : store.Acquire();
    }

    auto SceneService::Update(const std::shared_ptr<const CollidableScene>& scene, const Viewport& viewport) -> void
    {
        m_Scene = scene;
        const std::shared_ptr<const Tick> tick{ scene ? scene->Tick : nullptr };

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

        m_Hidden = tick ? this->HiddenHandlesOf(*tick) : std::unordered_set<std::uint32_t>{};

        m_PaletteService.Build(tick);

        m_SelectionService.Update(m_CameraService.GetCenterRay(), ShareObjects(scene));
    }

    auto SceneService::ClearSelection() -> void
    {
        m_SelectionService.Clear();
    }

    auto SceneService::Reset() -> void
    {
        m_Scene.reset();
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
        return m_CameraStore.GetMode() == Mode::Game || m_CameraService.IsDriving();
    }

    auto SceneService::IsOverrideActive() const -> bool
    {
        return m_CameraService.IsOverrideActive();
    }

    auto SceneService::GetOverrideFaults() const -> std::uint32_t
    {
        return m_CameraService.GetOverrideFaults();
    }

    auto SceneService::GetOverrideDistance() const -> std::optional<float>
    {
        return m_CameraService.GetOverrideDistance();
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