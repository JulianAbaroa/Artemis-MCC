#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Legacy/EventRegistryState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Legacy/EventRegistrySystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/FormatSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include <fstream>

EventRegistrySystem::EventRegistrySystem()
{
	this->InitializeDefaultRegistry();
}

void EventRegistrySystem::SaveEventRegistry()
{
	std::string path = g_pState->Infrastructure->Settings->GetAppDataDirectory() + "\\event_weights.cfg";
	
	std::ofstream file(path, std::ios::binary);
	if (!file.is_open()) return;
	
	file << "# AutoTheater Event Weights\n";

	//g_pState->Domain->EventRegistry->ForEachEvent([&](const std::wstring& name, const EventInfo& info) {
	//	file << g_pSystem->Infrastructure->Format->WStringToString(name) << "=" << "\n";
	//});

	file.close();
	g_pSystem->Debug->Log("[EventRegistrySystem] INFO: Event registry saved.");
}

void EventRegistrySystem::LoadEventRegistry()
{
	std::string path = g_pState->Infrastructure->Settings->GetAppDataDirectory() + "\\event_weights.cfg";
	std::ifstream file(path);
	if (!file.is_open()) return;

	std::string line;
	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r') line.pop_back();
		size_t delimiterPos = line.find('=');
		if (delimiterPos == std::string::npos) continue;

		std::string nameStr = line.substr(0, delimiterPos);
		std::string weightStr = line.substr(delimiterPos + 1);
		std::wstring eventName = g_pSystem->Infrastructure->Format->StringToWString(nameStr);

		//try {
		//	int weight = std::stoi(weightStr);
		//	if (g_pState->Domain->EventRegistry->IsEventRegistered(eventName))
		//	{
		//		EventType type = g_pState->Domain->EventRegistry->GetEventType(eventName);
		//	}
		//}
		//catch (...) {}
	}

	file.close();
	g_pSystem->Debug->Log("[EventRegistrySystem] INFO: Event registry loaded.");
}

