export module Resolved.Definitions.Type:Ctrl;

import :Object;
import std;

namespace
{
	using ResolvedObject = Resolved::Definitions::Type::Object::Object;
}

export namespace Resolved::Definitions::Type::Ctrl
{
	// Waiting for some system to need this
	struct Ctrl
	{
		ResolvedObject Base{};
	};
}