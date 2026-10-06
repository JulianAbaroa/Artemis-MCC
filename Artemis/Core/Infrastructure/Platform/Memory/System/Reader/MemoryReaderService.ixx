export module Platform.Memory.System:Reader;

import std;

export namespace Platform::Memory::System
{
    // Reads game memory safely. A failed read (access violation) never crashes.
    class MemoryReaderService
    {
    public:
        // return: The value at base + offset, or a zero-initialized value if the read fails.
        template<typename T>
        auto Read(std::uintptr_t base, std::uintptr_t offset) -> T
        {
            T value{};

            this->CopyFromGame(base + offset, &value, sizeof(T));

            return value;
        }

        // return: The N values at base + offset, zero-initialized if the read fails.
        template<typename T, std::size_t N>
        auto ReadArray(std::uintptr_t base, std::uintptr_t offset) -> std::array<T, N>
        {
            std::array<T, N> array{};

            this->CopyFromGame(base + offset, array.data(), sizeof(T) * N);

            return array;
        }

        // Copies bytes from address into destination.
        // return: False if the read fails.
        template<typename T>
        auto ReadRaw(std::uintptr_t address, T* destination, std::size_t bytes) -> bool
        {
            return this->CopyFromGame(address, destination, bytes);
        }

    private:
        auto CopyFromGame(std::uintptr_t address, void* destination, std::size_t bytes) -> bool;
    };
}