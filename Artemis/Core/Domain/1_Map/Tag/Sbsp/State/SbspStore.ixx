export module Map.Tag.State:Sbsp.Store;

import Map.Reader.State;

namespace
{
    template <typename TObject>
    using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Sbsp
{
    template <typename TObject>
    class SbspStore : public TagStore<TObject> {};
}