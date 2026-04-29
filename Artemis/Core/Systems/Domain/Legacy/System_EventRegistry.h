#pragma once

#include "Core/Types/Domain/Legacy/TimelineTypes.h"

class System_EventRegistry
{
public:
	System_EventRegistry();
	void InitializeDefaultRegistry();

	void SaveEventRegistry();
	void LoadEventRegistry();

private:
	EventInfo BuildEvent(EventType type);
	EventClass ResolveEventClass(EventType type);
};