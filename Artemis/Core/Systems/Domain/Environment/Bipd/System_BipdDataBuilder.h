#pragma once

#include "Core/Types/Domain/Domains/Environment/BipdPhysicsData.h"
#include "Generated/Bipd/BipdObject.h"

struct BipdObject;
struct BipdPhysicsData;

class System_BipdDataBuilder
{
public:
    static BipdPhysicsData Build(const BipdObject& bipd);
};