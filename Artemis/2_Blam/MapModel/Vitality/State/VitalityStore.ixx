export module Resolved.Vitality.State;

import Resolved.Vitality.Type;
import std;

export namespace Resolved::Vitality::State
{
    // Vitality layouts by model tag name. They are added while the map is built, then the store is frozen and only read.
    // note: Reads assert that the store is frozen, and Add asserts that it is not.
    class VitalityStore
    {
    private:
        using Vitality = Resolved::Vitality::Type::Vitality::Vitality;

    public:
        VitalityStore() = default;
        ~VitalityStore() = default;

        auto Has(const std::string& tagName) const -> bool;

        // return: Null if the model has no layout.
        auto Get(const std::string& tagName) const -> const Vitality*;

        // Same as Get, but shares ownership.
        // note: For data published to other threads. A snapshot keeps its layout alive after a Cleanup.
        // return: Null if the model has no layout.
        auto GetShared(const std::string& tagName) const -> std::shared_ptr<const Vitality>;

        auto Add(const std::string& tagName, Vitality layout) -> void;

        auto IsFrozen() const -> bool
        {
            return m_IsFrozen.load(std::memory_order_acquire);
        }

        // Ends the build. From now on the store can only be read.
        auto Freeze() -> void
        {
            m_IsFrozen.store(true, std::memory_order_release);
        }

        // Removes every layout and unfreezes the store.
        auto Cleanup() -> void;

    private:
        std::unordered_map<std::string, std::shared_ptr<const Vitality>> m_Vitalities{};
        std::atomic<bool> m_IsFrozen{ false };
    };
}