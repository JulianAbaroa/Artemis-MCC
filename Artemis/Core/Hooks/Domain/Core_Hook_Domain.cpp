#include "pch.h"
#include "Core/Hooks/Domain/Core_Hook_Domain.h"
#include "Core/Hooks/Domain/Map/Hook_BlamOpenMap.h"
#include "Core/Hooks/Domain/Tables/Hook_CreateObject.h"
#include "Core/Hooks/Domain/Tables/Hook_ReleaseObject.h"
#include "Core/Hooks/Domain/Tables/Hook_CreatePlayer.h"
#include "Core/Hooks/Domain/Tables/Hook_PlayerTable.h"
#include "Core/Hooks/Domain/Tables/Hook_ObjectTable.h"
#include "Core/Hooks/Domain/Tables/Hook_InteractionTable.h"
//#include "Core/Hooks/Domain/Legacy/Hook_BlamOpenFile.h"
#include "Core/Hooks/Domain/Legacy/Hook_BuildGameEvent.h"

Core_Hook_Domain::Core_Hook_Domain()
{
    // Map.
    BlamOpenMap = std::make_unique<Hook_BlamOpenMap>();

    // Tables.
    CreateGO = std::make_unique<Hook_CreateObject>();
    ReleaseGO = std::make_unique<Hook_ReleaseObject>();
    CreatePlayer = std::make_unique<Hook_CreatePlayer>();
    PlayerTable = std::make_unique<Hook_PlayerTable>();
    ObjectTable = std::make_unique<Hook_ObjectTable>();
    InteractionTable = std::make_unique<Hook_InteractionTable>();

    // Legacy.
    //BlamOpenFile = std::make_unique<BlamOpenFileHook>();
    BuildGameEvent = std::make_unique<Hook_BuildGameEvent>();
}

Core_Hook_Domain::~Core_Hook_Domain() = default;