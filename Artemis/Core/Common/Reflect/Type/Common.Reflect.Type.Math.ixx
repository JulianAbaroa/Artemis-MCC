export module Common.Reflect.Type:Math;

import :Core;
import Common.Math.Type;
import std;

namespace
{
	using Vec2 = Common::Math::Type::Vec2;
	using Vec3 = Common::Math::Type::Vec3;
	using Vec4 = Common::Math::Type::Vec4;
	using Triangle = Common::Math::Type::Triangle;
	using Node = Common::Math::Type::Node;
}

export namespace Common::Reflect::Type
{
	template <>
	struct Fields<Vec2>
	{
		static constexpr bool HasFields{ true };
		static constexpr auto Value = std::tuple{
			MakeField("X", &Vec2::X),
			MakeField("Y", &Vec2::Y),
		};
	};

	template <>
	struct Fields<Vec3>
	{
		static constexpr bool HasFields{ true };
		static constexpr auto Value = std::tuple{
			MakeField("X", &Vec3::X),
			MakeField("Y", &Vec3::Y),
			MakeField("Z", &Vec3::Z),
		};
	};

	template <>
	struct Fields<Vec4>
	{
		static constexpr bool HasFields{ true };
		static constexpr auto Value = std::tuple{
			MakeField("X", &Vec4::X),
			MakeField("Y", &Vec4::Y),
			MakeField("Z", &Vec4::Z),
			MakeField("W", &Vec4::W),
		};
	};

	template <>
	struct Fields<Triangle>
	{
		static constexpr bool HasFields{ true };
		static constexpr auto Value = std::tuple{
			MakeField("A", &Triangle::A),
			MakeField("B", &Triangle::B),
			MakeField("C", &Triangle::C),
			MakeField("SurfaceFlags", &Triangle::SurfaceFlags),
			MakeField("Material", &Triangle::Material),
		};
	};

	template <>
	struct Fields<Node>
	{
		static constexpr bool HasFields{ true };
		static constexpr auto Value = std::tuple{
			MakeField("Name", &Node::Name),
			MakeField("ParentIndex", &Node::ParentIndex),
			MakeField("NextSiblingIndex", &Node::NextSiblingIndex),
			MakeField("FirstChildIndex", &Node::FirstChildIndex),
			MakeField("DefaultTranslation", &Node::DefaultTranslation),
			MakeField("DefaultRotation", &Node::DefaultRotation),
			MakeField("InverseScale", &Node::InverseScale),
			MakeField("DoesNotAnimate", &Node::DoesNotAnimate),
		};
	};
}