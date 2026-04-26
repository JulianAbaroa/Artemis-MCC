#include "pch.h"
#include "Core/States/Domain/Interactable/InteractableState.h"

std::vector<AIInteractable> InteractableState::GetInteractables() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Interactables;
}

void InteractableState::SetInteractables(std::vector<AIInteractable> interactables)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Interactables = std::move(interactables);
}


void InteractableState::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Interactables.clear();
}