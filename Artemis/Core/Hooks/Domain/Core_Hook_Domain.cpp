#include "pch.h"

// Header.
#include "Core_Hook_Domain.h"

#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"

#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Interface/Core_System_Interface.h"

// Map.
#include "Core/Hooks/Domain/Map/Hook_BlamOpenMap.h"

// Object.
#include "Core/Hooks/Domain/Object/Hook_CreateObject.h"
#include "Core/Hooks/Domain/Object/Hook_ReleaseObject.h"
#include "Core/Hooks/Domain/Object/Hook_ObjectTable.h"

// Player.
#include "Core/Hooks/Domain/Player/Hook_CreatePlayer.h"
#include "Core/Hooks/Domain/Player/Hook_PlayerTable.h"

// Interaction.
#include "Core/Hooks/Domain/Interaction/Hook_InteractionTable.h"

Core_Hook_Domain::Core_Hook_Domain() = default;
Core_Hook_Domain::~Core_Hook_Domain() = default;

void Core_Hook_Domain::Initialize(Core_State& state, Core_System& system)
{
    // Map.
    BlamOpenMap = std::make_unique<Hook_BlamOpenMap>(
        Hook_BlamOpenMap_Dependencies {
            .System_Map = *system.Domain->Map,
            .System_MapTagGroup = *system.Domain->MapTagGroup,
            .System_Navigation = *system.Domain->Navigation,
            .System_Environment = *system.Domain->Environment,
            .System_Interactable = *system.Domain->Interactable,
            .System_AOBScanner = *system.Infrastructure->AOBScanner,
            .System_Debug = *system.Interface->Debug,
        }
    );

    // Object.
    CreateGameObject = std::make_unique<Hook_CreateObject>(
        Hook_CreateObject_Dependencies {
            .System_ObjectTable = *system.Domain->ObjectTable,
            .System_AOBScanner = *system.Infrastructure->AOBScanner,
            .System_Debug = *system.Interface->Debug,
        }
    );

    ReleaseGameObject = std::make_unique<Hook_ReleaseObject>(
        Hook_ReleaseObject_Dependencies {
            .System_ObjectTable = *system.Domain->ObjectTable,
            .System_AOBScanner = *system.Infrastructure->AOBScanner,
            .System_Debug = *system.Interface->Debug,
        }
    );

    ObjectTable = std::make_unique<Hook_ObjectTable>(
        Hook_ObjectTable_Dependencies {
            .State_ObjectTable = *state.Domain->ObjectTable,
            .System_AOBScanner = *system.Infrastructure->AOBScanner,
            .System_Debug = *system.Interface->Debug,
        }
    );

    // Player.
    CreatePlayer = std::make_unique<Hook_CreatePlayer>(
        Hook_CreatePlayer_Dependencies {
            .System_PlayerTable = *system.Domain->PlayerTable,
            .System_AOBScanner = *system.Infrastructure->AOBScanner,
            .System_Debug = *system.Interface->Debug,
        }
    );

    PlayerTable = std::make_unique<Hook_PlayerTable>(
        Hook_PlayerTable_Dependencies {
            .State_PlayerTable = *state.Domain->PlayerTable,
            .System_AOBScanner = *system.Infrastructure->AOBScanner,
            .System_Debug = *system.Interface->Debug,
        }
    );

    // Interaction.
    InteractionTable = std::make_unique<Hook_InteractionTable>(
        Hook_InteractionTable_Dependencies {
            .State_InteractionTable = *state.Domain->InteractionTable,
            .System_AOBScanner = *system.Infrastructure->AOBScanner,
            .System_Debug = *system.Interface->Debug,
        }
    );
}

void Core_Hook_Domain::Shutdown()
{
    if (BlamOpenMap) BlamOpenMap->Uninstall();
    BlamOpenMap.reset();

    if (CreateGameObject) CreateGameObject->Uninstall();
    CreateGameObject.reset();

    if (ReleaseGameObject) ReleaseGameObject->Uninstall();
    ReleaseGameObject.reset();

    if (CreatePlayer) CreatePlayer->Uninstall();
    CreatePlayer.reset();

    ObjectTable.reset();
    PlayerTable.reset();
    InteractionTable.reset();
}