export module DevTools.Viewer.Hud.System:HealthLabels;

import :Canvas;

import DevTools.Viewer.Hud.Type;
import std;

export namespace DevTools::Viewer::Hud::System
{
    // Collects the labels of the health sections.
    class HealthLabels
    {
    private:
        using LabelContext = DevTools::Viewer::Hud::Type::LabelContext;

    public:
        HealthLabels() = default;
        ~HealthLabels() = default;

        // Adds the health and shield of the vitality sections to the canvas.
        static auto Collect(LabelCanvas& canvas, const LabelContext& context) -> void;
    };
}