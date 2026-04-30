#pragma once

struct EqipObject;
enum class EquipmentType : uint8_t;
struct EquipmentData;

class System_EqipDataBuilder
{
public:
    EquipmentData BuildData(const EqipObject& eqip);

private:
    EquipmentType DeriveType(const EqipObject& eqip);
};