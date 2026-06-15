#include "pch.h"

#include "HandleDrawer.h"

#include "Core/Types/Sources/Tables/Player/LivePlayer.h"

#include "Core/UI/Utils/Field/CopyableField.h"
#include "Core/UI/Utils/Hex/HexFormater.h"

#include "External/imgui/imgui.h"

void HandleDrawer::DrawU32(const char* label, uint32_t handle,
    const LivePlayer& player, CopyableField& field)
{
    if (handle == 0xFFFFFFFF)
    {
        ImGui::TextDisabled("none", label);
    }
    else
    {
        field.Draw(label, HexFormater::Hex32(handle),
            player.Handle ^ (uint32_t)(uintptr_t)label);
    }
}