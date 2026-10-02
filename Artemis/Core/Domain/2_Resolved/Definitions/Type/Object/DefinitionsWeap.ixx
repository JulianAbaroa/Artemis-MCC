export module Resolved.Definitions.Type:Weap;

import :Object;
import std;

namespace
{
	using ResolvedObject = Resolved::Definitions::Type::Object::Object;
}

export namespace Resolved::Definitions::Type::Weap
{
	struct Weap
	{
		ResolvedObject Base{};

		float AutoaimRange{};
	};
}