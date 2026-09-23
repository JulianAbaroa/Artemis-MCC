export module Map.Tag.State:Bipd.Store;

import Map.Reader.State;
import std;

namespace
{
    template <typename TObject>
    using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Bipd
{
    template <typename TObject>
    class BipdStore : public TagStore<TObject> {};
}