#include "pch.h"

// Header.
#include "Core_Hook_Domain.h"

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

Core_Hook_Domain::Core_Hook_Domain()
{
    // Map.
    BlamOpenMap = std::make_unique<Hook_BlamOpenMap>();

    // Object.
    CreateGO = std::make_unique<Hook_CreateObject>();
    ReleaseGO = std::make_unique<Hook_ReleaseObject>();
    ObjectTable = std::make_unique<Hook_ObjectTable>();

    // Player.
    CreatePlayer = std::make_unique<Hook_CreatePlayer>();
    PlayerTable = std::make_unique<Hook_PlayerTable>();

    // Interaction.
    InteractionTable = std::make_unique<Hook_InteractionTable>();
}

Core_Hook_Domain::~Core_Hook_Domain() = default;