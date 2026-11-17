module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Hud.System;
import :Labels;

import Views.View.System;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Collidable.Type;
import DevTools.Viewer.Options.Type;
import DevTools.Viewer.Hud.Type;
import std;

namespace
{
    using DevTools::Viewer::Options::Type::Flag;
    using Views::View::System::RaycastsOf;
    using Views::View::System::ViewPositionOf;
}

namespace DevTools::Viewer::Hud::System
{
    auto LabelService::Draw() -> void
    {
        if (!m_SceneService.IsActive()) return;
        if (!m_OptionsStore.IsEnabled(Flag::Labels)) return;

        const auto scene = m_CollidableStore.Acquire();
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

        PlayerLabels::Collect(m_Canvas, context);
        ObjectLabels::Collect(m_Canvas, context);
        VehicleLabels::Collect(m_Canvas, context);
        HealthLabels::Collect(m_Canvas, context);
        InteractionLabels::Collect(m_Canvas, context);
        FixtureLabels::Collect(m_Canvas, context);

        m_Canvas.Flush(*ImGui::GetBackgroundDrawList());
    }
}