export module Resolved.Definitions.System:Proj;

import :Object;
import Map.Tag.Type;
import Resolved.Definitions.Type;
import std;

export namespace Resolved::Definitions::System
{
	class ProjBuilder
	{
	private:
		using ProjObject = Map::Tag::Type::Proj::Object::ProjObject;
		using ResolvedProj = Resolved::Definitions::Type::Proj::Proj;

	public:
		ProjBuilder(ObjectBuilder& objectBuilder) :
			m_ObjectBuilder(objectBuilder) {}
		~ProjBuilder() = default;

		auto Build(const ProjObject& proj) -> ResolvedProj;

	private:
		ObjectBuilder& m_ObjectBuilder;
	};
}