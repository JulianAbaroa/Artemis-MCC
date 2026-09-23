export module Map.Tag.State:Mach.Store;

import Map.Reader.State;

namespace
{
	template <typename TObject>
	using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Mach
{
	template <typename TObject>
	class MachStore : public TagStore<TObject> {};
}