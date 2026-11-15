export module Perception.Aim.State:TagVitalities;

import Blam.MapModel.Vitality.Type;
import std;

export namespace Perception::Aim::State
{
    // Vitality layout of each tag of the loaded map, found once per tag and kept by the datum index of the tag.
    // note: The datum index is unique per tag inside a loaded map. A null layout means the tag was looked up and has none.
    // Refresh, Find and Add run on the update thread. Cleanup runs on the lifecycle thread, so it only marks the store,
    // and the next Refresh empties it.
    class TagVitalitiesStore
    {
    private:
        using Vitality = Blam::MapModel::Vitality::Type::Vitality::Vitality;

    public:
        TagVitalitiesStore() = default;
        ~TagVitalitiesStore() = default;

        TagVitalitiesStore(const TagVitalitiesStore&) = delete;
        auto operator=(const TagVitalitiesStore&) -> TagVitalitiesStore& = delete;

        // Empties the store if a cleanup marked it. Called once per aim pass, before the Finds.
        auto Refresh() -> void
        {
            if (m_IsStale.exchange(false)) m_Layouts.clear();
        }

        // return: The stored layout (null when the tag has none), or null if the tag was not added since the last cleanup.
        auto Find(std::uint32_t datumIndex) const -> const std::shared_ptr<const Vitality>*
        {
            const auto it{ m_Layouts.find(datumIndex) };
            return it == m_Layouts.end() ? nullptr : &it->second;
        }

        // Keeps the layout of the tag, null when the tag has none.
        auto Add(std::uint32_t datumIndex, std::shared_ptr<const Vitality> layout) -> const std::shared_ptr<const Vitality>&
        {
            return m_Layouts.insert_or_assign(datumIndex, std::move(layout)).first->second;
        }

        // Drops every layout on the next Refresh.
        auto Cleanup() -> void
        {
            m_IsStale.store(true);
        }

    private:
        std::unordered_map<std::uint32_t, std::shared_ptr<const Vitality>> m_Layouts{};
        std::atomic<bool> m_IsStale{ false };
    };
}