export module Resolved.Definitions.State;

import Map.Reader.State;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Map::Reader::State::TagStore;
}

export namespace Resolved::Definitions::State
{
    // Resolved definitions by group, in the same shape as the map layer TagCatalog.
    // note: The builder adds the definitions, then freezes the stores and they are only read.
    class DefinitionsStore
    {
    public:
        // Damage
        TagStore<Type::Jpt::Jpt> m_Jpt{};

        // Level
        TagStore<Type::Sbsp::Sbsp> m_Sbsp{};
        TagStore<Type::Scnr::Scnr> m_Scnr{};
        TagStore<Type::Sddt::Sddt> m_Sddt{};

        // Model
        TagStore<Type::Coll::Coll> m_Coll{};
        TagStore<Type::Hlmt::Hlmt> m_Hlmt{};
        TagStore<Type::Mode::Mode> m_Mode{};
        TagStore<Type::Phmo::Phmo> m_Phmo{};

        // Object
        TagStore<Type::Bipd::Bipd> m_Bipd{};
        TagStore<Type::Bloc::Bloc> m_Bloc{};
        TagStore<Type::Ctrl::Ctrl> m_Ctrl{};
        TagStore<Type::Eqip::Eqip> m_Eqip{};
        TagStore<Type::Mach::Mach> m_Mach{};
        TagStore<Type::Proj::Proj> m_Proj{};
        TagStore<Type::Scen::Scen> m_Scen{};
        TagStore<Type::Vehi::Vehi> m_Vehi{};
        TagStore<Type::Weap::Weap> m_Weap{};

        // Calls the function with the store of every group.
        template <typename F>
        auto ForEach(F&& f) -> void
        {
            f(m_Bipd);
            f(m_Bloc);
            f(m_Coll);
            f(m_Ctrl);
            f(m_Eqip);
            f(m_Hlmt);
            f(m_Jpt);
            f(m_Mach);
            f(m_Mode);
            f(m_Phmo);
            f(m_Proj);
            f(m_Sbsp);
            f(m_Scen);
            f(m_Scnr);
            f(m_Sddt);
            f(m_Vehi);
            f(m_Weap);
        }

        // Ends the build. From now on the stores can only be read.
        auto Freeze() -> void
        {
            this->ForEach([](auto& store) {
                store.Freeze();
            });
        }

        // Removes every definition and unfreezes the stores.
        auto Cleanup() -> void
        {
            this->ForEach([](auto& store) {
                store.Cleanup();
            });
        }
    };
}