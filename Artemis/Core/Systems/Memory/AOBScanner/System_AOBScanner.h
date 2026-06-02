#pragma once

#include <cstdint>

struct Signature;

class System_Logs;

class System_AOBScanner
{
public:
	System_AOBScanner(System_Logs& systemLogs) : 
		System_Logs(systemLogs) {}
	~System_AOBScanner() = default;

	uintptr_t FindPattern(const Signature& sig, 
		const wchar_t* moduleName = L"haloreach.dll");

	uintptr_t FindPattern(const char* pattern, 
		const wchar_t* moduleName = L"haloreach.dll", 
		const char* name = "Unknown");

private:
	System_Logs& System_Logs;

	uintptr_t Scan(uintptr_t base, size_t size, 
		const char* pattern, const char* name = "Unknown");
};