module;

#include <windows.h>

module Engine.Base.Memory.System;
import :Reader;

namespace Engine::Base::Memory::System
{
    auto MemoryReaderService::CopyFromGame(std::uintptr_t address, void* destination,
        std::size_t bytes) -> bool
    {
        __try
        {
            std::memcpy(destination, reinterpret_cast<const void*>(address), bytes);
            return true;
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
            ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            return false;
        }
    }
}