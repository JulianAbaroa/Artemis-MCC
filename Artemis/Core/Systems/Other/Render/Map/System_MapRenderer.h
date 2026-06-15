#pragma once

#include "../FlyCamera.h"
#include "../GpuPipeline.h"
#include "CollidableClassifier.h"
#include "MapPass.h"
#include "DynamicPass.h"
#include "ZonePass.h"

#include <d3d11.h>
#include <memory>

struct Tick;

class State_WorldBuilder;
class State_Render;
class State_FlyCamera;
class State_Selection;
class System_Logs;

struct Sys_MapRenderer_Deps
{
    State_WorldBuilder& State_WorldBuilder;
    State_Render& State_Render;
    State_FlyCamera& State_FlyCamera;
    State_Selection& State_Selection;
    System_Logs& System_Logs;
};

class System_MapRenderer
{
public:
    System_MapRenderer(Sys_MapRenderer_Deps deps) : m_Deps(deps) {}
    ~System_MapRenderer() = default;

    void DrawFrame(std::shared_ptr<const Tick> tick);

    void ReleaseMapMesh();
    void ReleaseResources();

private:
    Sys_MapRenderer_Deps m_Deps;

    FlyCamera m_Camera;
    bool m_PrevCameraActive = false;

    GpuPipeline m_Pipeline;
    MapPass m_MapPass;
    DynamicPass m_DynamicPass;
    ZonePass m_ZonePass;
    CollidableClassifier m_Classifier;

    void UpdateCamera(const std::shared_ptr<const Tick>& tick,
        float canvasW, float canvasH);

    void PerformPick();

    void SetupFrame(ID3D11DeviceContext* ctx, float canvasW, float canvasH);
};