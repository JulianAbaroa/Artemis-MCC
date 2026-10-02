export module Resolved.Definitions.Reflect:Vehi;

import :Object;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
	using Vehi = Resolved::Definitions::Type::Vehi::Vehi;
}

export namespace Common::Reflect::Type
{
	template <>
	struct Fields<Vehi>
	{
		static constexpr bool HasFields = true;
		static constexpr auto Value = std::tuple{
			MakeField("Base", &Vehi::Base),
		};
	};
}
