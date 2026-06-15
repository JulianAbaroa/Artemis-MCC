#include "pch.h"

#include "Core_UI.h"

#include "Core/States/Core_State.h"

#include "Core/Systems/Core_System.h"

#include "Launcher/UI_Tab.h"

#include "Launcher/UI_Launcher.h"
#include "Overlay/UI_Overlay.h"
#include "ObjectTable/UI_ObjectTable.h"
#include "PlayerTable/UI_PlayerTable.h"
#include "Settings/UI_Settings.h"
#include "MemoryScanner/UI_MemoryScanner.h"
#include "Logs/UI_Logs.h"

Core_UI::Core_UI() = default;
Core_UI::~Core_UI() = default;

void Core_UI::Initialize(Core_State& state, Core_System& system)
{
	ObjectTable = std::make_unique<UI_ObjectTable>(
		*state.ObjectTable);

	PlayerTable = std::make_unique<UI_PlayerTable>(
		*state.PlayerTable);

	Settings = std::make_unique<UI_Settings>(
		UI_Settings_Deps {
			.State_Render = *state.Render,
			.State_Settings = *state.Settings,
			.System_Settings = *system.Settings,
			.System_Logs = *system.Logs,
		});

	MemoryScanner = std::make_unique<UI_MemoryScanner>(
		UI_MemoryScanner_Deps {
			.State_MemoryScanner = *state.Memory,
			.System_MemoryScanner = *system.MemoryScanner,
		});

	Logs = std::make_unique<UI_Logs>(
		UI_Logs_Deps{
			.State_Settings = *state.Settings,
			.State_Logs = *state.Logs,
			.System_Logs = *system.Logs,
		});


	Launcher = std::make_unique<UI_Launcher>(
		UI_Launcher_Deps { 
			.Tabs = {
				ObjectTable.get(),
				PlayerTable.get(),
				Settings.get(),
				MemoryScanner.get(),
				Logs.get()
			},
			.State_Render = *state.Render,
		});

	Overlay = std::make_unique<UI_Overlay>(
		UI_Overlay_Deps {
			.State_Render = *state.Render,
			.State_Selection = *state.Selection,
			.State_OverlayMode = *state.OverlayMode,
			.State_Telemetry = *state.Telemetry,
		});
}

void Core_UI::Deinitialize()
{
	Launcher.reset();
	Overlay.reset();
	ObjectTable.reset();
	PlayerTable.reset();
	Settings.reset();
	MemoryScanner.reset();
	Logs.reset();
}