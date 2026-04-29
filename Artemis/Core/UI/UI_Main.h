#pragma once

#include "Core/Types/Domain/ModTypes.h"

class UI_Main
{
public:
	void Draw();

private:
	void HandleWindowReset();
	void DrawStatusBar();
	void DrawTabs();
};