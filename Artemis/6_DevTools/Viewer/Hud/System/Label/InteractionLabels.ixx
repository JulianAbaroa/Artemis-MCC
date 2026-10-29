export module DevTools.Viewer.Hud.System:InteractionLabels;

import :Canvas;

import DevTools.Viewer.Hud.Type;
import std;

export namespace DevTools::Viewer::Hud::System
{
    // Collects the labels of the interactions.
    class InteractionLabels
    {
    private:
        using LabelContext = DevTools::Viewer::Hud::Type::LabelContext;

    public:
        InteractionLabels() = default;
        ~InteractionLabels() = default;

        // Adds the affordances between the players and their targets to the canvas.
        static auto Collect(LabelCanvas& canvas, const LabelContext& context) -> void;
    };
}