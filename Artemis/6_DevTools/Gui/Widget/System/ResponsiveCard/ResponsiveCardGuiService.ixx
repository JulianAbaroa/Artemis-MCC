export module DevTools.Gui.Widget.System:ResponsiveCard;

import std;

export namespace DevTools::Gui::Widget::System
{
    // Layout helper of the card grids (wrapping of the cards with the window).
    class ResponsiveCardGuiService
    {
    public:
        ResponsiveCardGuiService() = default;
        ~ResponsiveCardGuiService() = default;

        // Tells if a card of nextWidth fits on the line of the last item, so the cards wrap with the window.
        // param windowRightEdge: Right edge of the window in screen coordinates.
        // note: Reads the last item, so call it right after drawing a card.
        static auto FitsOnSameLine(float nextWidth, float spacing, float windowRightEdge) -> bool;
    };
}