#pragma once

struct BlocTeleporterData;
struct BlocObject;

class System_BlocTeleporterBuilder
{
public:
    BlocTeleporterData BuildData(const BlocObject& bloc) const;
    bool IsTeleporter(const BlocObject& bloc) const;
};