#pragma once

#include "../GpuPipeline.h"
#include "CollidableClassifier.h"

#include <d3d11.h>
#include <cstdint>
#include <memory>

class System_Logs;
struct Tick;

class ZonePass
{
public:
    ZonePass() = default;
    ~ZonePass() = default;

    ZonePass(const ZonePass&) = delete;
    ZonePass& operator=(const ZonePass&) = delete;

    void Upload(ID3D11Device* device, ID3D11DeviceContext* ctx,
        const std::shared_ptr<const Tick>& tick,
        const CollidableClassifier& classifier,
        uint64_t generation, System_Logs& logs);

    bool HasSolid() const { return m_SolidBuffer != nullptr && m_SolidCount > 0; }
    bool HasWire()  const { return m_WireBuffer != nullptr && m_WireCount > 0; }

    void DrawSolid(ID3D11DeviceContext* ctx);
    void DrawWire(ID3D11DeviceContext* ctx);

    void Release();

private:
    bool Ensure(ID3D11Device* device, ID3D11Buffer*& buf, UINT& cap,
        UINT needed, System_Logs& logs);

    ID3D11Buffer* m_SolidBuffer = nullptr;
    ID3D11Buffer* m_WireBuffer = nullptr;
    UINT m_SolidCap = 0, m_WireCap = 0;
    UINT m_SolidCount = 0, m_WireCount = 0;

    uint64_t m_LastGeneration = 0;
    bool m_HasGeneration = false;
};