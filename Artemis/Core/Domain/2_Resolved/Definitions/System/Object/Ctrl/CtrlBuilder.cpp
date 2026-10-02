module Resolved.Definitions.System;
import :Ctrl;

namespace Resolved::Definitions::System
{
    auto CtrlBuilder::Build(const CtrlObject& ctrl) -> ResolvedCtrl
    {
        ResolvedCtrl out{};

        out.Base = m_ObjectBuilder.Build(ctrl.TagName, ctrl.Data, ctrl.MultiplayerObject);

        return out;
    }
}