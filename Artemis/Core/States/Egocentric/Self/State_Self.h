#pragma once

#include "Core/Types/Egocentric/Self/Self.h"

#include <atomic>
#include <memory>

class State_Self
{
public:
    void Publish(Self self);
    std::shared_ptr<const Self> Acquire() const;

    void Cleanup();

private:
    std::atomic<std::shared_ptr<const Self>> m_pSelf;
};