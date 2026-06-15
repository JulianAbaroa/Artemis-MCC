#pragma once

#include "Core/Types/Environment/Fixtures/Fixtures.h"

#include <atomic>
#include <memory>

class State_Fixtures
{
public:
    void Publish(Fixtures data);
    std::shared_ptr<const Fixtures> Acquire() const;

    void Cleanup();

private:
    std::atomic<std::shared_ptr<const Fixtures>> m_pFixtures;
};