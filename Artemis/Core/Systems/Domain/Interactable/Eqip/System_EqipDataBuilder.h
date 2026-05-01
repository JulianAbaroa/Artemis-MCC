#pragma once

struct EqipObject;
struct EquipmentData;
enum class EquipmentType : uint8_t;

class System_EqipDataBuilder
{
public:
    EquipmentData BuildData(const EqipObject& eqip);

private:
    EquipmentType DeriveType(const EqipObject& eqip);
};