export module Platform.Memory.System:AOB;

import Service.Logs.System;
import Platform.Memory.Type;
import std;

export namespace Platform::Memory::System
{
	class AOBService
	{
	private:
		using Signature = Platform::Memory::Type::Signature::Signature;

		using LogsService = Service::Logs::System::LogsService;

	public:
		AOBService(LogsService& logsService) : m_LogsService(logsService) {}
		~AOBService() = default;

		std::uintptr_t FindPattern(const Signature& sig,
			const wchar_t* moduleName = L"haloreach.dll");

		std::uintptr_t FindPattern(const char* pattern,
			const wchar_t* moduleName = L"haloreach.dll",
			const char* name = "Unknown");

	private:
		LogsService& m_LogsService;

		auto Scan(std::uintptr_t base, std::size_t size, const char* pattern, 
			const char* name = "Unknown") -> std::uintptr_t;
	};
}