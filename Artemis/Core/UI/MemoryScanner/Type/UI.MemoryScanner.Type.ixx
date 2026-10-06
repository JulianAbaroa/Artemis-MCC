export module UI.MemoryScanner.Type;

import Platform.Memory.Type;
import std;

namespace
{
    using Mode = Platform::Memory::Type::Mode;
    using DataType = Platform::Memory::Type::DataType;
}

export namespace UI::MemoryScanner::Type
{
    // Size of the text buffers of the forms.
    inline constexpr std::size_t k_FieldSize{ 32 };

    // Inputs of the region scan.
    struct RegionForm
    {
        char Base[k_FieldSize]{ "0" };
        char Size[k_FieldSize]{};
        int DelayMs{ 500 };
    };

    // Inputs of a value scan.
    struct ScanForm
    {
        int ModeIndex{ 0 };
        int DataTypeIndex{ 0 };

        char ValueA[k_FieldSize]{ "0" };
        char ValueB[k_FieldSize]{ "0" };
        char BitMask[k_FieldSize]{ "0" };
        char BitPattern[k_FieldSize]{ "0" };
    };

    // Inputs of the result filter.
    struct FilterForm
    {
        char From[k_FieldSize]{ "0x0" };
        char To[k_FieldSize]{};

        bool ByBefore{ false };
        int BeforeValue{ 0x00 };

        bool ByAfter{ false };
        int AfterValue{ 0x01 };
    };

    // Scan mode shown in the mode selector with the inputs it needs.
    struct ModeEntry
    {
        const char* Label{};
        Mode Mode{};
        bool NeedsBefore{ false };
        bool NeedsValueA{ false };
        bool NeedsValueB{ false };
        bool NeedsBitMask{ false };
    };

    // Data type shown in the type selector.
    struct TypeEntry
    {
        const char* Label{};
        DataType Type{};
    };

    // Region size shown in the size selector.
    struct SizeEntry
    {
        const char* Label{};
        std::size_t Size{ 0 };
    };
}