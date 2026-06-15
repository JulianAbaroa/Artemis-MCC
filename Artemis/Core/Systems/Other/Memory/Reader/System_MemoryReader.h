#pragma once

#include <cstdint>
#include <array>

class System_MemoryReader
{
public:
	template<typename T>
	T Read(uintptr_t base, uintptr_t offset)
	{
		T value{};

		std::memcpy(&value, reinterpret_cast<void*>(
			base + offset), sizeof(T));

		return value;
	}

	template<typename T, size_t N>
	std::array<T, N> ReadArray(uintptr_t base, uintptr_t offset)
	{
		std::array<T, N> arr{};

		std::memcpy(arr.data(), reinterpret_cast<void*>(
			base + offset), sizeof(T) * N);

		return arr;
	}

	template<typename T>
	bool ReadRaw(uintptr_t address, T* dst, size_t bytes)
	{
		std::memcpy(dst, reinterpret_cast<const void*>(
			address), bytes);

		return true;
	}
};