void EventRegistrySystem::InitializeDefaultRegistry()
{
	std::unordered_map<std::wstring, EventInfo> tempMap;

	// TODO: Add all of them
	tempMap = {
		// Server
		{ L"#cause_player joined the game",								this->BuildEvent(EventType::Join) },
		{ L"#cause_player rejoined the game",							this->BuildEvent(EventType::Rejoin) },
		{ L"#cause_player quit",										this->BuildEvent(EventType::Quit) },
		{ L"#effect_player was booted",									this->BuildEvent(EventType::Booted) },
		{ L"#cause_player joined the #effect_team",						this->BuildEvent(EventType::JoinedTeam) },
		{ L"#cause_player joined your team",							this->BuildEvent(EventType::JoinedTeam) },
		{ L"You joined the #effect_team",								this->BuildEvent(EventType::JoinedTeam) },

		// Match
		{ L"#cause_team took the lead!",								this->BuildEvent(EventType::TookLead) },
		{ L"#cause_player took the lead!",								this->BuildEvent(EventType::TookLead) },
		{ L"Your team took the lead!",									this->BuildEvent(EventType::TookLead) },
		{ L"You took the lead!",										this->BuildEvent(EventType::TookLead) },
		{ L"You lost the lead",											this->BuildEvent(EventType::LostLead) },
		{ L"Your team lost the lead",									this->BuildEvent(EventType::LostLead) },
		{ L"#cause_team tied for the lead!",							this->BuildEvent(EventType::TiedLead) },
		{ L"You are tied for the lead!",								this->BuildEvent(EventType::TiedLead) },
		{ L"Your team is tied for the lead!",							this->BuildEvent(EventType::TiedLead) },
		{ L"#cause_player tied for the lead!",							this->BuildEvent(EventType::TiedLead) },
		{ L"#cause_team scored", 										this->BuildEvent(EventType::TeamScored) },
		{ L"1 minute to win!",											this->BuildEvent(EventType::OneMinuteToWin) },
		{ L"#cause_player 1 minute to win!",							this->BuildEvent(EventType::OneMinuteToWin) },
		{ L"30 seconds to win!",										this->BuildEvent(EventType::ThirtySecondsToWin) },
		{ L"#cause_player 30 seconds to win!",							this->BuildEvent(EventType::ThirtySecondsToWin) },
		{ L"1 minute remaining",										this->BuildEvent(EventType::OneMinuteRemaining) },
		{ L"30 seconds remaining",										this->BuildEvent(EventType::ThirtySecondsRemaining) },
		{ L"10 seconds remaining",										this->BuildEvent(EventType::TenSecondsRemaining) },
		{ L"Round over",												this->BuildEvent(EventType::RoundOver) },
		{ L"Game over",													this->BuildEvent(EventType::GameOver) },

		// Custom
		{ L"Custom",													this->BuildEvent(EventType::Custom) },

		// CTF
		{ L"Capture the Flag",											this->BuildEvent(EventType::CaptureTheFlag) },
		{ L"#cause_team captured a flag!",								this->BuildEvent(EventType::FlagCaptured) },
		{ L"You captured a flag!",										this->BuildEvent(EventType::FlagCaptured) },
		{ L"#effect_team flag was stolen!",								this->BuildEvent(EventType::FlagStolen) },
		{ L"Your flag has been stolen!",								this->BuildEvent(EventType::FlagStolen) },
		{ L"You grabbed the #effect_team flag!",						this->BuildEvent(EventType::FlagStolen) },
		{ L"#effect_team flag dropped",									this->BuildEvent(EventType::FlagDropped) },
		{ L"You dropped #effect_team flag",								this->BuildEvent(EventType::FlagDropped) },
		{ L"#effect_team flag reset",									this->BuildEvent(EventType::FlagReseted) },
		{ L"#effect_team flag recovered",								this->BuildEvent(EventType::FlagRecovered) },
		{ L"Your flag has been recovered",								this->BuildEvent(EventType::FlagRecovered) },

		// Slayer
		{ L"Slayer",													this->BuildEvent(EventType::Slayer) },

		// Race 
		{ L"Race",														this->BuildEvent(EventType::Race) },

		// Assault
		{ L"Assault",													this->BuildEvent(EventType::Assault) },
		{ L"You picked up the bomb", 									this->BuildEvent(EventType::HasTheBomb) },
		{ L"You armed the bomb",										this->BuildEvent(EventType::ArmedTheBomb) },
		{ L"#cause_player armed the bomb",								this->BuildEvent(EventType::ArmedTheBomb) },
		{ L"Bomb reset",												this->BuildEvent(EventType::BombReset) },
		{ L"You dropped the bomb",										this->BuildEvent(EventType::DroppedTheBomb) },

		// Juggernaut
		{ L"Juggernaut",												this->BuildEvent(EventType::Juggernaut) },
		{ L"You are the juggernaut",									this->BuildEvent(EventType::IsTheJuggernaut) },
		{ L"#cause_player is the juggernaut",							this->BuildEvent(EventType::IsTheJuggernaut) },
		{ L"You killed the juggernaut",									this->BuildEvent(EventType::YouKilledTheJuggernaut) },

		// KOTH
		{ L"King of the Hill",											this->BuildEvent(EventType::KingOfTheHill) },
		{ L"#cause_player controls the hill", 							this->BuildEvent(EventType::ControlsTheHill) },
		{ L"You control the hill", 										this->BuildEvent(EventType::ControlsTheHill) },
		{ L"Your team controls the hill", 								this->BuildEvent(EventType::ControlsTheHill) },
		{ L"Hill contested",											this->BuildEvent(EventType::HillContested) },
		{ L"Hill moved",												this->BuildEvent(EventType::HillMoved) },

		// Territories
		{ L"Territories",												this->BuildEvent(EventType::Territories) },
		{ L"#cause_team captured a territory",							this->BuildEvent(EventType::TeamCapturedATerritory) },
		{ L"Your team captured a territory",							this->BuildEvent(EventType::TeamCapturedATerritory) },
		{ L"Territory contested",										this->BuildEvent(EventType::TerritoryContested) },
		{ L"Territory lost",											this->BuildEvent(EventType::TerritoryLost) },

		// Infection
		{ L"Infection",													this->BuildEvent(EventType::Infection) },
		{ L"You are a zombie",											this->BuildEvent(EventType::YouAreAZombie) },
		{ L"You infected #effect_player",								this->BuildEvent(EventType::Infected) },
		{ L"#cause_player infected you",								this->BuildEvent(EventType::Infected) },
		{ L"#cause_player is the last man standing",					this->BuildEvent(EventType::IsTheLastManStanding) },
		{ L"You are the last man standing",								this->BuildEvent(EventType::IsTheLastManStanding) },
		{ L"Zombies Win!",												this->BuildEvent(EventType::ZombiesWin) },
		{ L"Survivors Win!",											this->BuildEvent(EventType::SurvivorsWin) },

		// Oddball
		{ L"Oddball",													this->BuildEvent(EventType::Oddball) },
		{ L"Play ball!",												this->BuildEvent(EventType::PlayBall) },
		{ L"#cause_player picked up the ball",							this->BuildEvent(EventType::PickedUpTheBall) },
		{ L"You picked up the ball",									this->BuildEvent(EventType::PickedUpTheBall) },
		{ L"Ball dropped",												this->BuildEvent(EventType::BallDropped) },
		{ L"You dropped the ball",										this->BuildEvent(EventType::BallDropped) },
		{ L"Ball reset",												this->BuildEvent(EventType::BallReset) },

		// Kill
		{ L"Killionaire!",												this->BuildEvent(EventType::Killionaire) },
		{ L"Killpocalypse!",											this->BuildEvent(EventType::Killpocalypse) },
		{ L"Killtastrophe!",											this->BuildEvent(EventType::Killtastrophe) },
		{ L"Killimanjaro!",												this->BuildEvent(EventType::Killimanjaro) },
		{ L"Killtrocity!",												this->BuildEvent(EventType::Killtrocity) },
		{ L"Killtacular!",												this->BuildEvent(EventType::Killtacular) },
		{ L"#cause_player stopped #effect_player",						this->BuildEvent(EventType::Stopped) },
		{ L"You stopped #effect_player",								this->BuildEvent(EventType::Stopped) },
		{ L"#cause_player stopped you",									this->BuildEvent(EventType::Stopped) },
		{ L"Overkill!",													this->BuildEvent(EventType::Overkill) },
		{ L"#cause_player stuck #effect_player",						this->BuildEvent(EventType::Stuck) },
		{ L"#cause_player stuck you" ,									this->BuildEvent(EventType::Stuck) },
		{ L"You stuck #effect_player",									this->BuildEvent(EventType::Stuck) },
		{ L"#cause_player assassinated #effect_player",					this->BuildEvent(EventType::Assassinated) },
		{ L"#effect_player was assassinated by #cause_player",			this->BuildEvent(EventType::Assassinated) },
		{ L"#cause_player assassinated you",							this->BuildEvent(EventType::Assassinated) },
		{ L"You assassinated #effect_player",							this->BuildEvent(EventType::Assassinated) },
		{ L"You were killed while performing an assassination.",		this->BuildEvent(EventType::ShowStopper) },
		{ L"Showstopper!",												this->BuildEvent(EventType::ShowStopper) },
		{ L"Yoink!",													this->BuildEvent(EventType::Yoink) },
		{ L"Triple Kill!",												this->BuildEvent(EventType::TripleKill) },
		{ L"Double Kill!",												this->BuildEvent(EventType::DoubleKill) },
		{ L"#cause_player beat down #effect_player",					this->BuildEvent(EventType::BeatDown) },
		{ L"#cause_player beat you down",								this->BuildEvent(EventType::BeatDown) },
		{ L"You beat down #effect_player",								this->BuildEvent(EventType::BeatDown) },
		{ L"Headcase!",													this->BuildEvent(EventType::Headcase) },
		{ L"Bulltrue!",													this->BuildEvent(EventType::Bulltrue) },
		{ L"Splatter Spree!",											this->BuildEvent(EventType::SplatterSpree) },
		{ L"Vehicular Manslaughter!",									this->BuildEvent(EventType::VehicularManslaughter) },
		{ L"Sunday Driver!",											this->BuildEvent(EventType::SundayDriver) },
		{ L"Sticky Spree!",												this->BuildEvent(EventType::StickySpree) },
		{ L"Sticky Fingers!",											this->BuildEvent(EventType::StickyFingers) },
		{ L"Corrected!",												this->BuildEvent(EventType::Corrected) },
		{ L"Shotgun Spree!",											this->BuildEvent(EventType::ShotgunSpree) },
		{ L"Open Season!",												this->BuildEvent(EventType::OpenSeason) },
		{ L"Buck Wild!",												this->BuildEvent(EventType::BuckWild) },
		{ L"Wrecking Crew!",											this->BuildEvent(EventType::WreckingCrew) },
		{ L"Dreamcrusher!",												this->BuildEvent(EventType::Dreamcrusher) },
		{ L"Hammer Spree!",												this->BuildEvent(EventType::HammerSpree) },
		{ L"Sword Spree!",												this->BuildEvent(EventType::SwordSpree) },
		{ L"Slice 'n Dice!",											this->BuildEvent(EventType::SliceNDice) },
		{ L"Cutting Crew!", 											this->BuildEvent(EventType::CuttingCrew) },

		{ L"Juggernaut Spree!",											this->BuildEvent(EventType::JuggernautSpree) },

		{ L"#cause_player's driving assisted you",						this->BuildEvent(EventType::Wheelman) },
		{ L"Your driving assisted #effect_player",						this->BuildEvent(EventType::Wheelman) },
		{ L"Wheelman Spree!",											this->BuildEvent(EventType::WheelmanSpree) },
		{ L"Road Hog!",													this->BuildEvent(EventType::Roadhog) },
		{ L"Road Rage!",												this->BuildEvent(EventType::Roadrage) },
		{ L"#cause_player is on a Killing Spree!",						this->BuildEvent(EventType::KillingSpree) },
		{ L"Killing Spree!",											this->BuildEvent(EventType::KillingSpree) },
		{ L"#cause_player is on a Killing Frenzy!",						this->BuildEvent(EventType::KillingFrenzy) },
		{ L"Killing Frenzy!",											this->BuildEvent(EventType::KillingFrenzy) },
		{ L"#cause_player is a Running Riot!",							this->BuildEvent(EventType::RunningRiot) },
		{ L"Running Riot!",												this->BuildEvent(EventType::RunningRiot) },
		{ L"#cause_player is on a Rampage!",							this->BuildEvent(EventType::Rampage) },
		{ L"Rampage!",													this->BuildEvent(EventType::Rampage) },
		{ L"#cause_player is Untouchable!",								this->BuildEvent(EventType::Untouchable) },
		{ L"Untouchable!",												this->BuildEvent(EventType::Untouchable) },
		{ L"#cause_player is Invincible!",								this->BuildEvent(EventType::Invincible) },
		{ L"Invincible!",												this->BuildEvent(EventType::Invincible) },
		{ L"#cause_player is Inconceivable!",							this->BuildEvent(EventType::Inconceivable) },
		{ L"Inconceivable!",											this->BuildEvent(EventType::Inconceivable) },
		{ L"#cause_player is Unfrigginbelievable!",						this->BuildEvent(EventType::Unfrigginbelievable) },
		{ L"Unfrigginbelievable!",										this->BuildEvent(EventType::Unfrigginbelievable) },
		{ L"#cause_player splattered #effect_player",					this->BuildEvent(EventType::Splattered) },
		{ L"You splattered #effect_player",								this->BuildEvent(EventType::Splattered) },
		{ L"#cause_player splattered you",								this->BuildEvent(EventType::Splattered) },
		{ L"#cause_player pummeled #effect_player",						this->BuildEvent(EventType::Pummeled) },
		{ L"#cause_player pummeled you",								this->BuildEvent(EventType::Pummeled) },
		{ L"You pummeled #effect_player",								this->BuildEvent(EventType::Pummeled) },
		{ L"#cause_player killed #effect_player with a headshot",		this->BuildEvent(EventType::Headshot) },
		{ L"#effect_player was killed by a #cause_player headshot",		this->BuildEvent(EventType::Headshot) },
		{ L"#cause_player killed you with a headshot",					this->BuildEvent(EventType::Headshot) },
		{ L"You killed #effect_player with a headshot",					this->BuildEvent(EventType::Headshot) },
		{ L"#cause_player killed you from the grave",					this->BuildEvent(EventType::KillFromTheGrave) },
		{ L"You killed #effect_player from the grave",					this->BuildEvent(EventType::KillFromTheGrave) },
		{ L"#cause_player got revenge!",								this->BuildEvent(EventType::Revenge) },
		{ L"Revenge!",													this->BuildEvent(EventType::Revenge) },
		{ L"First Strike!",												this->BuildEvent(EventType::FirstStrike) },
		{ L"Reload This!",												this->BuildEvent(EventType::ReloadThis) },
		{ L"You were killed while reloading",							this->BuildEvent(EventType::ReloadThis) },
		{ L"Close Call!",												this->BuildEvent(EventType::CloseCall) },
		{ L"Protector!",												this->BuildEvent(EventType::Protector) },
		{ L"#effect_player saved your life.",							this->BuildEvent(EventType::Protector) },
		{ L"Firebird!",													this->BuildEvent(EventType::Firebird) },
		{ L"Killjoy!",													this->BuildEvent(EventType::Killjoy) },
		{ L"#cause_player ended your spree",							this->BuildEvent(EventType::Killjoy) },
		{ L"Avenger!",													this->BuildEvent(EventType::Avenger) },
		{ L"#effect_player avenged your death.",						this->BuildEvent(EventType::Avenger) },
		{ L"Pull!",														this->BuildEvent(EventType::Pull) },
		{ L"#cause_player struck #effect_player down!",					this->BuildEvent(EventType::Struck) },
		{ L"#cause_player struck you down!",							this->BuildEvent(EventType::Struck) },
		{ L"#cause_player killed #effect_player with grenades",			this->BuildEvent(EventType::KillWithGrenades) },
		{ L"#effect_player was killed by #cause_player with grenades",	this->BuildEvent(EventType::KillWithGrenades) },
		{ L"You killed #effect_player with grenades",					this->BuildEvent(EventType::KillWithGrenades) },
		{ L"#cause_player killed you with grenades",					this->BuildEvent(EventType::KillWithGrenades) },
		{ L"#cause_player lasered #effect_player",						this->BuildEvent(EventType::Lasered) },
		{ L"#effect_player was lasered by #cause_player",				this->BuildEvent(EventType::Lasered) },
		{ L"You lasered #effect_player",								this->BuildEvent(EventType::Lasered) },
		{ L"#cause_player lasered you",									this->BuildEvent(EventType::Lasered) },
		{ L"#cause_player sniped #effect_player",						this->BuildEvent(EventType::Sniped) },
		{ L"#cause_player sniped you",									this->BuildEvent(EventType::Sniped) },
		{ L"You sniped #effect_player",									this->BuildEvent(EventType::Sniped) },
		{ L"Second Gunman!",											this->BuildEvent(EventType::SecondGunman) },
		{ L"Sidekick!",													this->BuildEvent(EventType::Sidekick) },
		{ L"Assist Spree!",												this->BuildEvent(EventType::AssistSpree) },
		{ L"Assist",													this->BuildEvent(EventType::Assist) },
		{ L"#cause_player killed #effect_player",						this->BuildEvent(EventType::Kill) },
		{ L"You killed #effect_player",									this->BuildEvent(EventType::Kill) },
		{ L"#cause_player killed you",									this->BuildEvent(EventType::Kill) },
		{ L"#cause_player committed suicide",							this->BuildEvent(EventType::Suicide) },
		{ L"You committed suicide",										this->BuildEvent(EventType::Suicide) },
		{ L"#cause_player betrayed #effect_player",						this->BuildEvent(EventType::Betrayal) },
		{ L"You betrayed #effect_player",								this->BuildEvent(EventType::Betrayal) },
		{ L"#cause_player betrayed you",								this->BuildEvent(EventType::Betrayal) },
		{ L"Killed by the Guardians",									this->BuildEvent(EventType::KilledByTheGuardians) },
		{ L"You fell to your death",									this->BuildEvent(EventType::FellToYourDeath) },
	};

	//g_pState->Domain->EventRegistry->SetEventRegistry(std::move(tempMap));
}

