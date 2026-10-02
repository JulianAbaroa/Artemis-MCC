module Resolved.Definitions.System;
import :Weap;

namespace Resolved::Definitions::System
{
	auto WeapBuilder::Build(const WeapObject& weap) -> ResolvedWeap
	{
		ResolvedWeap out{};

		out.Base = m_ObjectBuilder.Build(weap.TagName, weap.Data, weap.MultiplayerObject);

		out.AutoaimRange = weap.Data.AutoaimRange;

		return out;
	}
}