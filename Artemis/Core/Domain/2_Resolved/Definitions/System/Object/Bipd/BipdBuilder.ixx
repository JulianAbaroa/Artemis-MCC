export module Resolved.Definitions.System:Bipd;

import :Object;
import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
	class BipdBuilder
	{
	private:
		using BipdObject = Map::Tag::Type::Bipd::Object::BipdObject;
		using ResolvedBipd = Resolved::Definitions::Type::Bipd::Bipd;

	public:
		explicit BipdBuilder(ObjectBuilder& objectBuilder) :
			m_ObjectBuilder(objectBuilder) {}
		~BipdBuilder() = default;

		auto Build(const BipdObject& bipd) -> ResolvedBipd;

	private:
		ObjectBuilder& m_ObjectBuilder;
	};
}