export module Map.Tag.State:Scnr.Store;

import Map.Reader.State;

namespace
{
    template <typename TObject>
    using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Scnr
{
    template <typename TObject>
    class ScnrStore : public TagStore<TObject> {};
}