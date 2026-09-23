export module UI.Logs.Type;

import std;

export namespace UI::Logs::Type
{
	struct FilterState
	{
		std::string LowerQuery{};
		bool IsFiltering{ false };
	};
}