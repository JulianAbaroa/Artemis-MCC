export module Map.Tag.State:Coll.Store;

import Map.Reader.State;

namespace
{
	template <typename TObject>
	using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Coll
{
	template <typename TObject>
	class CollStore : public TagStore<TObject> {};
}