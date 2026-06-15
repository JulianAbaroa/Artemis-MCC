#pragma once

#include <d3d11.h>
#include <vector>

struct SbspGeometry;
class System_Logs;

class MapPass
{
public:
    MapPass() = default;
    ~MapPass() = default;

    MapPass(const MapPass&) = delete;
    MapPass& operator=(const MapPass&) = delete;

    void Upload(ID3D11Device* device,
        const std::vector<SbspGeometry>& sbsps, System_Logs& logs);

    bool IsUploaded() const { return m_Uploaded; }
    bool HasBuffer() const { return m_VertexBuffer != nullptr; }

    void Draw(ID3D11DeviceContext* ctx);

    void Release();

private:
    ID3D11Buffer* m_VertexBuffer = nullptr;
    UINT m_VertexCount = 0;
    bool m_Uploaded = false;
};