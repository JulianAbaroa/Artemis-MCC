module UI.Format.System;
import :Team;

namespace UI::Format::System
{
	auto TeamFormater::TeamToString(Team team) -> const char*
	{
		switch (team)
		{
		case Team::Red:		return "Red";
		case Team::Blue:	return "Blue";
		case Team::Green:	return "Green";
		case Team::Orange:	return "Orange";
		case Team::Purple:	return "Purple";
		case Team::Gold:	return "Gold";
		case Team::Brown:	return "Brown";
		case Team::Pink:	return "Pink";
		case Team::Neutral: return "Neutral";
		default:			return "Unknown";
		}
	}
}