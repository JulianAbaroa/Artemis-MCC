#pragma once

#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <memory>
#include <vector>

struct Tick;

struct RgbColor { float r, g, b; };

class CollidableClassifier
{
public:
    void Build(const std::shared_ptr<const Tick>& tick);

    RgbColor ColorFor(uint32_t handle) const;

private:
    std::unordered_set<uint32_t> m_VitalityHandles;
    std::unordered_set<uint32_t> m_AffordanceHandles;
    std::unordered_map<uint32_t, RgbColor> m_FixtureColors;
};