export module Export.Tick.State;

import Export.Tick.Type;
import std;

export namespace Export::Tick::State
{
    class TickStore
    {
    private:
        using Tick = Export::Tick::Type::Tick;

    public:
        TickStore() = default;
        ~TickStore() = default;

        auto Publish(Tick tick) -> void;
        auto Acquire() const -> std::shared_ptr<const Tick>;

        auto Cleanup() -> void;

    private:
        std::atomic<std::shared_ptr<const Tick>> m_pTick;
    };
}