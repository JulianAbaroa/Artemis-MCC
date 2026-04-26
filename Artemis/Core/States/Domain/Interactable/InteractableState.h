#pragma once

#include "Core/Common/Types/Domain/Interactable/InteractableTypes.h"
#include <vector>
#include <mutex>

// Holds the interactable list produced by InteractableSystem each tick.
// Los thresholds y prioridades de TagGroup viven en InteractableSystem,
// que es el único consumidor de esa lógica.
class InteractableState
{
public:
    std::vector<AIInteractable> GetInteractables() const;
    void SetInteractables(std::vector<AIInteractable> interactables);

    void Cleanup();

private:
    std::vector<AIInteractable> m_Interactables;
    mutable std::mutex m_Mutex;
};