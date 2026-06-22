#include "pch.h"

#include "System_MapRenderer.h"

#include "Core/Types/Tick/Tick.h"
#include "Core/Types/Sources/Tables/Player/LivePlayer.h"
#include "Core/Types/Environment/Collidable/Collidable.h"
#include "Core/Types/Egocentric/Self/Self.h"

#include "Core/States/Sources/Static/World/State_WorldBuilder.h"
#include "Core/States/Other/Render/State_Render.h"
#include "Core/States/Other/Render/State_FlyCamera.h"
#include "Core/States/Other/Render/State_Selection.h"

#include "Core/Systems/Other/Render/GpuStateGuard.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

#include "External/imgui/imgui.h"

#include <cmath>

void System_MapRenderer::DrawFrame(std::shared_ptr<const Tick> tick)
{
    if (!tick) return;
    if (!m_Deps.State_FlyCamera.IsActive())
    {
        m_PrevCameraActive = false;
        return;
    }

    if (!m_Pipeline.Init(m_Deps.State_Render.GetDevice(), m_Deps.System_Logs))
        return;

    auto& wb = m_Deps.State_WorldBuilder;

    if (wb.IsFrozen() && !m_MapPass.IsUploaded())
        m_MapPass.Upload(m_Deps.State_Render.GetDevice(),
            wb.GetAllSbspGeometry(), m_Deps.System_Logs);

    if (!m_MapPass.HasBuffer()) return;

    const float w = static_cast<float>(m_Deps.State_Render.GetWidth());
    const float h = static_cast<float>(m_Deps.State_Render.GetHeight());

    UpdateCamera(tick, w, h);

    ID3D11DeviceContext* ctx = m_Deps.State_Render.GetContext();
    if (!ctx) return;

    ID3D11Device* device = m_Deps.State_Render.GetDevice();

    m_Classifier.Build(tick);

    PerformPick();

    m_DynamicPass.Upload(device, ctx, tick->Collidables,
        m_Classifier, m_Deps.State_Selection.GetSelected(),
        tick->Generation, m_Deps.System_Logs);

    m_ZonePass.Upload(device, ctx, tick, m_Classifier,
        tick->Generation, m_Deps.System_Logs);

    GpuStateGuard guard(ctx);

    SetupFrame(ctx, w, h);

    m_Pipeline.UpdateCamera(ctx, m_Camera.ViewProjData());

    using Fill = GpuPipeline::Fill;

    m_Pipeline.Bind(ctx, Fill::Solid);
    m_MapPass.Draw(ctx);
    m_DynamicPass.Draw(ctx);

    m_Pipeline.Bind(ctx, Fill::Solid);
    m_ZonePass.DrawSolid(ctx);

    guard.Restore();

    ImDrawList* fg = ImGui::GetForegroundDrawList();
    const ImVec2 center(w * 0.5f, h * 0.5f);
    const ImU32 celeste = IM_COL32(115, 200, 245, 230);
    fg->AddCircleFilled(center, 5.0f, celeste, 16);
}

