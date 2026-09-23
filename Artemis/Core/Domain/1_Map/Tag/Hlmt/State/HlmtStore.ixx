export module Map.Tag.State:Hlmt.Store;

import Map.Reader.State;
import std;

namespace
{
    template <typename TObject>
    using TagStore = Map::Reader::State::TagStore<TObject>;
}

export namespace Map::Tag::State::Hlmt
{
    template <typename TObject>
    class HlmtStore : public TagStore<TObject>
    {
    public:
        auto GetTagNames() const -> std::vector<std::string>
        {
            std::vector<std::string> names;
            names.reserve(this->m_Map.size());

            for (const auto& kv : this->m_Map)
            {
                names.push_back(kv.first);
            }

            return names;
        }
    };
}