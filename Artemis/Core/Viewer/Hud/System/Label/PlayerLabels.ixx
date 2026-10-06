export module Viewer.Hud.System:PlayerLabels;

import :Canvas;
import Viewer.Hud.Type;
import std;

export namespace Viewer::Hud::System
{
    // Collects the labels of the players.
    class PlayerLabels
    {
    private:
        using LabelContext = Viewer::Hud::Type::LabelContext;

    public:
        PlayerLabels() = default;
        ~PlayerLabels() = default;

        // Adds the gamertag, weapons and distance of every player to the canvas.
        static auto Collect(LabelCanvas& canvas, const LabelContext& context) -> void;
    };
}