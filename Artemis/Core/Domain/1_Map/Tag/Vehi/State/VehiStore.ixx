export module Map.Tag.State:Vehi.Store;

import Map.Reader.State;

namespace
{
    template <typename TObject>
    using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Vehi
{
    template <typename TObject>
    class VehiStore : public TagStore<TObject> {};
}