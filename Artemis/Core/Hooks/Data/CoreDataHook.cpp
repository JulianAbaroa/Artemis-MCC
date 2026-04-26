#include "pch.h"
#include "Core/Hooks/Data/CoreDataHook.h"
#include "Core/Hooks/Data/Tables/CreateObjectHook.h"
#include "Core/Hooks/Data/Tables/ReleaseObjectHook.h"
#include "Core/Hooks/Data/Tables/CreatePlayerHook.h"
#include "Core/Hooks/Data/Map/BlamOpenMapHook.h"
//#include "Core/Hooks/Data/Legacy/BlamOpenFileHook.h"
#include "Core/Hooks/Data/Legacy/BuildGameEventHook.h"

CoreDataHook::CoreDataHook()
{
    CreateGO = std::make_unique<CreateObjectHook>();
    ReleaseGO = std::make_unique<ReleaseObjectHook>();
    CreatePlayer = std::make_unique<CreatePlayerHook>();

    BlamOpenMap = std::make_unique<BlamOpenMapHook>();

    //BlamOpenFile = std::make_unique<BlamOpenFileHook>();
    BuildGameEvent = std::make_unique<BuildGameEventHook>();
}

CoreDataHook::~CoreDataHook() = default;