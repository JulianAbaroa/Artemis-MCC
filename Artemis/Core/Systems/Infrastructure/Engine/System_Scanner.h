#pragma once

#include "Core/Common/Scanner/Signatures.h"
#include <cstdint>

class System_Scanner
{
public:
	uintptr_t FindPattern(const Signature& sig, 
		const wchar_t* moduleName = L"haloreach.dll");

	uintptr_t FindPattern(const char* pattern, 
		const wchar_t* moduleName = L"haloreach.dll", 
		const char* name = "Unknown");

private:
	uintptr_t Scan(uintptr_t base, size_t size, 
		const char* pattern, const char* name = "Unknown");
};