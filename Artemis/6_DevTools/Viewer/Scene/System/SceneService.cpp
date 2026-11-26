module DevTools.Viewer.Scene.System;

import Common.Engine.Type;
import Common.Math.System;
import Engine.Base.Pov.System;
import Engine.Control.Simulation.Type;
import Blam.MapModel.Definitions.Type;
import Perception.Entities.Type;
import Perception.ObjectGraph.Type;
import Perception.Players.Type;
import Views.View.System;
import DevTools.Viewer.Camera.Type;
import DevTools.Viewer.Collidable.Type;
import DevTools.Viewer.Director.Type;
import DevTools.Viewer.Options.Type;
import DevTools.Viewer.Roster.Type;
import DevTools.Viewer.Roster.System;
import DevTools.Viewer.Selection.State;
import std;

namespace
{
    using DevTools::Viewer::Camera::Type::Mode;
    using DevTools::Viewer::Director::Type::DirectorFile;
    using DevTools::Viewer::Director::Type::DirectorInput;
    using DevTools::Viewer::Director::Type::Shot;
    using DevTools::Viewer::Director::Type::ShotKind;
    using DevTools::Viewer::Options::Type::DecideScenePasses;
    using DevTools::Viewer::Options::Type::Flag;
    using DevTools::Viewer::Options::Type::Layer;
    using DevTools::Viewer::Options::Type::Scalar;
    using Blam::MapModel::Definitions::Type::Object::ObjectKind;
    using Engine::Base::Pov::System::IsPovOnOtherPlayer;
    using Perception::ObjectGraph::Type::ObjectNode;
    using Perception::Players::Type::Player;
    using Perception::ObjectGraph::Type::k_MaxParentDepth;

    using Views::View::System::FindObservedView;
    using Views::View::System::ObservedPlayerOf;
    using Views::View::System::ViewPositionOf;

    using DevTools::Viewer::Collidable::Type::ShareObjects;
    using DevTools::Viewer::Director::System::AddShot;
    using DevTools::Viewer::Director::System::DefaultShotName;
    using DevTools::Viewer::Director::System::MakeFixedShot;
    using DevTools::Viewer::Director::System::OrbitFromPose;
    using DevTools::Viewer::Roster::System::IndexOfPlayer;
    using DevTools::Viewer::Roster::System::KeepAgents;
    using DevTools::Viewer::Roster::System::SortRoster;
    using DevTools::Viewer::Roster::System::StepFrom;
    using DevTools::Viewer::Roster::Type::RosterEntry;

    using Common::Engine::Type::k_HandleIndexMask;
    using Common::Engine::Type::k_NoHandle;

    // return: The first biped on the chain of parents of the object (the object itself included) in the graph of the tick, none when there is
    // none (a vehicle or a turret is not hidden), or the object as it is when the tick has no graph or no entities. The game may answer the first
    // person test for a weapon before its unit.
    auto UnitOf(const Bridge::Codec::Tick::Type::Tick& tick, std::uint32_t handle) -> std::uint32_t
    {
        if (!tick.ObjectGraph || !tick.Entities) return handle;

        std::uint32_t current{ handle };
        for (int depth{ 0 }; depth < k_MaxParentDepth; ++depth)
        {
            const auto* entity{ tick.Entities->Find(current) };
            if (entity && entity->Kind == ObjectKind::Biped) return current;

            const ObjectNode* node{ tick.ObjectGraph->Find(current) };
            if (!node || node->ParentHandle == k_NoHandle || node->ParentHandle == current) break;

            current = node->ParentHandle;
        }

        return k_NoHandle;
    }
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

    auto SceneService::GetLocalVehicleHandle(const Tick& tick) const -> std::uint32_t
    {
        const View* view{ tick.Views ? FindObservedView(*tick.Views, k_NoHandle) : nullptr };
        if (!view || view->Head.BipedHandle == k_NoHandle || !tick.ObjectGraph) return k_NoHandle;

        const ObjectNode* node{ tick.ObjectGraph->Find(view->Head.BipedHandle) };
        return node ? node->ParentHandle : k_NoHandle;
    }

    auto SceneService::GetLocalVehiclePosition(const Tick& tick) const -> std::optional<Vec3>
    {
        const std::uint32_t handle{ this->GetLocalVehicleHandle(tick) };
        if (handle == k_NoHandle || !tick.ObjectTable) return std::nullopt;

        const auto parent{ tick.ObjectTable->find(handle) };
        if (parent == tick.ObjectTable->end()) return std::nullopt;

        return parent->second.WorldOrigin;
    }

