#include "pch.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Infrastructure/Engine/System_Input.h"
#include "Core/Systems/Infrastructure/Engine/System_Lifecycle.h"
#include "Core/Systems/Infrastructure/Engine/System_Render.h"
#include "Core/Systems/Infrastructure/Engine/System_Scanner.h"
#include "Core/Systems/Infrastructure/Persistence/System_Preferences.h"
#include "Core/Systems/Infrastructure/Persistence/System_Settings.h"
#include "Core/Systems/Infrastructure/Engine/System_Thread.h"
#include "Core/Systems/Infrastructure/Engine/System_Format.h"
#include "Core/Systems/Infrastructure/Engine/System_Dialog.h"

Core_System_Infrastructure::Core_System_Infrastructure()
{
	Input = std::make_unique<System_Input>();
	Lifecycle = std::make_unique<System_Lifecycle>();
	Render = std::make_unique<System_Render>();
	Scanner = std::make_unique<System_Scanner>();
	Preferences = std::make_unique<System_Preferences>();
	Settings = std::make_unique<System_Settings>();
	Thread = std::make_unique<System_Thread>();
	Format = std::make_unique<System_Format>();
	Dialog = std::make_unique<System_Dialog>();
}

Core_System_Infrastructure::~Core_System_Infrastructure() = default;