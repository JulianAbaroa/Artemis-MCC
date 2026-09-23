module;

#include "External/imgui/imgui.h"
#include <d3d11.h>

export module UI.Icon.Type;

export namespace UI::Icon::Type
{
    struct IconTexture
    {
        ID3D11ShaderResourceView* SRV = nullptr;
        int Width = 0;
        int Height = 0;

        auto IsValid() const -> bool
        {
            return SRV != nullptr;
        }

        auto ID() const -> ImTextureID
        {
            return (ImTextureID)SRV;
        }
    };
}