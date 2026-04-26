#pragma once

#include <cstdint>
#include <string>

class PhysicsDebugTab
{
public:
	void Draw();
	void Cleanup();

private:
	uint32_t m_SelectedHandle = 0xFFFFFFFF;

    void DrawPlayerSection();
    void DrawObjectList();
    void DrawObjectDetail(uint32_t handle);
    void DrawLiveObjectSection(uint32_t handle);
    void DrawPhysicsModelSection(uint32_t handle);
    void DrawLivePhysicsSection(uint32_t handle);
    void DrawOrientationVerifier(uint32_t handle);

    std::string GetShortName(const std::string& path) const;

    // Infers cardinal direction from a forward vector (XY plane).
    // Returns "N", "NE", "E", "SE", "S", "SW", "W", "NW"
    const char* ForwardToCardinal(float fx, float fy) const;

    // Computes distance between two 3D points.
    float Distance3D(const float a[3], const float b[3]) const;
};