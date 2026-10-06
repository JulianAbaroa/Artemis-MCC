export module Viewer.Hud.System:HealthLabels;

import :Canvas;
import Viewer.Hud.Type;
import std;

export namespace Viewer::Hud::System
{
    // Collects the labels of the health sections.
    class HealthLabels
    {
    private:
        using LabelContext = Viewer::Hud::Type::LabelContext;

    public:
        HealthLabels() = default;
        ~HealthLabels() = default;

        // Adds the health and shield of the vitality sections to the canvas.
        static auto Collect(LabelCanvas& canvas, const LabelContext& context) -> void;
    };
}