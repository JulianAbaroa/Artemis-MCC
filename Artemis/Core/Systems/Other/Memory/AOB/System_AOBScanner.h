#pragma once

#include <cstdint>

struct Signature;

class System_Logs;

struct Sys_AOBScanner_Deps
{
	System_Logs& System_Logs;
};

class System_AOBScanner
{
public:
	System_AOBScanner(Sys_AOBScanner_Deps deps) : m_Deps(deps) {}
	~System_AOBScanner() = default;

	uintptr_t FindPattern(const Signature& sig, 
		const wchar_t* moduleName = L"haloreach.dll");

	uintptr_t FindPattern(const char* pattern, 
		const wchar_t* moduleName = L"haloreach.dll", 
		const char* name = "Unknown");

private:
	Sys_AOBScanner_Deps m_Deps;

	uintptr_t Scan(uintptr_t base, size_t size, 
		const char* pattern, const char* name = "Unknown");
};