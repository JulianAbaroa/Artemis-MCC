#include "pch.h"
#include "Core/UI/Tabs/Domain/Map/TagRenderer.h"
#include "External/imgui/imgui.h"

void TagRenderer::Render(const std::vector<FieldDefinition>& schema, void* baseAddr) 
{
    for (const auto& field : schema) 
    {
        void* ptr = (uint8_t*)baseAddr + field.Offset;

        switch (field.Type) {
        case FieldType::Float32:
            ImGui::InputFloat(field.Name, (float*)ptr);
            break;

        case FieldType::Int32:
            ImGui::InputInt(field.Name, (int*)ptr);
            break;

        case FieldType::Flags32:
            ImGui::Text("%s: 0x%08X", field.Name, *(uint32_t*)ptr);
            break;

        case FieldType::TagBlock:
            this->DrawTagBlock(field.Name, field.ExtraInfo, ptr);
            break;

        default:
            ImGui::TextDisabled("%s (Tipo no soportado)", field.Name);
            break;
        }
    }
}

void TagRenderer::DrawTagBlock(const char* label, const char* schemaName, void* ptr) 
{
    if (ImGui::TreeNode(label)) 
    {
        ImGui::Text("Schema: %s", schemaName);
        ImGui::TreePop();
    }
}