    auto SceneService::GetLocalEyeRender(const Tick& tick) const -> std::optional<Vec3>
    {
        const std::optional<Vec3> eye{ this->GetLocalEye(tick) };
        const View* view{ tick.Views ? FindObservedView(*tick.Views, k_NoHandle) : nullptr };
        if (!eye || !view) return std::nullopt;

        return Common::Math::System::Add(*eye, m_FramePoses.OffsetOf(view->Head.BipedHandle));
    }

    auto SceneService::GetLocalVehicleRender(const Tick& tick) const -> std::optional<Vec3>
    {
        const std::optional<Vec3> position{ this->GetLocalVehiclePosition(tick) };
        if (!position) return std::nullopt;

        return Common::Math::System::Add(*position, m_FramePoses.OffsetOf(this->GetLocalVehicleHandle(tick)));
    }

    auto SceneService::SetFramePoses(const FramePoses& poses) -> void
    {
        m_FramePoses = poses;
    }

    auto SceneService::GetFramePoses() const -> const FramePoses&
    {
        return m_FramePoses;
    }

    auto SceneService::IsBot(std::uint32_t handle) const -> bool
    {
        const std::optional<std::size_t> index{ IndexOfPlayer(m_Roster, handle) };
        return index && m_Roster[*index].BotNumber.has_value();
    }

    auto SceneService::IsGameMode() const -> bool
    {
        return m_CameraStore.GetMode() == Mode::Game;
    }

    auto SceneService::HiddenHandlesOf(const Tick& tick) const -> std::unordered_set<std::uint32_t>
    {
        std::unordered_set<std::uint32_t> hidden{};
        if (!tick.Views) return hidden;

        // note: Only the Game mode hides the unit, and only when the unit hook says the game hid it: the object it answered true for, which is the
        // biped of the local player or, with the POV, the one of the watched player. Without the hook nothing is hidden (no fallback). The free camera
        // hides nothing: the game draws the body.
        if (m_CameraStore.GetMode() != Mode::Game || !m_RenderStore.IsFirstPersonUnitHooked() || !m_RenderStore.WasFirstPersonUnitLastFrame()) return hidden;

        // The unit above the answered object, so a weapon that answered first still hides the body that holds it.
        const std::uint32_t answered{ m_RenderStore.GetFirstPersonUnitObjectLastFrame() };
        if (answered == k_NoHandle) return hidden;

        const std::uint32_t unit{ UnitOf(tick, answered) };
        if (unit == k_NoHandle) return hidden;

        hidden.insert(unit);

        const ObjectNode* node{ tick.ObjectGraph ? tick.ObjectGraph->Find(unit) : nullptr };
        if (node)
        {
            for (const std::uint32_t child : tick.ObjectGraph->ChildrenOf(*node)) hidden.insert(child);
        }

        return hidden;
    }

    auto SceneService::Deactivate() -> void
    {
        m_Scene.reset();
        m_Roster.clear();
        m_FramePoses = FramePoses{};
        m_ObservedStore.Set(k_NoHandle);
        m_PovRequestStore.Clear();
        m_PossessRequestStore.Clear();
        m_DirectorService.Stop();
        this->ReleaseDirectedSelection();
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

        // The roster goes first: the director picks the agent of a POV shot from it, and the camera takes the pose the director gives.
        m_Roster = tick ? this->BuildRoster(*tick) : std::vector<RosterEntry>{};
        const DirectorFrame directed{ this->RunDirector(tick) };
        m_CameraService.Update(tick, viewport, acceptInput);

        // The steps go before the follow, so the same frame already watches the new player. The player of a POV shot wins over the arrows.
        const int steps{ m_SelectionService.ConsumeStep() };
        if (directed.PovPlayer != k_NoHandle)
        {
            this->FollowDirectedPlayer(directed.PovPlayer);
        }
        else
        {
            this->ReleaseDirectedSelection();
            this->StepSelection(steps, tick);
        }

        // note: The watched player is the one the selection follows, else the local player.
        m_ObservedStore.Set(this->FollowSelectedPlayer(tick));

        m_Hidden = tick ? this->HiddenHandlesOf(*tick) : std::unordered_set<std::uint32_t>{};

        m_PaletteService.Build(tick);

        m_SelectionService.Update(m_CameraService.GetCenterRay(), ShareObjects(scene));

        this->AddDirectorShots();

        m_IsFrameUpdated = m_CameraService.HasRenderPose();
    }

