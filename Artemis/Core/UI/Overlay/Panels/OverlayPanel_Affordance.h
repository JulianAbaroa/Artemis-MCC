#pragma once

#include <cstdint>
#include <memory>

struct Tick;

namespace OverlayPanel_Affordance
{
    void Draw(const std::shared_ptr<const Tick>& tick, uint32_t handle);
}