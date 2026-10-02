module Resolved.Definitions.System;
import :Proj;

namespace Resolved::Definitions::System
{
	auto ProjBuilder::Build(const ProjObject& proj) -> ResolvedProj
	{
		ResolvedProj out{};

		out.Base = m_ObjectBuilder.Build(proj.TagName, proj.Data, proj.MultiplayerObject);

		return out;
	}
}