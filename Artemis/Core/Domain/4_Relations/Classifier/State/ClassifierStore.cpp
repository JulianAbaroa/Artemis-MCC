module Relations.Classifier.State;

namespace Relations::Classifier::State
{
    void ClassifierStore::Publish(Classifieds classifieds)
    {
        auto snap = std::make_shared<const Classifieds>(std::move(classifieds));
        m_pClassifieds.store(snap, std::memory_order_release);
    }

    auto ClassifierStore::Acquire() const -> std::shared_ptr<const Classifieds>
    {
        return m_pClassifieds.load(std::memory_order_acquire);
    }

    void ClassifierStore::Cleanup()
    {
        m_pClassifieds.store(nullptr, std::memory_order_release);
    }
}