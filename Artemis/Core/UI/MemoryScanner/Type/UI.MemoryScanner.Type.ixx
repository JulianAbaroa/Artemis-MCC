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
	inline constexpr std::size_t k_FieldSize = 32;

	struct RegionForm
	{
		char Base[k_FieldSize]{ "0" };
		char Size[k_FieldSize]{};
		int DelayMs{ 500 };
	};

	struct ScanForm
	{
		int ModeIndex{ 0 };
		int DataTypeIndex{ 0 };

		char ValueA[k_FieldSize]{ "0" };
		char ValueB[k_FieldSize]{ "0" };
		char BitMask[k_FieldSize]{ "0" };
		char BitPattern[k_FieldSize]{ "0" };
		int StabilizeRounds{ 3 };
	};

	struct FilterForm
	{
		char From[k_FieldSize]{ "0x0" };
		char To[k_FieldSize]{};

		bool ByBefore{ false };
		int BeforeValue{ 0x00 };

		bool ByAfter{ false };
		int AfterValue{ 0x01 };
	};

	struct ModeEntry
	{
		const char* Label;
		Mode Mode;
		bool NeedsBefore;
		bool NeedsValueA;
		bool NeedsValueB;
		bool NeedsBitMask;
		bool NeedsStabilize;
	};

	struct TypeEntry
	{
		const char* Label;
		DataType Type;
	};

	struct SizeEntry
	{
		const char* Label;
		std::size_t Size;
	};
}