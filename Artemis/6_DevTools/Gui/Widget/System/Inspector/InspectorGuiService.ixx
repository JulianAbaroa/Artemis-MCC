module;

#include "External/imgui/imgui.h"

export module Gui.Widget.System:Inspector;

import Common.Math.Type;
import Common.Reflect.Type;
import std;

namespace
{
    using Common::Reflect::Type::Reflectable;
    using Common::Reflect::Type::ForEachField;
}

export namespace Gui::Widget::System
{
    // Draws every field of a reflectable object as a tree. Clicking a value copies it.
    // Nested objects and containers become tree nodes, and containers are cut after k_MaxVectorElements.
    class InspectorGuiService
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;

    public:
        InspectorGuiService() = default;
        ~InspectorGuiService() = default;

        InspectorGuiService(const InspectorGuiService&) = delete;
        auto operator=(const InspectorGuiService&) -> InspectorGuiService& = delete;

        // param ownerHandle: Makes the ImGui ids of the tree nodes unique per owner.
        template <typename T> requires Reflectable<T>
        auto Draw(const T& obj, std::uint32_t ownerHandle) -> void
        {
            m_CopyIdSeed = 0;
            this->DrawFields(obj, ownerHandle);
        }

    private:
        std::uint32_t m_CopyIdSeed{};

        static constexpr std::size_t k_MaxVectorElements{ 64 };

        auto DrawLeaf(std::string_view label, const std::string& text) -> void
        {
            const std::string line = std::format("{}: {}", label, text);

            ImGui::Selectable(line.c_str());
            if (ImGui::IsItemClicked())
            {
                ImGui::SetClipboardText(text.c_str());
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Click to copy");
            }
        }

        template <typename T> requires Reflectable<T>
        auto DrawFields(const T& obj, std::uint32_t ownerHandle) -> void
        {
            ForEachField(obj, [&](std::string_view name, auto const& value) {
                this->DrawField(name, value, ownerHandle);
            });
        }

        template <typename V>
        auto DrawField(std::string_view name, const V& value, std::uint32_t ownerHandle) -> void
        {
            if constexpr (std::same_as<V, bool>)
            {
                ImGui::TextColored(value ? ImVec4(0.4f, 0.9f, 0.4f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                    "%.*s: %s", static_cast<int>(name.size()), name.data(), value ? "true" : "false");
            }
            else if constexpr (std::same_as<V, std::string>)
            {
                this->DrawLeaf(name, value.empty() ? std::string("<empty>") : value);
            }
            else if constexpr (std::same_as<V, Vec3>)
            {
                this->DrawLeaf(name, std::format("({:.3f}, {:.3f}, {:.3f})", value.X, value.Y, value.Z));
            }
            else if constexpr (std::is_enum_v<V>)
            {
                this->DrawLeaf(name, std::format("{}", static_cast<std::underlying_type_t<V>>(value)));
            }
            else if constexpr (std::is_arithmetic_v<V>)
            {
                this->DrawLeaf(name, std::format("{}", value));
            }
            else if constexpr (Reflectable<V>)
            {
                const std::string id = std::format("{}##{}_{}", name, ownerHandle, ++m_CopyIdSeed);
                if (ImGui::TreeNode(id.c_str(), "%.*s", static_cast<int>(name.size()), name.data()))
                {
                    this->DrawFields(value, ownerHandle);
                    ImGui::TreePop();
                }
            }
            else if constexpr (requires { value.size(); value.begin(); value.end(); })
            {
                const std::string id = std::format("{}##{}_{}", name, ownerHandle, ++m_CopyIdSeed);
                const std::string header = std::format("{} ({})", name, value.size());

                if (ImGui::TreeNode(id.c_str(), "%s", header.c_str()))
                {
                    std::size_t index{ 0 };
                    for (const auto& element : value)
                    {
                        if (index >= k_MaxVectorElements)
                        {
                            ImGui::TextDisabled("... %zu more not shown", static_cast<std::size_t>(value.size()) - index);
                            break;
                        }

                        this->DrawField(std::format("[{}]", index), element, ownerHandle);
                        ++index;
                    }
                    ImGui::TreePop();
                }
            }
            else
            {
                ImGui::TextDisabled("%.*s: <type not supported by the Inspector>",
                    static_cast<int>(name.size()), name.data());
            }
        }
    };
}