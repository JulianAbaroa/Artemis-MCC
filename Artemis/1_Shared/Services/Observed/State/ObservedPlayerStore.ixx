export module Services.Observed.State;

import std;

export namespace Services::Observed::State
{
    // The player whose view the user is watching: the one of the object selected in the Viewer. The AI thread adds it to the agents that get a View (D307).
    // note: Atomic: the render thread writes it and the AI thread reads it. No player is 0xFFFFFFFF.
    class ObservedPlayerStore
    {
    public:
        ObservedPlayerStore() = default;
        ~ObservedPlayerStore() = default;

        ObservedPlayerStore(const ObservedPlayerStore&) = delete;
        auto operator=(const ObservedPlayerStore&) -> ObservedPlayerStore& = delete;

        // return: The handle of the observed player, or 0xFFFFFFFF when nobody is observed.
        auto Get() const -> std::uint32_t
        {
            return m_PlayerHandle.load(std::memory_order_relaxed);
        }

        // Sets the observed player. Pass 0xFFFFFFFF for none.
        auto Set(std::uint32_t playerHandle) -> void
        {
            m_PlayerHandle.store(playerHandle, std::memory_order_relaxed);
        }

    private:
        std::atomic<std::uint32_t> m_PlayerHandle{ 0xFFFFFFFFu };
    };
}