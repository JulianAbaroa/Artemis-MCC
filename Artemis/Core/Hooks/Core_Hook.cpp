#include "pch.h"
#include "Core/Hooks/Core_Hook.h"
#include "Core/Hooks/Domain/Core_Hook_Domain.h"
#include "Core/Hooks/Infrastructure/Core_Hook_Infrastructure.h"

Core_Hook::Core_Hook()
{
	Domain = std::make_unique<Core_Hook_Domain>();
	Infrastructure = std::make_unique<Core_Hook_Infrastructure>();
}

Core_Hook::~Core_Hook() = default;