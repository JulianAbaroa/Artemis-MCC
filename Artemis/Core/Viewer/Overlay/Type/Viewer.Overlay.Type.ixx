export module Viewer.Overlay.Type;

import std;

export namespace Viewer::Overlay::Type
{
	enum class Mode : std::uint8_t
	{
		Default = 0,
		Collidable,
		Health,
		Fixture,
		Affordance,

		Count
	};
}