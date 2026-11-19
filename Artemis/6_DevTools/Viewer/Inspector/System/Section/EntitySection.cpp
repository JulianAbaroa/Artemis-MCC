module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Inspector.System;
import :Entity;

import Common.Semantic.Type;
import DevTools.Gui.Format.System;
import std;

namespace
{
    using Perception::Entities::Type::Entity;

    using DevTools::Gui::Format::System::HexFormater;

    using Common::Semantic::Type::Capability;
    using Common::Semantic::Type::Lifecycle;
    using Common::Semantic::Type::HasCapability;

    namespace SemanticType = Common::Semantic::Type;

    // Capabilities in bit order, with the name the panel shows.
    constexpr std::array<std::pair<Capability, std::string_view>, 12> k_CapabilityNames
    {{
        { Capability::HasVitality, "HasVitality" },
        { Capability::HasCollision, "HasCollision" },
        { Capability::HasZone, "HasZone" },
        { Capability::IsPickable, "IsPickable" },
        { Capability::IsEnterable, "IsEnterable" },
        { Capability::IsUsable, "IsUsable" },
        { Capability::IsDestructible, "IsDestructible" },
        { Capability::IsDynamic, "IsDynamic" },
        { Capability::BlocksMovement, "BlocksMovement" },
        { Capability::BlocksProjectiles, "BlocksProjectiles" },
        { Capability::CanDamage, "CanDamage" },
        { Capability::IsObjective, "IsObjective" },
    }};
}

namespace DevTools::Viewer::Inspector::System
{
    auto EntitySection::Find(const Tick& tick, std::uint32_t handle) -> const Entity*
    {
        return tick.Entities ? tick.Entities->Find(handle) : nullptr;
    }

    auto EntitySection::Draw(const Entity& entity) -> void
    {
        std::string function{ SemanticType::NameOf(entity.Function) };
        const std::string_view subtype{ SemanticType::Subtype::NameOf(entity.Function, entity.Subtype) };
        if (!subtype.empty()) function += std::format(" {}", subtype);

        ImGui::Text("%s | %s", function.c_str(), std::string{ SemanticType::NameOf(entity.Group) }.c_str());

        if (entity.Lifecycle == Lifecycle::Dead) ImGui::TextColored({ 1.0f, 0.35f, 0.35f, 1.0f }, "[ DEAD ]");
        else                                      ImGui::TextColored({ 0.4f, 1.0f, 0.4f, 1.0f }, "[ ACTIVE ]");

        ImGui::Text("Binding: %s | Bound to: %s", std::string{ SemanticType::NameOf(entity.Binding) }.c_str(),
            HexFormater::Hex32(entity.BoundTo).c_str());
        ImGui::Text("Physics: %s | Team: %u", std::string{ SemanticType::NameOf(entity.Physics) }.c_str(),
            static_cast<unsigned>(entity.Team));

        if (entity.Vitality) ImGui::Text("Vitality: %.2f", *entity.Vitality);
        else                  ImGui::TextDisabled("Vitality: none");

        std::string capabilities{};
        for (const auto& [capability, name] : k_CapabilityNames)
        {
            if (!HasCapability(entity.Capabilities, capability)) continue;

            if (!capabilities.empty()) capabilities += ' ';
            capabilities += name;
        }

        ImGui::TextWrapped("Capabilities: %s", capabilities.empty() ? "none" : capabilities.c_str());
    }
}