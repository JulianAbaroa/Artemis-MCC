export module DevTools.Viewer.Hud.System:ObservedView;

import Views.View.Type;
import std;

export namespace DevTools::Viewer::Hud::System
{
    // Draws the panel of the View of the watched player: its head, what it knows of each other player and the two flags.
    class ViewHud
    {
    private:
        using View = Views::View::Type::View;

    public:
        ViewHud() = default;
        ~ViewHud() = default;

        // param view: The View of the watched player. Null shows that there is none.
        static auto Draw(const View* view) -> void;
    };
}