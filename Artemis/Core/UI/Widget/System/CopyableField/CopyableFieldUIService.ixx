export module UI.Widget.System:CopyableField;

import std;

export namespace UI::Widget::System
{
    class CopyableFieldUIService
    {
    public:
        CopyableFieldUIService() = default;
        ~CopyableFieldUIService() = default;

        auto Draw(const char* label, const std::string& value, 
            std::uint32_t ownerHandle) -> void;

    private:
        std::string m_AnimateCopyLabel = "";
        float m_AnimationStartTime = 0.0f;
        float m_AnimationDuration = 0.6f;
    };
}