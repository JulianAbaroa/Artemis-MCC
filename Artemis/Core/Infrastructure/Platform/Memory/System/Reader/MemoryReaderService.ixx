export module Platform.Memory.System:Reader;

import std;

export namespace Platform::Memory::System
{
	class MemoryReaderService
	{
	public:
		template<typename T>
		auto Read(std::uintptr_t base, std::uintptr_t offset) -> T
		{
			T value{};

			this->CopyFromGame(base + offset, &value, sizeof(T));

			return value;
		}

		template<typename T, std::size_t N>
		auto ReadArray(std::uintptr_t base, std::uintptr_t offset) -> std::array<T, N>
		{
			std::array<T, N> arr{};

			this->CopyFromGame(base + offset, arr.data(), sizeof(T) * N);

			return arr;
		}

		template<typename T>
		auto ReadRaw(std::uintptr_t address, T* dst, std::size_t bytes) -> bool
		{
			return this->CopyFromGame(address, dst, bytes);
		}

	private:
		auto CopyFromGame(std::uintptr_t address, void* destination,
			std::size_t bytes) -> bool;
	};
}