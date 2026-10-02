module Resolved.Definitions.System;
import :Vehi;

namespace Resolved::Definitions::System
{
	auto VehiBuilder::Build(const VehiObject& vehi) -> ResolvedVehi
	{
		ResolvedVehi out{};

		out.Base = m_ObjectBuilder.Build(vehi.TagName, vehi.Data, vehi.MultiplayerObject);

		return out;
	}
}