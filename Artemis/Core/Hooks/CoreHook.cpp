#include "pch.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/Hooks/Data/CoreDataHook.h"
#include "Core/Hooks/Input/CoreInputHook.h"
#include "Core/Hooks/Lifecycle/CoreLifecycleHook.h"
#include "Core/Hooks/Memory/CoreMemoryHook.h"
#include "Core/Hooks/Render/CoreRenderHook.h"
#include "Core/Hooks/Window/CoreWindowHook.h"

CoreHook::CoreHook()
{
	Data = std::make_unique<CoreDataHook>();
	Input = std::make_unique<CoreInputHook>();
	Lifecycle = std::make_unique<CoreLifecycleHook>();
	Memory = std::make_unique<CoreMemoryHook>();
	Render = std::make_unique<CoreRenderHook>();
	Window = std::make_unique<CoreWindowHook>();
}

CoreHook::~CoreHook() = default;