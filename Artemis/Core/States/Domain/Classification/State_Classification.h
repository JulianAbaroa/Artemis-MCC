#pragma once

// Types.
#include "Core/Types/Domain/Classification/ClassifiedObject.h"

#include <vector>
#include <mutex>

class State_Classification
{
public:
    const std::vector<ClassifiedObject>& GetObjects() const;
    void SetObjects(std::vector<ClassifiedObject> objects);

    void Cleanup();

private:
    std::vector<ClassifiedObject> m_Objects;
    mutable std::mutex m_Mutex;
};