export module Perception.Classifier.State:TagClassifications;

import Perception.Classifier.Type;
import std;

export namespace Perception::Classifier::State
{
    // Classification of each tag of the loaded map, by the datum index of the tag.
    // note: The datum index is unique per tag inside a loaded map, so it stands for group and name. Find, Add and Refresh run on the update thread.
    // Cleanup runs on the lifecycle thread, so it only marks the store, and the next Refresh empties it.
    class TagClassificationsStore
    {
    private:
        using TagClassification = Perception::Classifier::Type::TagClassification;
        using TagClassifications = Perception::Classifier::Type::TagClassifications;

    public:
        TagClassificationsStore() = default;
        ~TagClassificationsStore() = default;

        TagClassificationsStore(const TagClassificationsStore&) = delete;
        auto operator=(const TagClassificationsStore&) -> TagClassificationsStore& = delete;

        // Empties the store if a cleanup marked it. Called once per classification pass, before the Finds.
        auto Refresh() -> void;

        // return: The classification of the tag, or null if it was not added since the last cleanup.
        auto Find(std::uint32_t datumIndex) const -> const TagClassification*;

        // Keeps the classification of the tag.
        auto Add(std::uint32_t datumIndex, TagClassification classification) -> void;

        // Returns every classification, by datum index.
        auto GetAll() const -> const TagClassifications&;

        // Checks whether the summary of the map was logged.
        auto IsSummaryLogged() const -> bool;
        auto SetSummaryLogged(bool value) -> void;

        // Drops every classification and the summary flag on the next Refresh.
        auto Cleanup() -> void;

    private:
        TagClassifications m_Classifications{};
        bool m_IsSummaryLogged{ false };
        std::atomic<bool> m_IsStale{ false };
    };
}