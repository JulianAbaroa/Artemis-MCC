#pragma once

#include "Core/Common/Types/Domain/TimelineTypes.h"

class EventRegistrySystem
{
public:
	EventRegistrySystem();
	void InitializeDefaultRegistry();

	void SaveEventRegistry();
	void LoadEventRegistry();

private:
	EventInfo BuildEvent(EventType type);
	EventClass ResolveEventClass(EventType type);
};