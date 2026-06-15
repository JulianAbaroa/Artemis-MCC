#include "pch.h"

#include "FlyCamera.h"

#include <cmath>

namespace
{
    inline std::array<float, 3> Norm(const std::array<float, 3>& v)
    {
        float len = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

        if (len < 1e-6f)
        {
            return { 0.0f, 0.0f, 1.0f };
        }

        return { v[0] / len, v[1] / len, v[2] / len };
    }

    inline std::array<float, 3> Cross(const std::array<float, 3>& a,
        const std::array<float, 3>& b)
    {
        return { a[1] * b[2] - a[2] * b[1], 
            a[2] * b[0] - a[0] * b[2], 
        
            a[0] * b[1] - a[1] * b[0] };
    }
    inline float Dot(const std::array<float, 3>& a, const std::array<float, 3>& b)
    {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    }
}

void FlyCamera::SetPose(const std::array<float, 3>& eye, float yaw, float pitch)
{
    m_Eye = eye;
    m_Yaw = yaw;

    const float kLimit = 1.55334f;
    if (pitch > kLimit) pitch = kLimit;
    if (pitch < -kLimit) pitch = -kLimit;
    m_Pitch = pitch;
}

void FlyCamera::SetLens(float fovYRadians, float nearZ, float farZ)
{
    m_FovY = fovYRadians;
    m_Near = nearZ;
    m_Far = farZ;
}

void FlyCamera::Build(const Viewport& vp)
{
    m_Vp = vp;

    const float cp = std::cos(m_Pitch);
    const float sp = std::sin(m_Pitch);
    const float cy = std::cos(m_Yaw);
    const float sy = std::sin(m_Yaw);

    m_Forward = Norm({ cp * cy, cp * sy, sp });

    const std::array<float, 3> worldUp = { 0.0f, 0.0f, 1.0f };
    m_Right = Norm(Cross(m_Forward, worldUp));
    m_Up = Cross(m_Right, m_Forward);

    const std::array<float, 3> f = m_Forward;
    const std::array<float, 3> r = m_Right;
    const std::array<float, 3> u = m_Up;

    float view[16] = {
        r[0], u[0], -f[0], 0.0f,
        r[1], u[1], -f[1], 0.0f,
        r[2], u[2], -f[2], 0.0f,
        -Dot(r, m_Eye), -Dot(u, m_Eye), Dot(f, m_Eye), 1.0f,
    };

    const float aspect = (vp.CanvasSize.y > 0.0f)
        ? (vp.CanvasSize.x / vp.CanvasSize.y) : 1.0f;
    const float ff = 1.0f / std::tan(m_FovY * 0.5f);
    const float nf = 1.0f / (m_Near - m_Far);

    float proj[16] = {
        ff / aspect, 0.0f, 0.0f, 0.0f,
        0.0f, ff, 0.0f, 0.0f,
        0.0f, 0.0f, (m_Far + m_Near) * nf, -1.0f,
        0.0f, 0.0f, 2.0f * m_Far * m_Near * nf, 0.0f,
    };

    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            float s = 0.0f;

            for (int k = 0; k < 4; ++k)
            {
                s += view[row * 4 + k] * proj[k * 4 + col];
            }

            m_ViewProj[row * 4 + col] = s;
        }
    }
}

bool FlyCamera::ToCanvas(float wx, float wy, float wz, ImVec2& outPx) const
{
    const float* m = m_ViewProj;

    const float cx = wx * m[0] + wy * m[4] + wz * m[8] + m[12];
    const float cy = wx * m[1] + wy * m[5] + wz * m[9] + m[13];
    const float cw = wx * m[3] + wy * m[7] + wz * m[11] + m[15];

    if (cw <= 1e-6f) return false;

    const float ndcX = cx / cw;
    const float ndcY = cy / cw;

    outPx.x = m_Vp.CanvasPos.x + (ndcX * 0.5f + 0.5f) * m_Vp.CanvasSize.x;
    outPx.y = m_Vp.CanvasPos.y + (1.0f - (ndcY * 0.5f + 0.5f)) * m_Vp.CanvasSize.y;

    return true;
}

void FlyCamera::ScreenToRay(const ImVec2& canvasPx,
    std::array<float, 3>& outOrigin,
    std::array<float, 3>& outDir) const
{
    const float u = (canvasPx.x - m_Vp.CanvasPos.x) / m_Vp.CanvasSize.x;
    const float v = (canvasPx.y - m_Vp.CanvasPos.y) / m_Vp.CanvasSize.y;

    const float ndcX = u * 2.0f - 1.0f;
    const float ndcY = (1.0f - v) * 2.0f - 1.0f;

    const float tanHalf = std::tan(m_FovY * 0.5f);
    const float aspect = (m_Vp.CanvasSize.y > 0.0f)
        ? (m_Vp.CanvasSize.x / m_Vp.CanvasSize.y) : 1.0f;

    const float vx = ndcX * tanHalf * aspect;
    const float vy = ndcY * tanHalf;

    std::array<float, 3> dir = {
        m_Forward[0] + vx * m_Right[0] + vy * m_Up[0],
        m_Forward[1] + vx * m_Right[1] + vy * m_Up[1],
        m_Forward[2] + vx * m_Right[2] + vy * m_Up[2],
    };

    outOrigin = m_Eye;
    outDir = Norm(dir);
}