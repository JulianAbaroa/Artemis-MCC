export module Viewer.Hud.System:FixtureLabels;

import :Canvas;
import Viewer.Hud.Type;
import std;

export namespace Viewer::Hud::System
{
    // Collects the labels of the fixtures.
    class FixtureLabels
    {
    private:
        using LabelContext = Viewer::Hud::Type::LabelContext;

    public:
        FixtureLabels() = default;
        ~FixtureLabels() = default;

        // Adds the labels of the fixtures of the map to the canvas.
        static auto Collect(LabelCanvas& canvas, const LabelContext& context) -> void;
    };
}