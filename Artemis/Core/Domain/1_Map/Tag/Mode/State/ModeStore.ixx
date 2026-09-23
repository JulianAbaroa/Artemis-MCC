export module Map.Tag.State:Mode.Store;

import Map.Reader.State;

namespace
{
	template <typename TObject>
	using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Mode
{
	template <typename TObject>
	class ModeStore : public TagStore<TObject> {};
}