    auto SceneService::RunDirector(const std::shared_ptr<const Tick>& tick) -> DirectorFrame
    {
        const DirectorInput input{ .Now = std::chrono::steady_clock::now(), .HasRoster = tick != nullptr, .Roster = m_Roster,
            .Round = tick && tick->Match ? std::optional<std::int16_t>{ tick->Match->RoundIndex } : std::nullopt };

        const DirectorFrame frame{ m_DirectorService.Update(input) };
        if (!frame.IsPlaying || !frame.HasShot) return frame;

        // note: A POV shot watches from the game camera, the others fly the free camera. The saved options (Watch POV, layers) are not touched: the director
        // wins while it plays and the options rule again when it stops.
        if (frame.Kind == ShotKind::Pov)
        {
            m_CameraStore.SetMode(Mode::Game);
        }
        else
        {
            m_CameraStore.SetMode(Mode::Free);
            m_CameraService.SetScriptedPose(frame.Eye, frame.Yaw, frame.Pitch);
        }

        return frame;
    }

    auto SceneService::PreRoll(const Viewport& viewport) -> void
    {
        if (!m_DirectorStore.IsPlaying()) return;

        // note: No scene yet, so no Tick: the roster is empty and a Fixed or Orbit shot already gives the camera its pose. The frame is not drawn.
        m_Scene.reset();
        m_Roster.clear();
        const DirectorFrame directed{ this->RunDirector(nullptr) };

        // note: Only a shot with a pose of its own (Fixed or Orbit) takes the camera. A shot that is not there yet (a POV first, an empty or holding
        // playlist) leaves the camera alone: it would fly from a place that was never seated.
        if (directed.IsPlaying && directed.HasShot && directed.Kind != ShotKind::Pov)
        {
            m_CameraService.Update(nullptr, viewport, !m_SettingsStore.IsMenuVisible());
        }
    }

    auto SceneService::FollowDirectedPlayer(std::uint32_t playerHandle) -> void
    {
        const std::optional<std::size_t> index{ IndexOfPlayer(m_Roster, playerHandle) };
        if (!index) return;

        m_SelectionService.Follow(m_Roster[*index].Handle, m_Roster[*index].BipedHandle);
        m_DirectedPlayer = playerHandle;
    }

    auto SceneService::ReleaseDirectedSelection() -> void
    {
        if (m_DirectedPlayer == k_NoHandle) return;

        if (m_SelectionService.GetSelectedPlayer() == m_DirectedPlayer) m_SelectionService.Clear();
        m_DirectedPlayer = k_NoHandle;
    }

    auto SceneService::GetScenePasses() const -> ScenePasses
    {
        return DecideScenePasses(m_OptionsStore.IsEnabled(Flag::DrawOverlay), m_OptionsStore.GetLevel(Layer::Obs));
    }

    auto SceneService::AddDirectorShots() -> void
    {
        const bool isAddFixed{ m_DirectorStore.ConsumeAddFixed() };
        const bool isAddOrbit{ m_DirectorStore.ConsumeAddOrbit() };

        // note: Only the free camera has a pose of its own to save.
        if ((!isAddFixed && !isAddOrbit) || m_CameraStore.GetMode() != Mode::Free) return;

        if (isAddFixed)
        {
            Shot shot{ MakeFixedShot(DefaultShotName(m_DirectorStore.CopyFile().Shots, ShotKind::Fixed), m_CameraService.GetEye(), m_CameraService.GetYaw(),
                m_CameraService.GetPitch()) };
            shot.Id = m_DirectorStore.AllocateShotId();

            m_DirectorStore.Edit([&shot](DirectorFile& file) {
                AddShot(file, std::move(shot));
                return true;
            });
        }

        if (isAddOrbit)
        {
            const auto ray{ m_CameraService.GetCenterRay() };
            const std::optional<float> distance{ m_SelectionService.Cast(ray) };
            const std::optional<Vec3> hit{ distance ? std::optional<Vec3>{ Common::Math::System::Add(ray.Origin, Common::Math::System::Scale(ray.Direction, *distance)) }
                : std::nullopt };

            Shot shot{ OrbitFromPose(ray.Origin, ray.Direction, hit) };
            shot.Id = m_DirectorStore.AllocateShotId();
            shot.Name = DefaultShotName(m_DirectorStore.CopyFile().Shots, ShotKind::Orbit);
            shot.DegreesPerSecond = m_OptionsStore.GetScalar(Scalar::DirectorDegreesPerSecond);

            m_DirectorStore.Edit([&shot](DirectorFile& file) {
                AddShot(file, std::move(shot));
                return true;
            });
        }
    }