EventInfo EventRegistrySystem::BuildEvent(EventType type)
{
	return { type, this->ResolveEventClass(type) };
}

EventClass EventRegistrySystem::ResolveEventClass(EventType type)
{
	switch (type)
	{
	case EventType::Join:
	case EventType::Rejoin:
	case EventType::Quit:
	case EventType::Booted:
	case EventType::JoinedTeam:
		return EventClass::Server;

	case EventType::TookLead:
	case EventType::LostLead:
	case EventType::TiedLead:
	case EventType::TeamScored:
	case EventType::OneMinuteToWin:
	case EventType::ThirtySecondsToWin:
	case EventType::OneMinuteRemaining:
	case EventType::ThirtySecondsRemaining:
	case EventType::TenSecondsRemaining:
	case EventType::RoundOver:
	case EventType::GameOver:
		return EventClass::Match;

	case EventType::Custom:
		return EventClass::Custom;

	case EventType::CaptureTheFlag:
	case EventType::FlagCaptured:
	case EventType::FlagStolen:
	case EventType::FlagDropped:
	case EventType::FlagReseted:
	case EventType::FlagRecovered:
		return EventClass::CaptureTheFlag;

	case EventType::Slayer:
		return EventClass::Slayer;

	case EventType::Race:
		return EventClass::Race;

	case EventType::Assault:
	case EventType::HasTheBomb:
	case EventType::ArmedTheBomb:
	case EventType::DroppedTheBomb:
	case EventType::BombReset:
		return EventClass::Assault;

	case EventType::Juggernaut:
	case EventType::IsTheJuggernaut:
	case EventType::YouKilledTheJuggernaut:
		return EventClass::Juggernaut;

	case EventType::KingOfTheHill:
	case EventType::ControlsTheHill:
	case EventType::HillContested:
	case EventType::HillMoved:
		return EventClass::KingOfTheHill;

	case EventType::Territories:
	case EventType::TeamCapturedATerritory:
	case EventType::TerritoryContested:
	case EventType::TerritoryLost:
		return EventClass::Territories;

	case EventType::Infection:
	case EventType::YouAreAZombie:
	case EventType::Infected:
	case EventType::IsTheLastManStanding:
	case EventType::ZombiesWin:
	case EventType::SurvivorsWin:
		return EventClass::Infection;

	case EventType::Oddball:
	case EventType::PlayBall:
	case EventType::PickedUpTheBall:
	case EventType::BallDropped:
	case EventType::BallReset:
		return EventClass::Oddball;

	case EventType::Killionaire:
	case EventType::Killpocalypse:
	case EventType::Killtastrophe:
	case EventType::Killimanjaro:
	case EventType::Killtrocity:
	case EventType::Killtacular:
	case EventType::Stopped:
	case EventType::Overkill:
	case EventType::Stuck:
	case EventType::Assassinated:
	case EventType::ShowStopper:
	case EventType::Yoink:
	case EventType::TripleKill:
	case EventType::DoubleKill:
	case EventType::BeatDown:
	case EventType::Headcase:
	case EventType::Bulltrue:
	case EventType::SplatterSpree:
	case EventType::VehicularManslaughter:
	case EventType::SundayDriver:
	case EventType::StickySpree:
	case EventType::StickyFingers:
	case EventType::Corrected:
	case EventType::ShotgunSpree:
	case EventType::OpenSeason:
	case EventType::BuckWild:
	case EventType::WreckingCrew:
	case EventType::Dreamcrusher:
	case EventType::HammerSpree:
	case EventType::SwordSpree:
	case EventType::SliceNDice:
	case EventType::CuttingCrew:
	case EventType::JuggernautSpree:
	case EventType::Wheelman:
	case EventType::WheelmanSpree:
	case EventType::Roadhog:
	case EventType::Roadrage:
	case EventType::KillingSpree:
	case EventType::KillingFrenzy:
	case EventType::RunningRiot:
	case EventType::Rampage:
	case EventType::Untouchable:
	case EventType::Invincible:
	case EventType::Inconceivable:
	case EventType::Unfrigginbelievable:
	case EventType::Splattered:
	case EventType::Pummeled:
	case EventType::Headshot:
	case EventType::KillFromTheGrave:
	case EventType::Revenge:
	case EventType::FirstStrike:
	case EventType::ReloadThis:
	case EventType::CloseCall:
	case EventType::Protector:
	case EventType::Firebird:
	case EventType::Killjoy:
	case EventType::Avenger:
	case EventType::Pull:
	case EventType::Struck:
	case EventType::KillWithGrenades:
	case EventType::Lasered:
	case EventType::Sniped:
	case EventType::SecondGunman:
	case EventType::Sidekick:
	case EventType::AssistSpree:
	case EventType::Assist:
	case EventType::Kill:
	case EventType::Suicide:
	case EventType::Betrayal:
	case EventType::KilledByTheGuardians:
	case EventType::FellToYourDeath:
		return EventClass::KillRelated;

	default:
		return EventClass::Unknown;
	}
}