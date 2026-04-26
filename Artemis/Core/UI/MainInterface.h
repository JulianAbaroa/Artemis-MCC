#pragma once

#include "Core/Common/Types/Domain/ModTypes.h"

class MainInterface
{
public:
	void Draw();

private:
	void HandleWindowReset();
	void DrawStatusBar();
	void DrawTabs();
};