    auto SceneService::BuildRoster(const Tick& tick) -> std::vector<RosterEntry>
    {
        std::vector<RosterEntry> roster{};
        if (!tick.Players) return roster;

        m_BotStore.CopyBots(m_Bots);
        roster.reserve(tick.Players->Rows.size());

        for (const Player& player : tick.Players->Rows)
        {
            const std::size_t index{ player.Handle & k_HandleIndexMask };
            const auto bot{ std::ranges::find(m_Bots, index, &Bot::PlayerIndex) };

            roster.push_back(RosterEntry{
                .Handle = player.Handle,
                .BipedHandle = player.BipedHandle,
                .Name = player.Name,
                .Team = player.Team,
                .BotNumber = bot != m_Bots.end() ? std::optional<std::size_t>{ bot->Number } : std::nullopt,
                .IsAlive = player.IsAlive() });
        }

        SortRoster(roster);
        if (m_OptionsStore.IsEnabled(Flag::RosterAgentsOnly)) KeepAgents(roster);
        return roster;
    }

    auto SceneService::StepSelection(int delta, const std::shared_ptr<const Tick>& tick) -> void
    {
        if (delta == 0 || m_Roster.empty() || !tick || !tick->Players) return;

        // The player of the selection, found by its biped when the selection came from a pick. A selection that is not a player starts at the end.
        std::uint32_t current{ m_SelectionService.GetSelectedPlayer() };
        if (current == k_NoHandle) current = ObservedPlayerOf(*tick->Players, m_SelectionService.GetSelected());

        const std::optional<std::size_t> next{ StepFrom(m_Roster.size(), IndexOfPlayer(m_Roster, current), delta) };
        if (!next) return;

        m_SelectionService.Follow(m_Roster[*next].Handle, m_Roster[*next].BipedHandle);
    }

    auto SceneService::GetRoster() const -> const std::vector<RosterEntry>&
    {
        return m_Roster;
    }

    auto SceneService::FollowSelectedPlayer(const std::shared_ptr<const Tick>& tick) -> std::uint32_t
    {
        if (!tick || !tick->Players) return k_NoHandle;

        // A new pick holds only the object: its player is found by its current biped.
        std::uint32_t playerHandle{ m_SelectionService.GetSelectedPlayer() };
        if (playerHandle == k_NoHandle) playerHandle = ObservedPlayerOf(*tick->Players, m_SelectionService.GetSelected());
        if (playerHandle == k_NoHandle) return k_NoHandle;

        // A player that left the table ends the selection. A player without a biped keeps it.
        const Player* player{ tick->Players->Find(playerHandle) };
        if (!player)
        {
            m_SelectionService.Clear();
            return k_NoHandle;
        }

        m_SelectionService.Follow(playerHandle, player->BipedHandle);
        return playerHandle;
    }

    auto SceneService::IsPovPlayerDead(const Engine::Base::Pov::Type::PovStatus& status) const -> bool
    {
        if (!IsPovOnOtherPlayer(status) || !m_Scene || !m_Scene->Tick || !m_Scene->Tick->Players) return false;

        const Player* player{ m_Scene->Tick->Players->Find(status.RequestedHandle) };
        return player != nullptr && !player->Body;
    }

    auto SceneService::HasFrameUpdate() const -> bool
    {
        return m_IsFrameUpdated;
    }

    auto SceneService::EndFrame() -> void
    {
        m_IsFrameUpdated = false;
    }

    auto SceneService::ClearSelection() -> void
    {
        m_SelectionService.Clear();
    }

    auto SceneService::Reset() -> void
    {
        m_Scene.reset();
        m_Roster.clear();
        m_IsFrameUpdated = false;
        m_FramePoses = FramePoses{};
        m_ObservedStore.Set(k_NoHandle);
        m_PaletteService.Reset();
        m_SelectionService.Reset();
    }

    auto SceneService::GetViewProjection() const -> const Matrix&
    {
        return m_CameraService.GetViewProjection();
    }

    auto SceneService::IsOverrideActive() const -> bool
    {
        return m_CameraService.IsOverrideActive();
    }

    auto SceneService::GetOverrideFaults() const -> std::uint32_t
    {
        return m_CameraService.GetOverrideFaults();
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