module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Inspector.System;
import :Body;

import Common.Engine.Type;
import Common.Math.System;
import Perception.Players.Type;
import DevTools.Gui.Format.System;
import DevTools.Gui.Widget.System;
import DevTools.Viewer.Format.System;
import std;

namespace
{
    namespace VecMath = Common::Math::System;

    using Perception::Players::Type::GroundKind;

    using DevTools::Gui::Widget::System::PanelGuiService;
    using DevTools::Viewer::Format::System::PlayerFormater;

    using Common::Engine::Type::k_NoHandle;
}

namespace DevTools::Viewer::Inspector::System
{
    auto BodySection::Draw(const Tick& tick, const Body& body) -> void
    {
        ImGui::Text("Zoom: %s", PlayerFormater::ZoomToString(body.Zoom));
        ImGui::Text("Ticks alive: %u", static_cast<unsigned>(body.TicksAlive));

        if (body.LastDamagerPlayer == k_NoHandle)
        {
            ImGui::TextDisabled("Last damager: none");
        }
        else
        {
            const auto* damager{ tick.Players ? tick.Players->Find(body.LastDamagerPlayer) : nullptr };
            ImGui::Text("Last damager: %s", damager ? damager->Name.c_str() : ObjectName::Label(tick, body.LastDamagerPlayer).c_str());
        }

        if (body.TicksSinceDamage) ImGui::Text("Ticks since damage: %u", static_cast<unsigned>(*body.TicksSinceDamage));
        else                       ImGui::TextDisabled("Ticks since damage: never damaged");

        ImGui::Text("Ground: %s", PlayerFormater::GroundToString(body.Ground));

        if (body.Ground == GroundKind::Object) ImGui::Text("Ground object: %s", ObjectName::Label(tick, body.GroundObject).c_str());

        ImGui::Text("Grounded: %s | airborne ticks: %u", body.IsGrounded ? "yes" : "no", static_cast<unsigned>(body.AirborneTicks));
        ImGui::Text("Crouch: %.2f%s", body.CrouchFraction, body.IsCrouching ? " (crouching)" : "");

        PanelGuiService::DrawVec3("Velocity:", body.OwnVelocity);
        ImGui::Text("Speed: %.3f", VecMath::Length(body.OwnVelocity));
    }
}