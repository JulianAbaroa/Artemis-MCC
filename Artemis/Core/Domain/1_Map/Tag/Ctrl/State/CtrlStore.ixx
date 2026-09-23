export module Map.Tag.State:Ctrl.Store;

import Map.Reader.State;

namespace
{
	template <typename TObject>
	using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Ctrl
{
	template <typename TObject>
	class CtrlStore : public TagStore<TObject> {};
}