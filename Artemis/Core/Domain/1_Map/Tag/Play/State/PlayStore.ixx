export module Map.Tag.State:Play.Store;

import Map.Reader.State;
import Map.Tag.Type;

namespace
{
    template <typename TObject>
    using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Play
{
    template <typename TObject>
    class PlayStore : public TagStore<TObject>
    {
    private:
        using PlayObject = Map::Tag::Type::Play::Object::PlayObject;

    public:
        auto GetFirstPlay() const -> const PlayObject*
        {
            if (this->m_Map.empty())
            {
                return nullptr;
            }

            return &this->m_Map.begin()->second;
        }
    };
}