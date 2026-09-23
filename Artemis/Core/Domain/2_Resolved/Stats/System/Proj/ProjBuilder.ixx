export module Resolved.Stats.System:Proj;

import Map.Tag.Type;
import Resolved.Stats.Type;
import std;

export namespace Resolved::Stats::System
{
	class ProjBuilder
	{
	private:
		using ProjObject = Map::Tag::Type::Proj::Object::ProjObject;
		using ResolvedProj = Resolved::Stats::Type::Proj::Proj;
		using ProjType = Resolved::Stats::Type::Proj::Type;

	public:
		ProjBuilder() = default;
		~ProjBuilder() = default;

		auto Build(const ProjObject& proj) -> ResolvedProj;

	private:
		auto DeriveType(const ProjObject& proj) -> ProjType;
		auto DeriveCanBounce(const ProjObject& proj) -> bool;
	};
}