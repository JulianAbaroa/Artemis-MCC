#pragma once

#include "IconTexture.h"

#include "External/imgui/imgui.h"
#include "External/stb/stb_image.h"

#include <d3d11.h>

inline bool LoadIconFromMemory(ID3D11Device* device,
    const unsigned char* data, unsigned int size, IconTexture& out)
{
    int w, h, channels;

    unsigned char* pixels = stbi_load_from_memory(
        data, (int)size, &w, &h, &channels, 4);

    if (!pixels) return false;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sub = {};
    sub.pSysMem = pixels;
    sub.SysMemPitch = w * 4;

    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, &sub, &tex);
    stbi_image_free(pixels);
    if (FAILED(hr)) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    hr = device->CreateShaderResourceView(tex, &srvDesc, &out.SRV);
    tex->Release();
    if (FAILED(hr)) return false;

    out.Width = w;
    out.Height = h;
    return true;
}