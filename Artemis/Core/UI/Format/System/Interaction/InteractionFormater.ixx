export module UI.Format.System:Interaction;

import Tables.Interaction.Type;

export namespace UI::Format::System
{
	class InteractionFormater
	{
	private:
		using InteractionKind = Tables::Interaction::Type::Alive::Kind;
		using InteractionDetail = Tables::Interaction::Type::Alive::Detail;

	public:
		InteractionFormater() = default;
		~InteractionFormater() = default;

		static auto InteractionKindToString(InteractionKind kind) -> const char*;
		static auto InteractionDetailToString(InteractionKind kind,
			InteractionDetail detail) -> const char*;
	};
}