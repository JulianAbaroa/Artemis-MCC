#pragma once

#include "External/imgui/imgui.h"

#include <array>

class FlyCamera
{
public:
    struct Viewport
    {
        ImVec2 CanvasPos;
        ImVec2 CanvasSize;
    };

    void SetPose(const std::array<float, 3>& eye, float yaw, float pitch);
    void SetLens(float fovYRadians, float nearZ, float farZ);

    std::array<float, 3> Forward() const { return m_Forward; }
    std::array<float, 3> Right()   const { return m_Right; }
    std::array<float, 3> Up()      const { return m_Up; }
    std::array<float, 3> Eye()     const { return m_Eye; }

    float Yaw()   const { return m_Yaw; }
    float Pitch() const { return m_Pitch; }

    void Build(const Viewport& vp);

    const float* ViewProjData() const { return m_ViewProj; }

    bool ToCanvas(float wx, float wy, float wz, ImVec2& outPx) const;

    void ScreenToRay(const ImVec2& canvasPx,
        std::array<float, 3>& outOrigin,
        std::array<float, 3>& outDir) const;

private:
    std::array<float, 3> m_Eye{ 0.0f, 0.0f, 0.0f };
    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;

    float m_FovY = 1.0472f;
    float m_Near = 0.05f;
    float m_Far = 5000.0f;

    std::array<float, 3> m_Forward{ 1.0f, 0.0f, 0.0f };
    std::array<float, 3> m_Right{ 0.0f, 1.0f, 0.0f };
    std::array<float, 3> m_Up{ 0.0f, 0.0f, 1.0f };

    Viewport m_Vp{};
    float m_ViewProj[16]{};
};