module DevTools.Viewer.Inspector.System;
import :Name;

import Common.Engine.Type;
import Common.Semantic.Type;
import Perception.Entities.Type;
import DevTools.Gui.Format.System;
import std;

namespace
{
    using DevTools::Gui::Format::System::HexFormater;

    using Common::Engine::Type::k_NoHandle;

    namespace SemanticType = Common::Semantic::Type;
}

namespace DevTools::Viewer::Inspector::System
{
    auto ObjectName::Of(const Tick& tick, std::uint32_t handle) -> std::string
    {
        if (tick.ObjectTable)
        {
            const auto it{ tick.ObjectTable->find(handle) };
            if (it != tick.ObjectTable->end() && !it->second.TagName.empty()) return it->second.TagName;
        }

        const auto* entity{ tick.Entities ? tick.Entities->Find(handle) : nullptr };
        if (entity) return std::string{ SemanticType::NameOf(entity->Function) };

        return HexFormater::Hex32(handle);
    }

    auto ObjectName::Label(const Tick& tick, std::uint32_t handle) -> std::string
    {
        if (handle == k_NoHandle) return "none";

        return std::format("{} {}", ObjectName::Of(tick, handle), HexFormater::Hex32(handle));
    }
}