#pragma once

#include <memory>

class CreateObjectHook;
class ReleaseObjectHook;
class CreatePlayerHook;

class BlamOpenMapHook;

//class BlamOpenFileHook;
class BuildGameEventHook;

struct CoreDataHook
{
	CoreDataHook();
	~CoreDataHook();

	std::unique_ptr<CreateObjectHook> CreateGO;
	std::unique_ptr<ReleaseObjectHook> ReleaseGO;
	std::unique_ptr<CreatePlayerHook> CreatePlayer;

	std::unique_ptr<BlamOpenMapHook> BlamOpenMap;

	//std::unique_ptr<BlamOpenFileHook> BlamOpenFile;
	std::unique_ptr<BuildGameEventHook> BuildGameEvent;
};