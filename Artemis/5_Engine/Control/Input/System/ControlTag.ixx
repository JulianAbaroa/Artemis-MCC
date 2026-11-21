export module Engine.Control.Input.System:ControlTag;

import Engine.Base.Hook.System;
import Engine.Control.Input.Type;
import Blam.Layout.Action.Type;
import Blam.Layout.Object.Type;
import Blam.Layout.Player.Type;
import std;

namespace
{
    using Engine::Control::Input::Type::ControlTagLocator;
    using Engine::Control::Input::Type::ResolveTagAddressFunction;
    using Engine::Control::Input::Type::k_PlayerControlTagOffset;
    using Engine::Control::Input::Type::k_TagLoadCallNextOffset;
    using Engine::Control::Input::Type::k_TagLoadCallRelOffset;
    using Engine::Control::Input::Type::k_TagLoadTableNextOffset;
    using Engine::Control::Input::Type::k_TagLoadTableRelOffset;

    using Blam::Layout::Action::Type::Size::k_MaxActionPlayers;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextMappingOffset;
    using Blam::Layout::Player::Type::Mapping::k_PlayerInputUser;
}

export namespace Engine::Control::Input::System
{
    // return: The locator read from the PlayerControlTagLoad match (the address where its signature matched), or an empty one for 0.
    inline auto LocateControlTag(std::uintptr_t tagLoad) -> ControlTagLocator
    {
        if (tagLoad == 0) return {};

        return ControlTagLocator{
            .Table = reinterpret_cast<const std::uintptr_t*>(
                Engine::Base::Hook::System::ResolveRelative(tagLoad, k_TagLoadTableRelOffset, k_TagLoadTableNextOffset)),
            .Resolve = reinterpret_cast<ResolveTagAddressFunction>(
                Engine::Base::Hook::System::ResolveRelative(tagLoad, k_TagLoadCallRelOffset, k_TagLoadCallNextOffset)) };
    }

    // return: The data of the player control tag, or 0 when the locator is not valid or the map has no tag loaded.
    // note: Game thread only. Read it every time: the globals belong to the map.
    inline auto ReadControlTag(const ControlTagLocator& locator) -> std::uintptr_t
    {
        if (!locator.IsValid()) return 0;

        const std::uintptr_t table{ *locator.Table };
        if (table == 0) return 0;

        // note: Between maps the address can be empty. Resolving it would give a pointer to nothing.
        const auto* address{ reinterpret_cast<const std::uint32_t*>(table + k_PlayerControlTagOffset) };
        if (*address == 0 || *address == 0xFFFFFFFF) return 0;

        return locator.Resolve(address);
    }

    // return: The index of the player whose input user is the one given, or nullopt when no player has it.
    // param threadContext: Thread context of the engine, not 0.
    inline auto FindInputPlayer(std::uintptr_t threadContext, std::int32_t inputUser) -> std::optional<std::size_t>
    {
        const std::uintptr_t mapping{ *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ThreadContextMappingOffset) };
        if (mapping == 0) return std::nullopt;

        std::array<std::int32_t, k_MaxActionPlayers> inputUsers{};
        std::memcpy(inputUsers.data(), reinterpret_cast<const void*>(mapping + k_PlayerInputUser), sizeof(inputUsers));

        for (std::size_t player{ 0 }; player < inputUsers.size(); ++player)
        {
            if (inputUsers[player] == inputUser) return player;
        }

        return std::nullopt;
    }

    // return: Thread context of the engine for the current thread, or 0 when the thread local storage index is not known.
    inline auto GetEngineThreadContext(const std::uint32_t* tlsIndex) -> std::uintptr_t
    {
        if (tlsIndex == nullptr) return 0;

        return Engine::Base::Hook::System::ReadThreadContext(*tlsIndex);
    }
}