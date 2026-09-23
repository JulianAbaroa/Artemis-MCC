module Resolved.Stats.System;
import :Vehi;

namespace
{
	using Vec3 = Common::Math::Type::Vec3;
	using Seat = Resolved::Stats::Type::Vehi::Seat;
	using SeatType = Resolved::Stats::Type::Vehi::SeatType;
}

namespace Resolved::Stats::System
{
	auto VehiBuilder::Build(const VehiObject& vehi) -> ResolvedVehi
	{
		ResolvedVehi out{};
		out.TagName = vehi.TagName;
		out.Seats.reserve(vehi.Seats.size());

		for (const auto& source : vehi.Seats)
		{
			Seat seat;
			seat.SeatMarkerNameId = source.SeatMarkerName;
			seat.SeatType = static_cast<SeatType>(source.AiSeatType);
			seat.LocalTranslation = {};
			seat.LocalRotation = {.W = 1.0f};
			seat.EntryRadius = source.EntryRadius;
			out.Seats.push_back(seat);
		}

		return out;
	}
}