#pragma once

#include "Core/Types/Egocentric/Affordance/Affordance.h"

#include <memory>
#include <atomic>

using Affordances = std::vector<Affordance>;

class State_Affordances
{
public:
    void Publish(Affordances interactables);
    std::shared_ptr<const Affordances> Acquire() const;

    void Cleanup();

private:
    std::atomic<std::shared_ptr<const Affordances>> m_pAffordances;
};