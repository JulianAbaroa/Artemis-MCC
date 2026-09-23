export module Map.Tag.State:Phmo.Store;

import Map.Reader.State;

namespace
{
	template <typename TObject>
	using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Phmo
{
	template <typename TObject>
	class PhmoStore : public TagStore<TObject> {};
}