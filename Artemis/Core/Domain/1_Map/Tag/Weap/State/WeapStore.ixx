export module Map.Tag.State:Weap.Store;

import Map.Reader.State;

namespace
{
    template <typename TObject>
    using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Weap
{
    template <typename TObject>
    class WeapStore : public TagStore<TObject> {};
}