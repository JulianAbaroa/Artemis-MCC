export module DevTools.Viewer.Inspector.System:View;

import Views.View.Type;
import std;

export namespace DevTools::Viewer::Inspector::System
{
    // Draws the View section: the head of the View of the selected player, what it knows of each other player and the two flags.
    class ViewSection
    {
    private:
        using View = Views::View::Type::View;

    public:
        ViewSection() = default;
        ~ViewSection() = default;

        // param view: The View of the player of the selected biped. Null shows that there is none.
        static auto Draw(const View* view) -> void;
    };
}