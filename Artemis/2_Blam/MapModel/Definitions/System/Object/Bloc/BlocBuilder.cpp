module Blam.MapModel.Definitions.System;
import :Bloc;

namespace Blam::MapModel::Definitions::System
{
    auto BlocBuilder::Build(const BlocObject& bloc) -> Bloc
    {
        Bloc out{};

        out.Base = m_ObjectBuilder.Build(bloc.TagName, bloc.Data, bloc.MultiplayerObject);

        return out;
    }
}