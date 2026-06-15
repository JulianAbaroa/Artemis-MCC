#pragma once

struct Weap;
struct WeapObject;
struct WeaponBarrelData;
struct WeaponMagazineData;
struct WeaponTriggerData;
struct WeaponTargetTrackingData;

class System_Logs;

struct Sys_WeapBuilder_Deps
{
    System_Logs& System_Logs;
};

class System_WeapBuilder
{
public:
    System_WeapBuilder(Sys_WeapBuilder_Deps deps) : m_Deps(deps) {}
    ~System_WeapBuilder() = default;

    Weap Build(const WeapObject& weap);

private:
    Sys_WeapBuilder_Deps m_Deps;

    WeaponBarrelData BuildBarrel(const WeapObject& weap);
    WeaponMagazineData BuildMagazine(const WeapObject& weap);
    WeaponTriggerData BuildTrigger(const WeapObject& weap);
    WeaponTargetTrackingData BuildTargetTracking(const WeapObject& weap);
};