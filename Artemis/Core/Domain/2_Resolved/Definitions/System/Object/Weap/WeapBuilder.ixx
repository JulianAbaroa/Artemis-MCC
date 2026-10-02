export module Resolved.Definitions.System:Weap;

import :Object;
import Map.Tag.Type;
import Resolved.Definitions.Type;
import std;

export namespace Resolved::Definitions::System
{
	class WeapBuilder
	{
	private:
		using WeapObject = Map::Tag::Type::Weap::Object::WeapObject;
		using ResolvedWeap = Resolved::Definitions::Type::Weap::Weap;

	public:
		WeapBuilder(ObjectBuilder& objectBuilder) :
			m_ObjectBuilder(objectBuilder) {}
		~WeapBuilder() = default;

		auto Build(const WeapObject& weap) -> ResolvedWeap;

	private:
		ObjectBuilder& m_ObjectBuilder;
	};
}