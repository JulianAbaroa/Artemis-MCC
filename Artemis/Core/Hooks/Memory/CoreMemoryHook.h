#pragma once

// Blam! uses TLS (Thread Local Storage) to safely manage data on multi-threaded contexts.
// Only a thread with a TLS index initialized by Blam! can "see" or access this data.
// This is why these functions must be called within the scope of a Blam! function hook (or a game-managed thread).

#include <memory>

class PlayerTableHook;
class ObjectTableHook;
class InteractionTableHook;

struct CoreMemoryHook
{
	CoreMemoryHook();
	~CoreMemoryHook();
	
	std::unique_ptr<PlayerTableHook> PlayerTable;
	std::unique_ptr<ObjectTableHook> ObjectTable;
	std::unique_ptr<InteractionTableHook> InteractionTable;
};