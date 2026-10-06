module;

#include <d3d11.h>

export module UI.Icon.System;

import UI.Icon.Type;
import std;

export namespace UI::Icon::System
{
    // Creates and releases the GPU textures of the UI icons.
    class IconLoader
    {
    private:
        using IconTexture = UI::Icon::Type::IconTexture;

    public:
        IconLoader() = default;
        ~IconLoader() = default;

        // Decodes an image from memory into a texture.
        // param out: Receives the texture. Its previous texture is released first.
        // return: true if the texture was created.
        static auto LoadIconFromMemory(ID3D11Device* device, const unsigned char* data,
            unsigned int size, IconTexture& out) -> bool;

        // Decodes an image from a byte span into a texture.
        static auto LoadIconFromMemory(ID3D11Device* device,
            std::span<const unsigned char> data, IconTexture& out) -> bool;

        // Releases the texture and resets its size.
        static auto Release(IconTexture& icon) -> void;
    };
}