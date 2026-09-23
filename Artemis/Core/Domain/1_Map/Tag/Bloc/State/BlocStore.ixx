export module Map.Tag.State:Bloc.Store;

import Map.Reader.State;

namespace
{
	template <typename TObject>
	using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Bloc
{
	template <typename TObject>
	class BlocStore : public TagStore<TObject> {};
}