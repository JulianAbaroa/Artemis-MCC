#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct ModeVec3 { float X, Y, Z; };
struct ModeVec4 { float X, Y, Z, W; };

// An individual marker within a group.
struct ModeMarker
{
    // Index of the region and permutation to which this marker belongs.
    // -1 = applies to all.
    int8_t RegionIndex = -1;
    int8_t PermutationIndex = -1;

    // Bone to which it is attached (index in ModeGeometry::Nodes).
    int8_t  NodeIndex = -1;

    // Local-space transform.
    ModeVec3 Translation = {};
    ModeVec4 Rotation = {};  // quaternion
    float Scale = 1.0f;
};

// A group of markers with the same semantic name.
// Examples: "seat_driver", "right_hand", "gun", "engine".
struct ModeMarkerGroup
{
    uint32_t NameId;
    std::string Name;
    std::vector<ModeMarker> Markers;
};

// A node of the render skeleton.
struct ModeNode
{
    std::string Name;

    int16_t ParentIndex = -1;
    int16_t FirstChildIndex = -1;
    int16_t NextSiblingIndex = -1;

    // T-pose local-space transform.
    ModeVec3 DefaultTranslation = {};
    ModeVec4 DefaultRotation = {};  // quaternion
    float    InverseScale = 1.0f;
};

// Top-level.
struct ModeGeometry
{
    std::string TagName;

    // Marker groups, consumed by Interactable for seat positions,
    // weapon attach points, etc.
    std::vector<ModeMarkerGroup> MarkerGroups;

    // Render skeleton, required to resolve NodeIndex in markers.
    std::vector<ModeNode> Nodes;

    // AABB rendering in local space, derived from CompressionInfo.
    // Fallback when phmo is unavailable.
    ModeVec3 BoundsMin = {};
    ModeVec3 BoundsMax = {};
};