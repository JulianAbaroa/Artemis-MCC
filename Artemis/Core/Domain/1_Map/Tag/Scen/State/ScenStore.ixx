export module Map.Tag.State:Scen.Store;

import Map.Reader.State;

namespace
{
    template <typename TObject>
    using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Scen
{
    template <typename TObject>
    class ScenStore : public TagStore<TObject> {};
}