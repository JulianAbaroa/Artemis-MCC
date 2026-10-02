export module Resolved.Definitions.System:Vehi;

import :Object;
import Map.Tag.Type;
import Resolved.Definitions.Type;
import std;

export namespace Resolved::Definitions::System
{
	class VehiBuilder
	{
	private:
		using VehiObject = Map::Tag::Type::Vehi::Object::VehiObject;
		using ResolvedVehi = Resolved::Definitions::Type::Vehi::Vehi;

	public:
		VehiBuilder(ObjectBuilder& objectBuilder) :
			m_ObjectBuilder(objectBuilder) {}
		~VehiBuilder() = default;

		auto Build(const VehiObject& vehi) -> ResolvedVehi;

	private:
		ObjectBuilder& m_ObjectBuilder;
	};
}