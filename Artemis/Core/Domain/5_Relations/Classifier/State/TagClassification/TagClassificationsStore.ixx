export module Relations.Classifier.State:TagClassifications;

import Relations.Classifier.Type;
import std;

export namespace Relations::Classifier::State
{
    // Classification of each tag of the loaded map, by group and tag name.
    // note: Find and Add run on the update thread. Cleanup runs on the lifecycle thread, so it only marks the store, and the next Find empties it.
    class TagClassificationsStore
    {
    private:
        using TagClassification = Relations::Classifier::Type::TagClassification;
        using TagClassifications = std::unordered_map<std::string, TagClassification>;

    public:
        TagClassificationsStore() = default;
        ~TagClassificationsStore() = default;

        TagClassificationsStore(const TagClassificationsStore&) = delete;
        auto operator=(const TagClassificationsStore&) -> TagClassificationsStore& = delete;

        // return: The classification of the tag, or null if it was not added since the last cleanup.
        auto Find(std::string_view fourCC, std::string_view tagName) -> const TagClassification*;

        // Keeps the classification of the tag.
        auto Add(std::string_view fourCC, std::string_view tagName, TagClassification classification) -> void;

        // Returns every classification, by group and tag name joined with '|'.
        auto GetAll() const -> const TagClassifications&;

        // Checks whether the summary of the map was logged.
        auto IsSummaryLogged() const -> bool;
        auto SetSummaryLogged(bool value) -> void;

        // Drops every classification and the summary flag on the next Find.
        auto Cleanup() -> void;

    private:
        TagClassifications m_Classifications{};
        bool m_IsSummaryLogged{ false };
        std::atomic<bool> m_IsStale{ false };
    };
}