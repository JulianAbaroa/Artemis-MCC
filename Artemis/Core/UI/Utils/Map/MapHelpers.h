#pragma once

#include "Core/UI/Utils/Map/MapTypes.h"
#include "Core/Types/Environment/PhmoGeometry.h"

#include "External/imgui/imgui.h"

#include <vector>
#include <array>

class UI_MapHelpers
{
public:
    // --- Coordinate transform ---

    static ImVec2 ToCanvas(float wx, float wy, const MapTransform& transform);

    // --- Math ---

    static std::array<float, 3> Normalize(const std::array<float, 3>& v);

    static std::array<float, 3> Cross(const std::array<float, 3>& a,
        const std::array<float, 3>& b);

    // --- Convex hull ---

    static std::vector<int> ComputeConvexHull(const std::vector<ImVec2>& pts);

    static void DrawConvexHullFilled(ImDrawList* draw,
        const std::vector<ImVec2>& pts, ImU32 color);

    static void DrawConvexHullDashed(ImDrawList* draw,
        const std::vector<ImVec2>& pts, ImU32 color,
        float thickness = 1.5f);

    // --- Phmo drawing ---

    static void DrawPhmoRigidBody(ImDrawList* draw,
        const PhmoRigidBody& rb,
        const std::array<float, 3>& position,
        const std::array<float, 3>& forward,
        const std::array<float, 3>& up,
        const MapTransform& transform,
        ImU32 color, ImU32 fillColor);
};