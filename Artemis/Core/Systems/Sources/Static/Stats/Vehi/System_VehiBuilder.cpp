#include "pch.h"

#include "System_VehiBuilder.h"

#include "Core/Types/Sources/Static/Stats/Vehi.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

#include "Generated/Vehi/VehiObject.h"

Vehi System_VehiBuilder::Build(const VehiObject& vehi)
{
    Vehi out{};
    out.TagName = vehi.TagName;
    out.Seats.reserve(vehi.Seats.size());

    for (const auto& source : vehi.Seats)
    {
        VehicleSeat seat;
        seat.SeatMarkerNameId = source.SeatMarkerName;
        seat.EntryMarkerNameId = source.EntryMarkerSName;
        seat.AISeatType = static_cast<VehicleAISeatType>(source.AiSeatType);
        seat.EntryRadius = source.EntryRadius;

        seat.IsDriver = (source.Flags & (1u << 2)) != 0;
        seat.IsGunner = (source.Flags & (1u << 3)) != 0;
        seat.HasMarker = (source.SeatMarkerName != 0);
        seat.InvalidForPlayer = (source.Flags & (1u << 14)) != 0;
        seat.InvalidForNonPlayer = (source.Flags & (1u << 15)) != 0;

        out.Seats.push_back(seat);
    }

    return out;
}