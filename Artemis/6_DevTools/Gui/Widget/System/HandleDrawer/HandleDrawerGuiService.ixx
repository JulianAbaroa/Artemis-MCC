export module Gui.Widget.System:HandleDrawer;

import :CopyableField;

import std;

export namespace Gui::Widget::System
{
    // Draws a 32-bit object handle.
    class HandleDrawerGuiService
    {
    private:
        using CopyableFieldGuiService = Gui::Widget::System::CopyableFieldGuiService;

    public:
        HandleDrawerGuiService() = default;
        ~HandleDrawerGuiService() = default;

        // Draws the handle as a copyable hex field, or as disabled "none" if it is the invalid handle.
        // param ownerHandle: Passed to the copyable field to make its id unique.
        static auto DrawU32(const char* label, std::uint32_t handle,
            std::uint32_t ownerHandle, CopyableFieldGuiService& copyableField) -> void;
    };
}