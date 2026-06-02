#pragma once

#include "Core/Types/Classified/Classified.h"

#include <vector>
#include <mutex>

class State_Classifier
{
public:
    const std::vector<Classified>& GetClassifieds() const;
    void SetClassifieds(std::vector<Classified> classifieds);

    void Cleanup();

private:
    std::vector<Classified> m_Objects;
    mutable std::mutex m_Mutex;
};