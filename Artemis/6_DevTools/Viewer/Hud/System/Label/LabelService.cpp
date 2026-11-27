module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Hud.System;
import :Labels;

import Perception.Players.Type;
import Views.View.System;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Collidable.Type;
import DevTools.Viewer.Options.Type;
import DevTools.Viewer.Roster.System;
import DevTools.Viewer.Hud.Type;
import std;

namespace
{
    using DevTools::Viewer::Options::Type::Flag;
    using DevTools::Viewer::Options::Type::Layer;
    using DevTools::Viewer::Options::Type::Level;
    using Views::View::System::RaycastsOf;
    using Views::View::System::ViewPositionOf;
}

namespace DevTools::Viewer::Hud::System
{
    auto LabelService::Draw(const ScenePasses& passes) -> void
    {
        if (!m_SceneService.IsActive()) return;
        if (!m_OptionsStore.IsEnabled(Flag::Labels)) return;

        const bool isObsOn{ m_OptionsStore.GetLevel(Layer::Obs) == Level::Full };
        if (!passes.DrawDebug && !isObsOn) return;

        const auto scene = m_SceneService.GetFrameScene(m_CollidableStore);
        if (!scene || !scene->Tick) return;
        const auto& tick = scene->Tick;

        m_Classifications.clear();
        if (tick->Classifieds)
        {
            for (const auto& classified : *tick->Classifieds)
            {
                m_Classifications[classified.Handle] = &classified;
            }
        }

        LabelContext context{ *tick, m_Classifications };
        context.Selected = m_SelectionStore.GetSelected();
        context.Hidden = &m_SceneService.GetHiddenHandles();
        context.Poses = &m_SceneService.GetFramePoses();

        // note: The player the camera watches: the followed one, else the primary local player.
        if (const auto* view{ m_SceneService.GetObservedView(*tick) })
        {
            context.SelfBiped = view->Head.BipedHandle;
            if (view->Head.HasBody)
            {
                context.HasSelf = true;
                context.SelfPosition = ViewPositionOf(*view);
                context.AimHit = RaycastsOf(*view).AimHit;
            }
        }

        m_Canvas.Reset();

        if (passes.DrawDebug)
        {
            PlayerLabels::Collect(m_Canvas, context);
            ObjectLabels::Collect(m_Canvas, context);
            VehicleLabels::Collect(m_Canvas, context);
            HealthLabels::Collect(m_Canvas, context);
            InteractionLabels::Collect(m_Canvas, context);
            FixtureLabels::Collect(m_Canvas, context);
        }

        // note: The distances of the observation arrows (VW.17) are only drawn at the Full level, under the master switch of the labels but not the one of the
        // debug overlay (a Presentation layer).
        if (isObsOn) ObsLabels::Collect(m_Canvas, context, m_SceneService.GetObsRows());

        m_Canvas.Flush(*ImGui::GetBackgroundDrawList());
    }
}