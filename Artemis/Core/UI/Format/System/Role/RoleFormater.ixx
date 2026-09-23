export module UI.Format.System:Role;

import Relations.Classifier.Type;

export namespace UI::Format::System
{
	class RoleFormater
	{
	private:
		using Role = Relations::Classifier::Type::Role;

	public:
		RoleFormater() = default;
		~RoleFormater() = default;

		static auto RoleToString(Role role) -> const char*;
	};
}