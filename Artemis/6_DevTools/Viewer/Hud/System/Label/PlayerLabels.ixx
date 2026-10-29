export module DevTools.Viewer.Hud.System:PlayerLabels;

import :Canvas;

import Perception.Entities.Type;
import Perception.Players.Type;
import DevTools.Viewer.Hud.Type;
import std;

export namespace DevTools::Viewer::Hud::System
{
    // Collects the labels of the players.
    class PlayerLabels
    {
    private:
        using LabelContext = DevTools::Viewer::Hud::Type::LabelContext;

    public:
        PlayerLabels() = default;
        ~PlayerLabels() = default;

        // Adds the gamertag, vitality, weapons and distance of every live player to the canvas.
        static auto Collect(LabelCanvas& canvas, const LabelContext& context) -> void;
    };
}