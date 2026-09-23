export module Map.Tag.State:Lbsp.Store;

import Map.Reader.State;

namespace
{
	template <typename TObject>
	using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Lbsp
{
	template <typename TObject>
	class LbspStore : public TagStore<TObject> {};
}