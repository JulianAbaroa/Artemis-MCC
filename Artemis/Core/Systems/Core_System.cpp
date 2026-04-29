#include "pch.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Interface/System_Debug.h"

Core_System::Core_System()
{
	Domain = std::make_unique<Core_System_Domain>();
	Infrastructure = std::make_unique<Core_System_Infrastructure>();
	Debug = std::make_unique<System_Debug>();
}

Core_System::~Core_System() = default;