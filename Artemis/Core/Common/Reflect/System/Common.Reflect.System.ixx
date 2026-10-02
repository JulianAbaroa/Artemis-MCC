export module Common.Reflect.System;

import Common.Reflect.Type;
import std;

namespace
{
	using Common::Reflect::Type::ForEachField;
	using Common::Reflect::Type::Reflectable;
}

export namespace Common::Reflect::System
{
	struct FieldRow
	{
		std::string Path{};
		std::string Value{};
	};

	namespace Detail
	{
		template <typename V>
		auto AppendField(std::string_view path, const V& value, std::vector<FieldRow>& out) -> void
		{
			if constexpr (std::same_as<V, bool>)
			{
				out.push_back({ std::string(path), value ? "true" : "false" });
			}
			else if constexpr (std::same_as<V, std::string>)
			{
				out.push_back({ std::string(path), value });
			}
			else if constexpr (std::is_enum_v<V>)
			{
				out.push_back({ std::string(path),
					std::format("{}", static_cast<std::underlying_type_t<V>>(value)) });
			}
			else if constexpr (std::is_arithmetic_v<V>)
			{
				out.push_back({ std::string(path), std::format("{}", value) });
			}
			else if constexpr (Reflectable<V>)
			{
				ForEachField(value, [&](std::string_view name, auto const& nested) {
					AppendField(std::format("{}.{}", path, name), nested, out);
				});
			}
			else if constexpr (requires { value.size(); value.begin(); value.end(); })
			{
				std::size_t index = 0;
				for (const auto& element : value)
				{
					AppendField(std::format("{}[{}]", path, index), element, out);
					++index;
				}

				if (index == 0) out.push_back({ std::format("{}#count", path), "0" });
			}
			else
			{
				out.push_back({ std::string(path), "<unsupported>" });
			}
		}
	}

	template <typename T> requires Reflectable<T>
	auto DumpToRows(const T& obj, std::string_view rootPath = "") -> std::vector<FieldRow>
	{
		std::vector<FieldRow> rows;
		ForEachField(obj, [&](std::string_view name, auto const& value) {
			Detail::AppendField(rootPath.empty() ? name : std::format("{}.{}", rootPath, name), value, rows);
		});
		return rows;
	}
}