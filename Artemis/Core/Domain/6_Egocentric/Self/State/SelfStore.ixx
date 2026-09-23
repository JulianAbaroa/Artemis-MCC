export module Egocentric.Self.State;

import Egocentric.Self.Type;
import std;

export namespace Egocentric::Self::State
{
    class SelfStore
    {
    private:
        using Self = Egocentric::Self::Type::Self;

    public:
        SelfStore() = default;
        ~SelfStore() = default;

        auto Publish(Self self) -> void;
        auto Acquire() const -> std::shared_ptr<const Self>;

        void Cleanup();

    private:
        std::atomic<std::shared_ptr<const Self>> m_pSelf;
    };
}