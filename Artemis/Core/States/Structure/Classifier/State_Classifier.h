#pragma once

#include "Core/Types/Structure/Classified/Classified.h"

#include <memory>
#include <vector>
#include <atomic>

using Classifieds = std::vector<Classified>;

class State_Classifier
{
public:
    void Publish(Classifieds classifieds);
    std::shared_ptr<const Classifieds> Acquire() const;

    void Cleanup();

private:
    std::atomic<std::shared_ptr<const Classifieds>> m_pClassifieds;
};