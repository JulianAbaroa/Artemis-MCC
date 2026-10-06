export module Viewer.Options.State;

import Viewer.Options.Type;
import std;

namespace
{
    using Viewer::Options::Type::k_FlagCount;
    using Viewer::Options::Type::k_ScalarCount;
}

export namespace Viewer::Options::State
{
    // Holds the value of every viewer option, indexed by Flag and Scalar.
    // The values are atomic so they can be read and written from different threads without locks.
    class OptionsStore
    {
    private:
        using Flag = Viewer::Options::Type::Flag;
        using Scalar = Viewer::Options::Type::Scalar;

    public:
        // Prefix of the keys of this store in the preferences file.
        static constexpr std::string_view k_PreferencePrefix{ "Viewer_" };

        // Starts with the default values.
        OptionsStore();
        ~OptionsStore() = default;

        OptionsStore(const OptionsStore&) = delete;
        auto operator=(const OptionsStore&) -> OptionsStore& = delete;

        auto IsEnabled(Flag flag) const -> bool;
        auto SetEnabled(Flag flag, bool value) -> void;
        auto ToggleEnabled(Flag flag) -> void;

        auto GetScalar(Scalar scalar) const -> float;

        // note: The value is clamped to the range of the scalar.
        auto SetScalar(Scalar scalar, float value) -> void;

        auto ResetToDefaults() -> void;

        // Writes every option as a "<prefix><key>=<value>" line.
        auto Save(std::ostream& stream) const -> void;

        // Applies one saved option. Ignores unknown keys.
        // note: A scalar that cannot be parsed falls back to its default.
        auto Load(std::string_view key, std::string_view value) -> void;

    private:
        std::array<std::atomic<bool>, k_FlagCount> m_Flags{};
        std::array<std::atomic<float>, k_ScalarCount> m_Scalars{};
    };
}