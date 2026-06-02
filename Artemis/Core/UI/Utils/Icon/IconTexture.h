#pragma once

#include "External/imgui/imgui.h"

#include <d3d11.h>

struct IconTexture
{
    ID3D11ShaderResourceView* SRV = nullptr;
    int Width = 0;
    int Height = 0;

    ImTextureID ID() const
    {
        return (ImTextureID)SRV;
    }
};