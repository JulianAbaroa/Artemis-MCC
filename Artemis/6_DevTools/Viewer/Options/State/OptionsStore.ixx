export module DevTools.Viewer.Options.State;

import DevTools.Viewer.Options.Type;
import std;

namespace
{
    using DevTools::Viewer::Options::Type::k_FlagCount;
    using DevTools::Viewer::Options::Type::k_LayerCount;
    using DevTools::Viewer::Options::Type::k_ScalarCount;
}

export namespace DevTools::Viewer::Options::State
{
    // Holds the value of every viewer option: the level of each layer, the flags that belong to no layer and the scalars.
    // The flags of a layer are not stored, IsEnabled derives them from the level of the layer (IsFlagOn).
    // The values are atomic so they can be read and written from different threads without locks.
    class OptionsStore
    {
    private:
        using Flag = DevTools::Viewer::Options::Type::Flag;
        using Layer = DevTools::Viewer::Options::Type::Layer;
        using Level = DevTools::Viewer::Options::Type::Level;
        using Scalar = DevTools::Viewer::Options::Type::Scalar;

    public:
        // Prefix of the keys of this store in the preferences file.
        static constexpr std::string_view k_PreferencePrefix{ "Viewer_" };

        // Starts with the default values.
        OptionsStore();
        ~OptionsStore() = default;

        OptionsStore(const OptionsStore&) = delete;
        auto operator=(const OptionsStore&) -> OptionsStore& = delete;

        // return: Whether the flag is on: derived from the level of its layer, or its own value when it has no layer.
        auto IsEnabled(Flag flag) const -> bool;

        // note: Only for the flags that belong to no layer. On a flag of a layer it does nothing, the level of the layer decides (SetLevel).
        auto SetEnabled(Flag flag, bool value) -> void;

        // note: Same restriction as SetEnabled.
        auto ToggleEnabled(Flag flag) -> void;

        auto GetLevel(Layer layer) const -> Level;
        auto SetLevel(Layer layer, Level level) -> void;

        auto GetScalar(Scalar scalar) const -> float;

        // note: The value is clamped to the range of the scalar.
        auto SetScalar(Scalar scalar, float value) -> void;

        auto ResetToDefaults() -> void;

        // Writes every option as a "<prefix><key>=<value>" line: the level of each layer (0, 1 or 2), the flags without a layer and the scalars.
        auto Save(std::ostream& stream) const -> void;

        // Applies one saved option. Ignores unknown keys, which include the keys the flags of a layer were saved with before the layers.
        // note: A scalar that cannot be parsed falls back to its default, and so does a level that is not 0, 1 or 2.
        auto Load(std::string_view key, std::string_view value) -> void;

    private:
        std::array<std::atomic<Level>, k_LayerCount> m_Levels{};
        std::array<std::atomic<bool>, k_FlagCount> m_Flags{};
        std::array<std::atomic<float>, k_ScalarCount> m_Scalars{};
    };
}