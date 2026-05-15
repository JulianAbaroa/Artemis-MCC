#pragma once

#include <memory>

struct Core_State;
struct Core_System;
struct Core_Hook;
struct Core_Thread;
struct Core_UI;

// Artemis orchestrator class.
class Mod_Core
{
public:
	Mod_Core();
	~Mod_Core();

	void Initialize();
	void Shutdown() const;
	
	std::unique_ptr<Core_State> State;
	std::unique_ptr<Core_System> System;
	std::unique_ptr<Core_Hook> Hook;
	std::unique_ptr<Core_Thread> Thread;
	std::unique_ptr<Core_UI> UI;
};