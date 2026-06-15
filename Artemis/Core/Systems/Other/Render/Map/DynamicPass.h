#pragma once

#include "Core/States/Other/Render/State_Selection.h"

#include <d3d11.h>
#include <cstdint>
#include <memory>
#include <vector>

struct Collidable;
class System_Logs;
class CollidableClassifier;

using Collidables = std::vector<Collidable>;

struct ObjectAABB
{
    uint32_t Handle;
    float MinX, MinY, MinZ;
    float MaxX, MaxY, MaxZ;
};

class DynamicPass
{
public:
    DynamicPass() = default;
    ~DynamicPass() = default;

    DynamicPass(const DynamicPass&) = delete;
    DynamicPass& operator=(const DynamicPass&) = delete;

    void Upload(ID3D11Device* device, ID3D11DeviceContext* ctx,
        const std::shared_ptr<const Collidables>& collidables,
        const CollidableClassifier& classifier,
        uint32_t selectedHandle,
        uint64_t generation, System_Logs& logs);

    bool HasBuffer() const { return m_VertexBuffer != nullptr; }

    const std::vector<ObjectAABB>& GetAABBs() const { return m_AABBs; }

    void Draw(ID3D11DeviceContext* ctx);

    void Release();

private:
    bool EnsureCapacity(ID3D11Device* device, UINT vertexCapacity,
        System_Logs& logs);

    ID3D11Buffer* m_VertexBuffer = nullptr;
    UINT m_Capacity = 0; 
    UINT m_VertexCount = 0;
    uint64_t m_LastGeneration = 0;
    bool m_HasGeneration = false;

    uint32_t m_LastSelected = kNoSelection;
    std::vector<ObjectAABB> m_AABBs;
};