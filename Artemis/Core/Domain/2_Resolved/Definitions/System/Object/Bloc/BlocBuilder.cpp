module Resolved.Definitions.System;
import :Bloc;

namespace Resolved::Definitions::System
{
    auto BlocBuilder::Build(const BlocObject& bloc) -> ResolvedBloc
    {
        ResolvedBloc out{};
        out.Base = m_ObjectBuilder.Build(bloc.TagName, bloc.Data, bloc.MultiplayerObject);
        return out;
    }
}