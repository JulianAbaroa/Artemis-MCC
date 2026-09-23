export module Relations.Classifier.State;

import Relations.Classifier.Type;
import std;

export namespace Relations::Classifier::State
{
    class ClassifierStore
    {
    private:
        using Classified = Relations::Classifier::Type::Classified;
        using Classifieds = std::vector<Classified>;

    public:
        ClassifierStore() = default;
        ~ClassifierStore() = default;

        auto Publish(Classifieds classifieds) -> void;
        auto Acquire() const -> std::shared_ptr<const Classifieds>;

        auto Cleanup() -> void;

    private:
        std::atomic<std::shared_ptr<const Classifieds>> m_pClassifieds;
    };
}