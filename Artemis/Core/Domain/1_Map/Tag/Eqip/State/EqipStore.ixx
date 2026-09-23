export module Map.Tag.State:Eqip.Store;

import Map.Reader.State;

namespace
{
	template <typename TObject>
	using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Eqip
{
	template <typename TObject>
	class EqipStore : public TagStore<TObject> {};
}