#include "pch.h"
#include "Core/Systems/Domain/Interactable/Vehi/System_VehiDataBuilder.h"

VehicleData System_VehiDataBuilder::BuildData(const VehiObject& vehi)
{
    VehicleData out{};
    out.TagName = vehi.TagName;
    out.Seats.reserve(vehi.Seats.size());

    for (const auto& src : vehi.Seats)
    {
        VehicleSeat seat;
        seat.SeatMarkerNameId = src.SeatMarkerName;
        seat.EntryMarkerNameId = src.EntryMarkerSName;
        seat.AISeatType = static_cast<VehicleAISeatType>(src.AiSeatType);
        seat.EntryRadius = src.EntryRadius;

        // Flags: Driver=bit2, Gunner=bit3, InvalidForPlayer=bit14, InvalidForNonPlayer=bit15
        seat.IsDriver = (src.Flags & (1u << 2)) != 0;
        seat.IsGunner = (src.Flags & (1u << 3)) != 0;
        seat.HasMarker = (src.SeatMarkerName != 0);
        seat.InvalidForPlayer = (src.Flags & (1u << 14)) != 0;
        seat.InvalidForNonPlayer = (src.Flags & (1u << 15)) != 0;

        out.Seats.push_back(seat);
    }

    return out;
}