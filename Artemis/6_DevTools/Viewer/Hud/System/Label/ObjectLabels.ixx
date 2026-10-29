export module DevTools.Viewer.Hud.System:ObjectLabels;

import :Canvas;

import DevTools.Viewer.Hud.Type;
import std;

export namespace DevTools::Viewer::Hud::System
{
    // Collects the labels of the objects.
    class ObjectLabels
    {
    private:
        using LabelContext = DevTools::Viewer::Hud::Type::LabelContext;

    public:
        ObjectLabels() = default;
        ~ObjectLabels() = default;

        // Adds the name, speed and seats of every selected kind of object to the canvas.
        static auto Collect(LabelCanvas& canvas, const LabelContext& context) -> void;
    };
}