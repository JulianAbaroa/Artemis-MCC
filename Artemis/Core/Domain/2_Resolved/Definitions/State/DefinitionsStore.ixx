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
    struct DefinitionsStore
    {
        // Damage
        TagStore<Type::Jpt::Jpt> Jpt{};

        // Level
        TagStore<Type::Sbsp::Sbsp> Sbsp{};
        TagStore<Type::Scnr::Scnr> Scnr{};

        // Model
        TagStore<Type::Coll::Coll> Coll{};
        TagStore<Type::Hlmt::Hlmt> Hlmt{};
        TagStore<Type::Mode::Mode> Mode{};

        // Object
        TagStore<Type::Bipd::Bipd> Bipd{};
        TagStore<Type::Bloc::Bloc> Bloc{};
        TagStore<Type::Ctrl::Ctrl> Ctrl{};
        TagStore<Type::Eqip::Eqip> Eqip{};
        TagStore<Type::Mach::Mach> Mach{};
        TagStore<Type::Proj::Proj> Proj{};
        TagStore<Type::Scen::Scen> Scen{};
        TagStore<Type::Vehi::Vehi> Vehi{};
        TagStore<Type::Weap::Weap> Weap{};

        // Calls the function with the store of every group.
        template <typename F>
        auto ForEach(F&& f) -> void
        {
            f(Bipd);
            f(Bloc);
            f(Coll);
            f(Ctrl);
            f(Eqip);
            f(Hlmt);
            f(Jpt);
            f(Mach);
            f(Mode);
            f(Proj);
            f(Sbsp);
            f(Scen);
            f(Scnr);
            f(Vehi);
            f(Weap);
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