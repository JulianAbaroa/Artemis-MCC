module;

#include "External/imgui/imgui.h"

module Viewer.Hud.System;
import :Labels;

import Viewer.Options.Type;
import Viewer.Hud.Type;
import std;

namespace
{
    using Viewer::Options::Type::Flag;
}

namespace Viewer::Hud::System
{
    auto LabelService::Draw() -> void
    {
        if (!m_SceneService.IsActive()) return;
        if (!m_OptionsStore.IsEnabled(Flag::Labels)) return;

        const auto tick = m_TickStore.Acquire();
        if (!tick) return;

        m_Roles.clear();
        if (tick->Classifieds)
        {
            for (const auto& classified : *tick->Classifieds)
            {
                m_Roles[classified.Handle] = classified.Role;
            }
        }

        LabelContext context{ *tick, m_OptionsStore, m_Roles };
        context.Selected = m_SelectionStore.GetSelected();

        if (tick->Self && tick->Self->IsAlive)
        {
            context.HasSelf = true;
            context.SelfPosition = tick->Self->Position;
            context.SelfBiped = tick->Self->BipedHandle;
        }
        else if (tick->Self)
        {
            context.SelfBiped = tick->Self->BipedHandle;
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