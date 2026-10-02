export module Resolved.Definitions.Type:Mach;

import :Object;
import std;

namespace
{
	using ResolvedObject = Resolved::Definitions::Type::Object::Object;
}

export namespace Resolved::Definitions::Type::Mach
{
	struct Mach
	{
		ResolvedObject Base{};
	};
}