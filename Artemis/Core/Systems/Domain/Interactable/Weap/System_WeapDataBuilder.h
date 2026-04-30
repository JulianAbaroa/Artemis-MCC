#pragma once

struct WeaponMagazineData;
struct WeaponBarrelData;
struct WeapObject;
struct WeaponData;

class System_WeapDataBuilder
{
public:
    WeaponData BuildData(const WeapObject& weap);

private:
    WeaponBarrelData BuildBarrel(const WeapObject& weap);
    WeaponMagazineData BuildMagazine(const WeapObject& weap);
};