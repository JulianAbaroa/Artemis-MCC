export module DevTools.Viewer.Hud.System:ObsLabels;

import :Canvas;

import DevTools.Viewer.Hud.Type;
import std;

export namespace DevTools::Viewer::Hud::System
{
    // Collects the distances of the observation arrows of the agents (VW.17, Full level).
    class ObsLabels
    {
    private:
        using LabelContext = DevTools::Viewer::Hud::Type::LabelContext;

    public:
        ObsLabels() = default;
        ~ObsLabels() = default;

        // Adds, near the tip of each arrow that ObsPass draws, its length in world units in the color of the arrow.
        // param rows: The rows of Players (and of the ViewTable) drawn, only agents (VW.17a).
        // param arrowLength: The length ObsPass draws the arrows with (0 for none). The label is at the tip of the drawn arrow and shows the real distance.
        // note: The length is the one the View sees (from the feet to the flag or the stand), with no interpolation offset.
        static auto Collect(LabelCanvas& canvas, const LabelContext& context, const std::vector<std::size_t>& rows, float arrowLength) -> void;
    };
}