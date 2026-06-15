#pragma once

#include "Core/Types/Environment/Collidable/Collidable.h"

#include <atomic>
#include <memory>

using Collidables = std::vector<Collidable>;

class State_Collidables
{
public:
    void Publish(Collidables instances);
    std::shared_ptr<const Collidables> Acquire() const;
    
    void Cleanup();

private:
    std::atomic<std::shared_ptr<const Collidables>> m_pCollidables;
};