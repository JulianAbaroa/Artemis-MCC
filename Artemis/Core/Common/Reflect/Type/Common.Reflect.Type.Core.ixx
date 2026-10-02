export module Common.Reflect.Type:Core;

import std;

export namespace Common::Reflect::Type
{
	template <typename T, typename M>
	struct FieldDesc
	{
		std::string_view Name;
		M T::* Ptr;
	};

	template <typename T, typename M>
	constexpr auto MakeField(std::string_view name, M T::* ptr) -> FieldDesc<T, M>
	{
		return FieldDesc<T, M>{ name, ptr };
	}

	template <typename T>
	struct Fields
	{
		static constexpr bool HasFields{ false };
	};

	template <typename T>
	concept Reflectable = Fields<std::remove_cvref_t<T>>::HasFields;

	template <typename T, typename Fn> requires Reflectable<T>
	constexpr auto ForEachField(const T& obj, Fn&& fn) -> void
	{
		std::apply([&](auto const&... field) {
			(fn(field.Name, obj.*(field.Ptr)), ...);
			}, Fields<T>::Value);
	}
}