#pragma once

#include <string>

enum class LogLevel
{
	Default,
	Info,
	Warning,
	Error
};

struct LogEntry
{
	std::string FullText{};
	std::string Timestamp{};
	std::string Tag{};
	std::string MessagePrefix{};
	std::string Message{};
	LogLevel Level{};
};