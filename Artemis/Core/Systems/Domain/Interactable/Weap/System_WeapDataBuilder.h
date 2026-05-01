#pragma once

struct WeapObject;
struct WeaponData;
struct WeaponBarrelData;
struct WeaponMagazineData;
struct WeaponTriggerData;
struct WeaponTargetTrackingData;

class System_WeapDataBuilder
{
public:
    WeaponData BuildData(const WeapObject& weap);

private:
    WeaponBarrelData         BuildBarrel(const WeapObject& weap);
    WeaponMagazineData       BuildMagazine(const WeapObject& weap);
    WeaponTriggerData        BuildTrigger(const WeapObject& weap);
    WeaponTargetTrackingData BuildTargetTracking(const WeapObject& weap);
};