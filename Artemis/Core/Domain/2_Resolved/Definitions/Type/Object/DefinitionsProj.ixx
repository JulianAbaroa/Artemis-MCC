export module Resolved.Definitions.Type:Proj;

import :Object;
import std;

namespace
{
	using ResolvedObject = Resolved::Definitions::Type::Object::Object;
}

export namespace Resolved::Definitions::Type::Proj
{
	struct Proj
	{
		ResolvedObject Base{};
	};
}