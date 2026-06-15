#pragma once

#include "Core/Types/Tick/Tick.h"

#include <atomic>

class State_Tick
{
public:
    void Publish(Tick tick);
    std::shared_ptr<const Tick> Acquire() const;

    void Cleanup();

private:
    std::atomic<std::shared_ptr<const Tick>> m_pTick;
};