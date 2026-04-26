#include "pch.h"
#include "Core/Hooks/Memory/CoreMemoryHook.h"
#include "Core/Hooks/Memory/PlayerTableHook.h"
#include "Core/Hooks/Memory/ObjectTableHook.h"
#include "Core/Hooks/Memory/InteractionTableHook.h"

CoreMemoryHook::CoreMemoryHook()
{
	PlayerTable = std::make_unique<PlayerTableHook>();
	ObjectTable = std::make_unique<ObjectTableHook>();
	InteractionTable = std::make_unique<InteractionTableHook>();
}

CoreMemoryHook::~CoreMemoryHook() = default;