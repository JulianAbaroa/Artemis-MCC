module;

#include <d3d11.h>

export module UI.Icon.System;

import UI.Icon.Type;
import std;

export namespace UI::Icon::System
{
    class IconLoader
    {
    private:
        using IconTexture = UI::Icon::Type::IconTexture;

    public:
        IconLoader() = default;
        ~IconLoader() = default;

        static auto LoadIconFromMemory(ID3D11Device* device, const unsigned char* data,
            unsigned int size, IconTexture& out) -> bool;

        static auto LoadIconFromMemory(ID3D11Device* device,
            std::span<const unsigned char> data, IconTexture& out) -> bool;

        static auto Release(IconTexture& icon) -> void;
    };
}