export module Map.Reader.System:Formula;

import Map.Reader.Type;
import std;

export namespace Map::Reader::System
{
	class FormulaService
	{
	private:
		using HeaderInfo = Map::Reader::Type::Info::HeaderInfo;

	public:
		FormulaService() = default;
		~FormulaService() = default;

		auto Initialize(HeaderInfo headerInfo) -> void;
		auto Cleanup() -> void;

		auto Expand(std::uint32_t address) const -> std::int64_t;
		auto ToFileOffset(std::int64_t virtualAddress) const -> std::int64_t;
		auto ToDebugOffset(std::int64_t pointer) const -> std::int64_t;
		auto ToResourceOffset(std::int64_t blockOffset) const -> std::int64_t;

		auto ResolveFixupOffset(std::uint16_t address, std::uint16_t addressUpperBits, 
			std::uint8_t addressLocationHighBits) const -> std::uint32_t;

	private:
		HeaderInfo m_HeaderInfo;
	};
}