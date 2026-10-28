export module Common.Reflect.Type:Core;

import std;

export namespace Common::Reflect::Type
{
    // Describes a reflected field with its name and a pointer to the member.
    template <typename T, typename M>
    struct FieldDesc
    {
        std::string_view Name{};
        M T::* Ptr{ nullptr };
    };

    // Builds the descriptor of a field. Used by the specializations of Fields.
    template <typename T, typename M>
    constexpr auto MakeField(std::string_view name, M T::* ptr) -> FieldDesc<T, M>
    {
        return FieldDesc<T, M>{ name, ptr };
    }

    // Field list of T. Specialize it with a k_Value tuple of MakeField entries.
    template <typename T>
    struct Fields
    {
    };

    // Satisfied by the types that specialize Fields.
    template <typename T>
    concept Reflectable = requires { Fields<std::remove_cvref_t<T>>::k_Value; };

    // Calls fn(name, value) for every field of obj, in the order of the k_Value tuple.
    template <typename T, typename Fn> requires Reflectable<T>
    constexpr auto ForEachField(const T& obj, Fn&& fn) -> void
    {
        std::apply([&](auto const&... field) {
            (fn(field.Name, obj.*(field.Ptr)), ...);
        }, Fields<T>::k_Value);
    }

    // One flattened field of a reflected object.
    struct FieldRow
    {
        // Dotted path of the field, with [index] for the elements of a container.
        std::string Path{};
        std::string Value{};
    };
}