void System_MapRenderer::UpdateCamera(const std::shared_ptr<const Tick>& tick,
    float w, float h)
{
    auto& fc = m_Deps.State_FlyCamera;

    const bool active = fc.IsActive();
    const bool justActivated = active && !m_PrevCameraActive;
    m_PrevCameraActive = active;

    if (fc.FollowEnabled() && tick)
    {
        const uint32_t sel = m_Deps.State_Selection.GetSelected();

        const std::array<float, 3>* pos = nullptr;
        const std::array<float, 3>* fwd = nullptr;

        if (sel != kNoSelection)
        {
            if (tick->PlayerTable)
            {
                for (const auto& [handle, p] : *tick->PlayerTable)
                {
                    if (p.AliveBipedHandle == sel)
                    {
                        pos = &p.WeaponPosition;
                        fwd = &p.WeaponForward;
                        break;
                    }
                }
            }

            if (!pos && tick->Collidables)
            {
                for (const auto& c : *tick->Collidables)
                {
                    if (c.Handle == sel)
                    {
                        pos = &c.Position;
                        fwd = &c.Forward;
                        break;
                    }
                }
            }
        }

        if (pos && fwd)
        {
            const auto& f = *fwd;
            const float len = std::sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);

            float yaw = m_Camera.Yaw();
            float pitch = m_Camera.Pitch();
            if (len > 1e-6f)
            {
                yaw = std::atan2(f[1], f[0]);
                pitch = std::asin(f[2] / len);
            }

            m_Camera.SetLens(1.0472f, 0.05f, 10000.0f);
            m_Camera.SetPose(*pos, yaw, pitch);

            float dx, dy;
            fc.ConsumeMouseDelta(dx, dy);

            FlyCamera::Viewport vp;
            vp.CanvasPos = ImVec2(0, 0);
            vp.CanvasSize = ImVec2(w, h);
            m_Camera.Build(vp);
            return;
        }
    }

    if (justActivated)
    {
        m_Camera.SetLens(1.0472f, 0.05f, 10000.0f);

        std::array<float, 3> eye = { 0.0f, 0.0f, 0.0f };
        float yaw = 0.0f;
        float pitch = 0.0f;

        if (tick && tick->Self)
        {
            const auto& self = *tick->Self;
            eye = self.Position;

            const auto& f = self.Forward;
            const float len = std::sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
            if (len > 1e-6f)
            {
                const float fx = f[0] / len, fy = f[1] / len, fz = f[2] / len;
                yaw = std::atan2(fy, fx);
                pitch = std::asin(fz);
            }
        }

        m_Camera.SetPose(eye, yaw, pitch);
    }

    float mdx, mdy;
    fc.ConsumeMouseDelta(mdx, mdy);
    const float lookSpeed = 0.0025f;
    float yaw = m_Camera.Yaw() - mdx * lookSpeed;
    float pitch = m_Camera.Pitch() - mdy * lookSpeed;

    auto eye = m_Camera.Eye();
    auto fwd = m_Camera.Forward();
    auto rgt = m_Camera.Right();

    float moveSpeed = 0.05f;
    if (fc.KeyShift()) moveSpeed = 0.5f;
    if (fc.KeyCtrl()) moveSpeed = 0.01f;

    if (fc.KeyW()) { eye[0] += fwd[0] * moveSpeed; eye[1] += fwd[1] * moveSpeed; eye[2] += fwd[2] * moveSpeed; }
    if (fc.KeyS()) { eye[0] -= fwd[0] * moveSpeed; eye[1] -= fwd[1] * moveSpeed; eye[2] -= fwd[2] * moveSpeed; }
    if (fc.KeyD()) { eye[0] += rgt[0] * moveSpeed; eye[1] += rgt[1] * moveSpeed; eye[2] += rgt[2] * moveSpeed; }
    if (fc.KeyA()) { eye[0] -= rgt[0] * moveSpeed; eye[1] -= rgt[1] * moveSpeed; eye[2] -= rgt[2] * moveSpeed; }
    if (fc.KeyUp())   eye[2] += moveSpeed;
    if (fc.KeyDown()) eye[2] -= moveSpeed;

    m_Camera.SetPose(eye, yaw, pitch);

    FlyCamera::Viewport vp;
    vp.CanvasPos = ImVec2(0, 0);
    vp.CanvasSize = ImVec2(w, h);
    m_Camera.Build(vp);
}

void System_MapRenderer::PerformPick()
{
    if (!m_Deps.State_Selection.ConsumePick()) return;

    const auto origin = m_Camera.Eye();
    const auto dir = m_Camera.Forward();

    auto intersect = [&](const ObjectAABB& b) -> float {
        float tmin = 0.0f;
        float tmax = 1e30f;

        const float o[3] = { origin[0], origin[1], origin[2] };
        const float d[3] = { dir[0], dir[1], dir[2] };
        const float lo[3] = { b.MinX, b.MinY, b.MinZ };
        const float hi[3] = { b.MaxX, b.MaxY, b.MaxZ };

        for (int i = 0; i < 3; ++i)
        {
            if (std::abs(d[i]) < 1e-8f)
            {
                if (o[i] < lo[i] || o[i] > hi[i]) return -1.0f;
            }
            else
            {
                const float inv = 1.0f / d[i];
                float t1 = (lo[i] - o[i]) * inv;
                float t2 = (hi[i] - o[i]) * inv;
                if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
                if (t1 > tmin) tmin = t1;
                if (t2 < tmax) tmax = t2;
                if (tmin > tmax) return -1.0f;
            }
        }
        return tmin;
        };

    uint32_t best = kNoSelection;
    float bestT = 1e30f;

    for (const auto& aabb : m_DynamicPass.GetAABBs())
    {
        const float t = intersect(aabb);
        if (t >= 0.0f && t < bestT)
        {
            bestT = t;
            best = aabb.Handle;
        }
    }

    m_Deps.State_Selection.SetSelected(best);
}

void System_MapRenderer::SetupFrame(ID3D11DeviceContext* ctx, float w, float h)
{
    ID3D11RenderTargetView* rtv = m_Deps.State_Render.GetRTV();
    ID3D11DepthStencilView* dsv = m_Deps.State_Render.GetDSV();
    if (!rtv || !dsv) return;

    const float bgColor[4] = { 0.05f, 0.06f, 0.08f, 1.0f };
    ctx->ClearRenderTargetView(rtv, bgColor);
    ctx->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    D3D11_VIEWPORT vp = {};
    vp.Width = w;
    vp.Height = h;
    vp.MaxDepth = 1.0f;

    ctx->OMSetRenderTargets(1, &rtv, dsv);
    ctx->RSSetViewports(1, &vp);
}

void System_MapRenderer::ReleaseMapMesh()
{
    m_MapPass.Release();
}

void System_MapRenderer::ReleaseResources()
{
    m_MapPass.Release();
    m_DynamicPass.Release();
    m_ZonePass.Release();
    m_Pipeline.Release();
}