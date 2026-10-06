export module Viewer.Hud.System:VehicleLabels;

import :Canvas;
import Viewer.Hud.Type;
import std;

export namespace Viewer::Hud::System
{
    // Collects the labels of the vehicles.
    class VehicleLabels
    {
    private:
        using LabelContext = Viewer::Hud::Type::LabelContext;

    public:
        VehicleLabels() = default;
        ~VehicleLabels() = default;

        // Adds a marker for every seat and hijacker slot of the vehicles to the canvas.
        static auto Collect(LabelCanvas& canvas, const LabelContext& context) -> void;
    };
}