#pragma once

#include <chrono>

class System_Thread
{
public:
	bool WaitOrExit(std::chrono::milliseconds ms);
};