export module Map.Tag.State:Sldt.Store;

import Map.Reader.State;

namespace
{
    template <typename TObject>
    using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Sldt
{
    template <typename TObject>
    class SldtStore : public TagStore<TObject> {};
}