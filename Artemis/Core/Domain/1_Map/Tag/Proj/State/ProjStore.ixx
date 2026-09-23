export module Map.Tag.State:Proj.Store;

import Map.Reader.State;

namespace
{
    template <typename TObject>
    using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Proj
{
    template <typename TObject>
    class ProjStore : public TagStore<TObject> {};
}