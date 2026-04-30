#pragma once

struct BipdObject;
struct BipdPhysicsData;

class System_BipdDataBuilder
{
public:
    BipdPhysicsData BuildData(const BipdObject& bipd);
};