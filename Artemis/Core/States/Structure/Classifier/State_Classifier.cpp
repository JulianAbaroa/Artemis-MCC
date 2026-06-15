#include "pch.h"

#include "State_Classifier.h"

void State_Classifier::Publish(Classifieds classifieds)
{
    auto snap = std::make_shared<const Classifieds>(std::move(classifieds));
    m_pClassifieds.store(snap, std::memory_order_release);
}

std::shared_ptr<const Classifieds> State_Classifier::Acquire() const
{
    return m_pClassifieds.load(std::memory_order_acquire);
}

void State_Classifier::Cleanup()
{
    m_pClassifieds.store(nullptr, std::memory_order_release);
}