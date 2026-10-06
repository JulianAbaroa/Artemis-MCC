export module Viewer.Hud.System:ObjectLabels;

import :Canvas;
import Viewer.Hud.Type;
import std;

export namespace Viewer::Hud::System
{
    // Collects the labels of the objects.
    class ObjectLabels
    {
    private:
        using LabelContext = Viewer::Hud::Type::LabelContext;

    public:
        ObjectLabels() = default;
        ~ObjectLabels() = default;

        // Adds the name, speed and seats of every selected kind of object to the canvas.
        static auto Collect(LabelCanvas& canvas, const LabelContext& context) -> void;
    };
}