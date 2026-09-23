export module Map.Tag.State:Zone.Store;

import Map.Reader.State;

namespace
{
    template <typename TObject>
    using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Zone
{
    template <typename TObject>
    class ZoneStore : public TagStore<TObject> {};
}