#pragma once

#include <memory>

class Core_State;
class Core_System;
class Core_UI;
class Core_Hook;
class Core_Thread;

class Core
{
public:
	Core();
	~Core();

	void Initialize();
	void Deinitialize() const;
	
	std::unique_ptr<Core_State> State;
	std::unique_ptr<Core_System> System;
	std::unique_ptr<Core_UI> UI;
	std::unique_ptr<Core_Hook> Hook;
	std::unique_ptr<Core_Thread> Thread;
};