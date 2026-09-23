export module Service.Logs.Type;

import std;

export namespace Service::Logs::Type
{
	enum class Level
	{
		Default,
		Info,
		Warning,
		Error
	};

	struct Entry
	{
		std::string FullText{};
		std::string Timestamp{};
		std::string Tag{};
		std::string MessagePrefix{};
		std::string Message{};
		Level Level{};
	};
}