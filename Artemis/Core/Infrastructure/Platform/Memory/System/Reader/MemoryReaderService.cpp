module;

#include <windows.h>

module Platform.Memory.System;
import :Reader;

namespace Platform::Memory::System
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