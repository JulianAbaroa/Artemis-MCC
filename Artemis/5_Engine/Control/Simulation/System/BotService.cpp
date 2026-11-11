module Engine.Control.Simulation.System;

import Common.Engine.Type;
import Common.Math.Type;
import Control.Action.Type;
import Engine.Base.Memory.Type;
import Engine.Base.Hook.System;
import Blam.Layout.Object.Type;
import std;

namespace
{
    namespace Signature = Engine::Base::Memory::Type::Signature;

    using Engine::Control::Simulation::Type::SessionSnapshot;
    using Engine::Control::Simulation::Type::SessionPlayer;
    using Engine::Control::Simulation::Type::MachineIdentifier;
    using Engine::Control::Simulation::Type::Bot;
    using Engine::Control::Simulation::Type::FakeMachine;
    using Engine::Control::Simulation::Type::SetMachinesFunction;
    using Engine::Control::Simulation::Type::ApplyPlayerAddedFunction;
    using Engine::Control::Simulation::Type::ApplyPlayerUpdateFunction;
    using Engine::Control::Simulation::Type::ApplyPlayerConfigurationFunction;
    using Engine::Control::Simulation::Type::TeamRequest;
    using Engine::Control::Simulation::Type::RosterBot;
    using Engine::Control::Simulation::Type::RosterPlan;
    using Engine::Control::Simulation::Type::RosterResult;
    using Engine::Control::Simulation::Type::PlanRoster;
    using Engine::Control::Simulation::Type::GetRosterStep;
    using Engine::Control::Simulation::Type::RosterStep;
    using Engine::Control::Simulation::Type::ToText;
    using Engine::Base::Hook::System::ReadRoundIndex;
    using Engine::Base::Hook::System::ReadRoundState;
    using Engine::Control::Simulation::Type::GatherInteractionsFunction;
    using Engine::Control::Simulation::Type::UnitSelectorFunction;
    using Engine::Control::Simulation::Type::NextZoomLevelFunction;
    using Engine::Control::Simulation::Type::CanUnitZoomFunction;
    using ::Control::Action::Type::UnitClientState;

    using Engine::Control::Simulation::Type::k_MaxMachines;
    using Common::Engine::Type::k_MaxPlayers;
    using Engine::Control::Simulation::Type::k_MaxPlayersPerMachine;
    using Engine::Control::Simulation::Type::k_ObjectsSlot;
    using Engine::Control::Simulation::Type::k_PlayersSlot;
    using Engine::Control::Simulation::Type::k_PlayersGlobalsSlot;
    using Engine::Control::Simulation::Type::k_GameOptionsSlot;
    using Blam::Layout::Object::Type::Constant::k_DataArrayCapacity;
    using Blam::Layout::Object::Type::Constant::k_DataArrayElementSize;
    using Blam::Layout::Object::Type::Constant::k_DataArrayData;
    using Engine::Control::Simulation::Type::k_PlayerSalt;
    using Engine::Control::Simulation::Type::k_PlayerFlags;
    using Engine::Control::Simulation::Type::k_PlayerFlagLeft;
    using Engine::Control::Simulation::Type::k_PlayerIdentifier;
    using Engine::Control::Simulation::Type::k_PlayerMachineIdentifier;
    using Engine::Control::Simulation::Type::k_PlayerMachineIndex;
    using Engine::Control::Simulation::Type::k_PlayerUserIndex;
    using Engine::Control::Simulation::Type::k_PlayerController;
    using Engine::Control::Simulation::Type::k_PlayerUnit;
    using Engine::Control::Simulation::Type::k_ObjectHeaderSize;
    using Engine::Control::Simulation::Type::k_ObjectHeaderData;
    using Engine::Control::Simulation::Type::k_UnitFlags;
    using Engine::Control::Simulation::Type::k_UnitActionSlot;
    using Engine::Control::Simulation::Type::k_PlayerGamerTag;
    using Engine::Control::Simulation::Type::k_PlayerTeam;
    using Engine::Control::Simulation::Type::k_PlayerConfiguration;
    using Engine::Control::Simulation::Type::k_ConfigurationTeam;
    using Common::Engine::Type::k_MaxTeams;
    using Common::Engine::Type::k_GamerTagLength;
    using Engine::Control::Simulation::Type::k_MachineMask;
    using Engine::Control::Simulation::Type::k_MachineIdentifiers;
    using Engine::Control::Simulation::Type::k_LocalMachineIndex;
    using Engine::Control::Simulation::Type::k_OptionsPlayers;
    using Engine::Control::Simulation::Type::k_OptionsPlayerSize;
    using Engine::Control::Simulation::Type::k_OptionsPlayerConfiguration;
    using Engine::Control::Simulation::Type::k_PlayerConfigurationSize;
    using Engine::Control::Simulation::Type::k_PlayerUpdateSize;
    using Engine::Control::Simulation::Type::k_UpdateMachine;
    using Engine::Control::Simulation::Type::k_UpdateUserIndex;
    using Engine::Control::Simulation::Type::k_UpdateController;
    using Engine::Control::Simulation::Type::k_UpdateJoinedInProgress;
    using Engine::Control::Simulation::Type::k_UpdateConfiguration;
    using Engine::Control::Simulation::Type::k_PlayerUpdateAdded;
    using Engine::Control::Simulation::Type::k_PlayerUpdateLeft;
    using Engine::Control::Simulation::Type::k_PlayerUpdateRemove;
    using Engine::Control::Simulation::Type::MakePlayerUpdate;
    using Engine::Control::Simulation::Type::FindFreeBotNumber;
    using Engine::Control::Simulation::Type::k_InteractionRecordCount;
    using Engine::Control::Simulation::Type::k_InteractionRecordSize;
    using Engine::Control::Simulation::Type::k_InteractionRecordObject;
    using Engine::Control::Simulation::Type::k_UnitWeaponSlot;
    using Engine::Control::Simulation::Type::k_UnitInventoryVersion;
    using Engine::Control::Simulation::Type::k_UnitGrenadeType;
    using Engine::Control::Simulation::Type::k_UnitZoomLevel;
    using Engine::Control::Simulation::Type::k_ObjectParent;
    using Engine::Control::Simulation::Type::k_UnitSeat;
    using Engine::Control::Simulation::Type::k_UnitDesiredAim;
    using Common::Math::Type::k_TwoPi;

    // Step of the weapon and grenade selectors: the next one, as a local client asks for it (0x61a38).
    constexpr std::int16_t k_SelectorStep{ 1 };

    // High bytes of the identifier of a bot ("ARTE"). The low bytes are its number.
    constexpr std::uint64_t k_BotIdentifierBase{ 0x4152544500000000 };

    // First bytes of the identifier of a fake machine ("ARTM"). The last byte is its index.
    constexpr std::array<std::uint8_t, 4> k_FakeMachinePrefix{ 0x41, 0x52, 0x54, 0x4D };

    // return: The value of type T at the address.
    template<typename T>
    auto Read(std::uintptr_t address) -> T
    {
        T value{};
        std::memcpy(&value, reinterpret_cast<const void*>(address), sizeof(T));
        return value;
    }

    // return: The address of the player datum of the index, or 0 if the slot is outside the array.
    auto GetPlayerDatum(std::uintptr_t players, std::size_t index) -> std::uintptr_t
    {
        const auto capacity{ Read<std::int32_t>(players + k_DataArrayCapacity) };
        if (capacity <= 0 || index >= static_cast<std::size_t>(capacity)) return 0;

        const auto elementSize{ Read<std::int32_t>(players + k_DataArrayElementSize) };
        const auto data{ Read<std::uintptr_t>(players + k_DataArrayData) };
        if (elementSize <= 0 || data == 0) return 0;

        return data + (index * static_cast<std::size_t>(elementSize));
    }

    // return: The handle of the player in that slot (salt in the high half, index in the low half).
    auto MakePlayerHandle(std::uint16_t salt, std::size_t index) -> std::uint32_t
    {
        return (static_cast<std::uint32_t>(salt) << 16) | static_cast<std::uint32_t>(index);
    }

    // Datum of a player slot that is in use, with the salt that makes its handle.
    struct PlayerSlot
    {
        std::uintptr_t Datum{};
        std::uint16_t Salt{};
    };

    // return: The slot of the player, or nothing when there is no thread context, no player table or the slot is free (salt 0).
    auto FindPlayerSlot(std::uintptr_t threadContext, std::size_t playerIndex) -> std::optional<PlayerSlot>
    {
        if (threadContext == 0) return std::nullopt;

        const auto players{ Read<std::uintptr_t>(threadContext + k_PlayersSlot) };
        if (players == 0) return std::nullopt;

        const std::uintptr_t datum{ GetPlayerDatum(players, playerIndex) };
        const auto salt{ datum != 0 ? Read<std::uint16_t>(datum + k_PlayerSalt) : std::uint16_t{ 0 } };
        if (salt == 0) return std::nullopt;

        return PlayerSlot{ .Datum = datum, .Salt = salt };
    }

    // return: The address of the data of the object, or 0 if the handle is not a live object.
    auto GetObjectData(std::uintptr_t threadContext, std::uint32_t handle) -> std::uintptr_t
    {
        if (handle == 0xFFFFFFFF) return 0;

        const auto objects{ Read<std::uintptr_t>(threadContext + k_ObjectsSlot) };
        if (objects == 0) return 0;

        const std::size_t index{ handle & 0xFFFF };
        const auto capacity{ Read<std::int32_t>(objects + k_DataArrayCapacity) };
        if (capacity <= 0 || index >= static_cast<std::size_t>(capacity)) return 0;

        const auto headers{ Read<std::uintptr_t>(objects + k_DataArrayData) };
        if (headers == 0) return 0;

        return Read<std::uintptr_t>(headers + (index * k_ObjectHeaderSize) + k_ObjectHeaderData);
    }

    // Replaces every copy of the gamertag inside a player configuration with the name of the bot.
    // return: How many copies were replaced (the configuration of Reach has two, at +0x00 and +0x58).
    auto RenameConfiguration(std::span<std::uint8_t> configuration, std::span<const wchar_t> gamerTag, std::wstring_view name)
        -> std::size_t
    {
        std::size_t length{ 0 };
        while (length < gamerTag.size() && gamerTag[length] != L'\0') ++length;
        if (length == 0) return 0;

        const std::size_t bytes{ length * sizeof(wchar_t) };
        const auto* pattern{ reinterpret_cast<const std::uint8_t*>(gamerTag.data()) };
        std::size_t replaced{ 0 };

        for (std::size_t offset{ 0 }; offset + bytes <= configuration.size(); offset += sizeof(wchar_t))
        {
            if (std::memcmp(configuration.data() + offset, pattern, bytes) != 0) continue;

            // note: The whole gamertag field is rewritten, so the old name does not leak past the new terminator.
            const std::size_t field{ (std::min)(k_GamerTagLength * sizeof(wchar_t), configuration.size() - offset) };
            std::memset(configuration.data() + offset, 0, field);

            const std::size_t copy{ (std::min)(name.size() * sizeof(wchar_t), field - sizeof(wchar_t)) };
            std::memcpy(configuration.data() + offset, name.data(), copy);

            ++replaced;
            offset += field - sizeof(wchar_t);
        }

        return replaced;
    }

    // Replaces every copy of the identifier of the local player inside a player configuration (Reach keeps one at +0x20).
    // return: How many copies were replaced.
    auto ReplaceIdentifier(std::span<std::uint8_t> configuration, std::uint64_t from, std::uint64_t to) -> std::size_t
    {
        std::size_t replaced{ 0 };

        for (std::size_t offset{ 0 }; offset + sizeof(from) <= configuration.size(); ++offset)
        {
            if (std::memcmp(configuration.data() + offset, &from, sizeof(from)) != 0) continue;

            std::memcpy(configuration.data() + offset, &to, sizeof(to));
            ++replaced;
            offset += sizeof(from) - 1;
        }

        return replaced;
    }
}

namespace Engine::Control::Simulation::System
{
    auto BotService::Update(std::uintptr_t threadContext) -> void
    {
        if (threadContext == 0) return;

        this->PublishSnapshot(threadContext);
        this->UpdateRoster(threadContext);

        for (const std::size_t number : m_BotStore.TakeRemoveRequests())
        {
            this->RemoveBot(threadContext, number);
        }

        for (const std::optional<std::int8_t> team : m_BotStore.TakeRequests())
        {
            if (!this->AddBot(threadContext, team)) break;
        }

        for (const TeamRequest& request : m_BotStore.TakeTeamRequests())
        {
            this->ApplyTeam(threadContext, request);
        }
    }

    auto BotService::PublishSnapshot(std::uintptr_t threadContext) -> void
    {
        SessionSnapshot snapshot{};
        snapshot.Sequence = ++m_Sequence;

        const auto globals{ Read<std::uintptr_t>(threadContext + k_PlayersGlobalsSlot) };
        if (globals != 0)
        {
            snapshot.MachineMask = Read<std::uint32_t>(globals + k_MachineMask);
            std::memcpy(snapshot.Machines.data(), reinterpret_cast<const void*>(globals + k_MachineIdentifiers),
                sizeof(snapshot.Machines));
            snapshot.LocalMachineIndex = Read<std::int32_t>(globals + k_LocalMachineIndex);
        }

        const auto players{ Read<std::uintptr_t>(threadContext + k_PlayersSlot) };
        if (players != 0)
        {
            for (std::size_t index{ 0 }; index < k_MaxPlayers; ++index)
            {
                const std::uintptr_t datum{ GetPlayerDatum(players, index) };
                if (datum == 0) break;

                const auto salt{ Read<std::uint16_t>(datum + k_PlayerSalt) };
                if (salt == 0) continue;

                SessionPlayer& player{ snapshot.Players[index] };
                player.IsValid = true;
                player.Handle = MakePlayerHandle(salt, index);
                player.Flags = Read<std::uint32_t>(datum + k_PlayerFlags);
                player.Identifier = Read<std::uint64_t>(datum + k_PlayerIdentifier);
                player.MachineIndex = Read<std::int16_t>(datum + k_PlayerMachineIndex);
                player.UserIndex = Read<std::int32_t>(datum + k_PlayerUserIndex);
                player.Controller = Read<std::int32_t>(datum + k_PlayerController);
                player.Team = Read<std::int8_t>(datum + k_PlayerTeam);
                player.UnitHandle = Read<std::uint32_t>(datum + k_PlayerUnit);
                const std::uintptr_t unit{ GetObjectData(threadContext, player.UnitHandle) };
                if (unit != 0) player.UnitFlags = Read<std::uint32_t>(unit + k_UnitFlags);
                player.Client = this->GetUnitClientState(threadContext, index);
                std::memcpy(player.GamerTag.data(), reinterpret_cast<const void*>(datum + k_PlayerGamerTag), sizeof(player.GamerTag));
            }
        }

        const auto options{ Read<std::uintptr_t>(threadContext + k_GameOptionsSlot) };
        if (options != 0)
        {
            std::memcpy(snapshot.Options.data(), reinterpret_cast<const void*>(options + k_OptionsPlayers), sizeof(snapshot.Options));
        }

        m_BotStore.PublishSnapshot(snapshot);
    }

    auto BotService::AddBot(std::uintptr_t threadContext, std::optional<std::int8_t> team) -> bool
    {
        if (!m_IsResolved)
        {
            m_Log.Message("[BotService] ERROR: Failed to add a bot: the engine functions were not found.");
            return false;
        }

        // note: The lowest free number, so a bot removed and added again gets the same machine, user and controller.
        std::vector<Bot> bots{};
        m_BotStore.CopyBots(bots);

        std::vector<std::size_t> usedNumbers{};
        for (const Bot& bot : bots) usedNumbers.push_back(bot.Number);

        const std::optional<std::size_t> freeNumber{ FindFreeBotNumber(usedNumbers) };
        if (!freeNumber)
        {
            m_Log.Message("[BotService] WARNING: No machine slot left for another bot.");
            return false;
        }

        const auto players{ Read<std::uintptr_t>(threadContext + k_PlayersSlot) };
        const auto globals{ Read<std::uintptr_t>(threadContext + k_PlayersGlobalsSlot) };
        const auto options{ Read<std::uintptr_t>(threadContext + k_GameOptionsSlot) };
        if (players == 0 || globals == 0 || options == 0)
        {
            m_Log.Message("[BotService] ERROR: Failed to read the players, the machines or the game options of the engine.");
            return false;
        }

        // The first player of the local machine (user 0) lends its configuration. Free slots are taken from the top.
        const auto localMachine{ Read<std::int32_t>(globals + k_LocalMachineIndex) };
        std::optional<std::size_t> localPlayer{};
        std::optional<std::size_t> freeIndex{};

        for (std::size_t index{ 0 }; index < k_MaxPlayers; ++index)
        {
            const std::uintptr_t datum{ GetPlayerDatum(players, index) };
            if (datum == 0) break;

            if (Read<std::uint16_t>(datum + k_PlayerSalt) == 0)
            {
                freeIndex = index;
                continue;
            }

            if (!localPlayer && Read<std::int16_t>(datum + k_PlayerMachineIndex) == localMachine
                && Read<std::int32_t>(datum + k_PlayerUserIndex) == 0)
            {
                localPlayer = index;
            }
        }

        if (!localPlayer || !freeIndex)
        {
            m_Log.Message("[BotService] ERROR: Failed to find {}.", !localPlayer ? "the local player" : "a free player slot");
            return false;
        }

        const std::size_t number{ *freeNumber };
        const std::size_t position{ number - 1 };
        const std::size_t machineIndex{ (k_MaxMachines - 1) - (position / k_MaxPlayersPerMachine) };
        const std::size_t slot{ position % k_MaxPlayersPerMachine };

        if (!this->EnsureFakeMachine(globals, machineIndex)) return false;

        const auto playerIndex{ static_cast<std::int32_t>(*freeIndex) };
        const std::uint64_t identifier{ k_BotIdentifierBase | number };
        std::array<std::uint8_t, k_PlayerUpdateSize> update{ MakePlayerUpdate(playerIndex, identifier, k_PlayerUpdateAdded) };
        const auto userIndex{ static_cast<std::int32_t>(slot) };

        MachineIdentifier machine{};
        std::memcpy(machine.Bytes.data(), reinterpret_cast<const void*>(globals + k_MachineIdentifiers
            + (machineIndex * sizeof(MachineIdentifier))), sizeof(machine));

        std::memcpy(update.data() + k_UpdateMachine, machine.Bytes.data(), sizeof(machine));
        std::memcpy(update.data() + k_UpdateUserIndex, &userIndex, sizeof(userIndex));
        std::memcpy(update.data() + k_UpdateController, &userIndex, sizeof(userIndex));
        update[k_UpdateJoinedInProgress] = 1;

        std::span<std::uint8_t> configuration{ update.data() + k_UpdateConfiguration, k_PlayerConfigurationSize };
        std::memcpy(configuration.data(), reinterpret_cast<const void*>(options + k_OptionsPlayers
            + (*localPlayer * k_OptionsPlayerSize) + k_OptionsPlayerConfiguration), k_PlayerConfigurationSize);

        const std::uintptr_t localDatum{ GetPlayerDatum(players, *localPlayer) };

        std::array<wchar_t, k_GamerTagLength> gamerTag{};
        std::memcpy(gamerTag.data(), reinterpret_cast<const void*>(localDatum + k_PlayerGamerTag), sizeof(gamerTag));

        // note: "Mini-Place 16" fits the 15 characters of a gamertag (user request 2026-10-21).
        const std::wstring name{ std::format(L"Mini-Place {}", number) };
        const std::size_t names{ RenameConfiguration(configuration, gamerTag, name) };
        const std::size_t identifiers{ ReplaceIdentifier(configuration, Read<std::uint64_t>(localDatum + k_PlayerIdentifier),
            identifier) };

        if (team) configuration[k_ConfigurationTeam] = static_cast<std::uint8_t>(*team);

        if (!m_ApplyPlayerAdded(update.data()))
        {
            m_Log.Message("[BotService] ERROR: The engine refused bot {} (player {}, machine {}, user {}).", number,
                *freeIndex, machineIndex, slot);
            return false;
        }

        // EXPERIMENT (A9 sec. 13.8): the engine only activates a remote player when the simulation world has a view of its machine,
        // and a fake machine has none, so the bot stayed inactive (never spawned). The player keeps the index of its fake machine (+0x1A,
        // used for the local mapping and players_set_machines) but takes the identifier of the local machine, which the activation
        // check treats as active. The player was already created as remote, so the simulation world keeps it remote.
        const std::uintptr_t botDatum{ GetPlayerDatum(players, *freeIndex) };
        std::memcpy(reinterpret_cast<void*>(botDatum + k_PlayerMachineIdentifier), reinterpret_cast<const void*>(globals
            + k_MachineIdentifiers + (static_cast<std::size_t>(localMachine) * sizeof(MachineIdentifier))), sizeof(MachineIdentifier));

        m_BotStore.AddBot(Bot{ .Number = number, .PlayerIndex = *freeIndex, .MachineIndex = machineIndex, .Slot = slot,
            .Identifier = identifier });

        m_Log.Message("[BotService] INFO: Bot {} added as player {} on machine {} (user {}). Configuration: {} names and {}"
            " identifiers replaced.", number, *freeIndex, machineIndex, slot, names, identifiers);
        return true;
    }

    auto BotService::RemoveBot(std::uintptr_t threadContext, std::size_t number) -> bool
    {
        std::vector<Bot> bots{};
        m_BotStore.CopyBots(bots);

        const auto found{ std::ranges::find(bots, number, &Bot::Number) };
        if (found == bots.end())
        {
            m_Log.Message("[BotService] WARNING: Bot {} does not exist, nothing was removed.", number);
            return false;
        }

        const Bot bot{ *found };
        if (!m_ApplyPlayerUpdate)
        {
            m_Log.Message("[BotService] ERROR: Failed to remove bot {}: the engine function was not found.", number);
            return false;
        }

        // The slot must still hold this bot. If it is free or taken by another player, the bot is gone and nobody else is touched.
        auto slot{ FindPlayerSlot(threadContext, bot.PlayerIndex) };
        if (!slot || Read<std::uint64_t>(slot->Datum + k_PlayerIdentifier) != bot.Identifier)
        {
            m_Log.Message("[BotService] WARNING: Bot {} is already gone from player {}.", number, bot.PlayerIndex);
            this->ForgetBot(bot);
            return true;
        }

        const auto playerIndex{ static_cast<std::int32_t>(bot.PlayerIndex) };

        // note: The "left" update is refused if the flag is already set (a retry after a failed "remove").
        if ((Read<std::uint32_t>(slot->Datum + k_PlayerFlags) & k_PlayerFlagLeft) == 0)
        {
            const auto left{ MakePlayerUpdate(playerIndex, bot.Identifier, k_PlayerUpdateLeft) };
            if (!m_ApplyPlayerUpdate(left.data()))
            {
                m_Log.Message("[BotService] ERROR: Failed to remove bot {} (player {}): the engine refused the left update.", number,
                    bot.PlayerIndex);
                return false;
            }

            slot = FindPlayerSlot(threadContext, bot.PlayerIndex);
            if (slot && (Read<std::uint32_t>(slot->Datum + k_PlayerFlags) & k_PlayerFlagLeft) == 0)
            {
                m_Log.Message("[BotService] ERROR: Failed to remove bot {} (player {}): the left flag was not set.", number,
                    bot.PlayerIndex);
                return false;
            }
        }

        if (slot)
        {
            const auto remove{ MakePlayerUpdate(playerIndex, bot.Identifier, k_PlayerUpdateRemove) };
            if (!m_ApplyPlayerUpdate(remove.data()))
            {
                m_Log.Message("[BotService] ERROR: Failed to remove bot {} (player {}): the engine refused the remove update.", number,
                    bot.PlayerIndex);
                return false;
            }

            if (FindPlayerSlot(threadContext, bot.PlayerIndex))
            {
                m_Log.Message("[BotService] ERROR: Failed to remove bot {} (player {}): the slot is still taken.", number,
                    bot.PlayerIndex);
                return false;
            }
        }

        this->ForgetBot(bot);

        m_Log.Message("[BotService] INFO: Bot {} removed from player {}.", number, bot.PlayerIndex);
        return true;
    }

    auto BotService::ForgetBot(const Bot& bot) -> void
    {
        m_PlayerActionStore.SetOverride(bot.PlayerIndex, std::nullopt);
        m_BotStore.RemoveBot(bot.Number);
    }

    auto BotService::UpdateRoster(std::uintptr_t threadContext) -> void
    {
        const bool isStarted{ m_BotStore.IsMatchStarted() };
        const bool hasPending{ m_BotStore.HasPendingRoster() };
        const RosterStep step{ GetRosterStep(isStarted, hasPending, hasPending ? ReadRoundState(threadContext) : std::nullopt) };

        if (step == RosterStep::Nothing) return;

        // note: At the first tick a request made before it (the lobby) is the roster of the load. It only becomes the active roster if
        // it is applied. Between rounds it is the request made in the match.
        const bool isLoad{ step == RosterStep::Load };
        if (isLoad) m_BotStore.SetMatchStarted();

        const std::optional<RosterRequest> request{ isLoad ? m_BotStore.TakeLoadRoster() : m_BotStore.TakePendingRoster() };
        if (!request) return;

        const RosterResult result{ this->ApplyRoster(threadContext, request->Value, isLoad ? RosterOutcome::AtLoad : RosterOutcome::Applied) };
        const bool isApplied{ result.Outcome != RosterOutcome::Rejected };

        m_BotStore.FinishRoster(*request, result, isApplied);

        if (!isApplied) return;

        if (isLoad) m_Log.Message("[BotService] INFO: Roster applied at load.");
        else        m_Log.Message("[BotService] INFO: Roster applied between rounds {}.", result.Round);
    }

    auto BotService::ApplyRoster(std::uintptr_t threadContext, const Roster& roster, RosterOutcome outcome) -> RosterResult
    {
        const std::int16_t round{ ReadRoundIndex(threadContext).value_or(-1) };
        const auto reject = [&](const std::string& reason) {
            m_Log.Message("[BotService] WARNING: Roster {} was not applied: {}.", ToText(roster), reason);
            return RosterResult{ .Outcome = RosterOutcome::Rejected, .Round = round, .Reason = reason };
        };

        const auto players{ Read<std::uintptr_t>(threadContext + k_PlayersSlot) };
        const auto globals{ Read<std::uintptr_t>(threadContext + k_PlayersGlobalsSlot) };
        if (!m_IsResolved || players == 0 || globals == 0) return reject("the players or the engine functions are not available");

        std::vector<Bot> bots{};
        m_BotStore.CopyBots(bots);

        // A bot already gone from the engine goes in with no team: the plan only removes it, and the removal forgets it.
        std::vector<RosterBot> current{};
        std::size_t botsInEngine{ 0 };
        for (const Bot& bot : bots)
        {
            const auto slot{ FindPlayerSlot(threadContext, bot.PlayerIndex) };
            const bool isInEngine{ slot && Read<std::uint64_t>(slot->Datum + k_PlayerIdentifier) == bot.Identifier };

            if (isInEngine) ++botsInEngine;
            current.push_back(RosterBot{ .Number = bot.Number, .PlayerIndex = bot.PlayerIndex,
                .Team = isInEngine ? Read<std::int8_t>(slot->Datum + k_PlayerTeam) : std::int8_t{ -1 } });
        }

        // The local player 0 gives the team of the auto bots. Every other player in the table takes a slot.
        const auto localMachine{ Read<std::int32_t>(globals + k_LocalMachineIndex) };
        std::int8_t localTeam{ -1 };
        std::size_t occupied{ 0 };
        for (std::size_t index{ 0 }; index < k_MaxPlayers; ++index)
        {
            const auto slot{ FindPlayerSlot(threadContext, index) };
            if (!slot) continue;

            ++occupied;
            if (Read<std::int16_t>(slot->Datum + k_PlayerMachineIndex) == localMachine && Read<std::int32_t>(slot->Datum + k_PlayerUserIndex) == 0)
            {
                localTeam = Read<std::int8_t>(slot->Datum + k_PlayerTeam);
            }
        }

        const auto plan{ PlanRoster(current, roster, localTeam, occupied - botsInEngine) };
        if (!plan) return reject(plan.error());

        const std::size_t total{ plan->Moves.size() + plan->Removes.size() + plan->Adds.size() };
        std::size_t done{ 0 };
        std::size_t failures{ 0 };

        for (const auto& move : plan->Moves)
        {
            const auto found{ std::ranges::find(bots, move.Number, &Bot::Number) };

            if (this->ApplyTeam(threadContext, TeamRequest{ .PlayerIndex = found->PlayerIndex, .Team = move.Team })) ++done;
            else                                                                                                    ++failures;
        }

        for (const std::size_t number : plan->Removes)
        {
            if (this->RemoveBot(threadContext, number)) ++done;
            else                                        ++failures;
        }

        for (const std::int8_t team : plan->Adds)
        {
            if (!this->AddBot(threadContext, team))
            {
                ++failures;
                break;
            }

            ++done;
        }

        m_Log.Message("[BotService] INFO: Roster {}: {} moves, {} removes, {} adds, {} of {} operations done, {} failed.", ToText(roster),
            plan->Moves.size(), plan->Removes.size(), plan->Adds.size(), done, total, failures);

        // note: Operations left after a failed add are skipped, so they count as not done. Nothing is undone or retried.
        if (done < total) return reject(std::format("{} of {} operations done, see the log", done, total));

        return RosterResult{ .Outcome = outcome, .Round = round };
    }

    auto BotService::EnsureFakeMachine(std::uintptr_t playersGlobals, std::size_t machineIndex) -> bool
    {
        std::vector<FakeMachine> fakeMachines{};
        m_BotStore.CopyFakeMachines(fakeMachines);

        const bool isKnown{ std::ranges::any_of(fakeMachines, [machineIndex](const FakeMachine& machine) {
            return machine.Index == machineIndex;
        }) };

        const std::uint32_t bit{ 1u << machineIndex };
        auto mask{ Read<std::uint32_t>(playersGlobals + k_MachineMask) };

        if (isKnown && (mask & bit) != 0) return true;

        if (!isKnown)
        {
            if ((mask & bit) != 0)
            {
                m_Log.Message("[BotService] ERROR: Machine slot {} belongs to a real machine.", machineIndex);
                return false;
            }

            FakeMachine machine{ .Index = machineIndex };
            std::copy(k_FakeMachinePrefix.begin(), k_FakeMachinePrefix.end(), machine.Identifier.Bytes.begin());
            machine.Identifier.Bytes.back() = static_cast<std::uint8_t>(machineIndex);
            m_BotStore.AddFakeMachine(machine);
        }

        // note: The current machines go in unchanged. The SetMachines hook adds the fake ones before the engine applies them.
        std::array<MachineIdentifier, k_MaxMachines> identifiers{};
        std::memcpy(identifiers.data(), reinterpret_cast<const void*>(playersGlobals + k_MachineIdentifiers), sizeof(identifiers));
        m_SetMachines(mask, identifiers.data());

        mask = Read<std::uint32_t>(playersGlobals + k_MachineMask);
        if ((mask & bit) == 0)
        {
            m_Log.Message("[BotService] ERROR: The engine did not take fake machine {} (is the SetMachines hook installed?).",
                machineIndex);
            return false;
        }

        m_Log.Message("[BotService] INFO: Fake machine {} added. Machine mask {:05X}.", machineIndex, mask);
        return true;
    }

    auto BotService::ApplyTeam(std::uintptr_t threadContext, const TeamRequest& request) -> bool
    {
        if (!m_IsResolved || request.Team < 0 || static_cast<std::size_t>(request.Team) >= k_MaxTeams) return false;

        const auto players{ Read<std::uintptr_t>(threadContext + k_PlayersSlot) };
        if (players == 0) return false;

        const std::uintptr_t datum{ GetPlayerDatum(players, request.PlayerIndex) };
        const auto salt{ datum != 0 ? Read<std::uint16_t>(datum + k_PlayerSalt) : std::uint16_t{ 0 } };
        if (salt == 0)
        {
            m_Log.Message("[BotService] WARNING: Player {} is gone, its team was not changed.", request.PlayerIndex);
            return false;
        }

        std::array<std::uint8_t, k_PlayerConfigurationSize> configuration{};
        std::memcpy(configuration.data(), reinterpret_cast<const void*>(datum + k_PlayerConfiguration), configuration.size());
        configuration[k_ConfigurationTeam] = static_cast<std::uint8_t>(request.Team);

        const std::uint32_t handle{ MakePlayerHandle(salt, request.PlayerIndex) };
        m_ApplyPlayerConfiguration(handle, configuration.data());

        const auto team{ Read<std::int8_t>(datum + k_PlayerTeam) };
        if (team != request.Team)
        {
            m_Log.Message("[BotService] ERROR: Player {} did not move to team {} (still on {}).", request.PlayerIndex, request.Team, team);
            return false;
        }

        m_Log.Message("[BotService] INFO: Player {} moved to team {} (now {}).", request.PlayerIndex, request.Team, team);
        return true;
    }

    auto BotService::GetUnitActionSlot(std::uintptr_t threadContext, std::size_t playerIndex) const -> std::optional<std::uint8_t>
    {
        const auto slot{ FindPlayerSlot(threadContext, playerIndex) };
        if (!slot) return std::nullopt;

        const std::uintptr_t unit{ GetObjectData(threadContext, Read<std::uint32_t>(slot->Datum + k_PlayerUnit)) };
        if (unit == 0) return std::nullopt;

        return Read<std::uint8_t>(unit + k_UnitActionSlot);
    }

    auto BotService::WriteInteraction(std::uintptr_t threadContext, std::size_t playerIndex, std::span<std::uint8_t> interaction) const
        -> bool
    {
        if (!m_IsInteractionResolved || threadContext == 0) return false;
        if (interaction.size() != k_InteractionRecordCount * k_InteractionRecordSize) return false;

        const auto slot{ FindPlayerSlot(threadContext, playerIndex) };
        if (!slot) return false;

        // note: Same steps as 0x4f280: clear the records, then fill them only if the player has a unit (the steps read it unchecked).
        std::ranges::fill(interaction, std::uint8_t{ 0 });
        for (std::size_t record{ 0 }; record < k_InteractionRecordCount; ++record)
        {
            std::memset(interaction.data() + (record * k_InteractionRecordSize) + k_InteractionRecordObject, 0xFF,
                sizeof(std::uint32_t));
        }

        if (Read<std::uint32_t>(slot->Datum + k_PlayerUnit) == 0xFFFFFFFF) return true;

        const std::uint32_t handle{ MakePlayerHandle(slot->Salt, playerIndex) };
        m_GatherNearbyInteractions(handle, interaction.data());
        m_GatherAimedInteractions(handle, interaction.data());
        return true;
    }

    auto BotService::GetUnitClientState(std::uintptr_t threadContext, std::size_t playerIndex) const -> std::optional<UnitClientState>
    {
        const auto slot{ FindPlayerSlot(threadContext, playerIndex) };
        if (!slot) return std::nullopt;

        const auto unitHandle{ Read<std::uint32_t>(slot->Datum + k_PlayerUnit) };
        const std::uintptr_t unit{ GetObjectData(threadContext, unitHandle) };
        if (unit == 0) return std::nullopt;

        const float aimX{ Read<float>(unit + k_UnitDesiredAim) };
        const float aimY{ Read<float>(unit + k_UnitDesiredAim + sizeof(float)) };
        const float aimZ{ Read<float>(unit + k_UnitDesiredAim + (2 * sizeof(float))) };
        float yaw{ std::atan2(aimY, aimX) };
        if (yaw < 0.0f) yaw += k_TwoPi;

        return UnitClientState{
            .Unit = unitHandle,
            .WeaponSlot = Read<std::int8_t>(unit + k_UnitWeaponSlot),
            .InventoryVersion = Read<std::uint16_t>(unit + k_UnitInventoryVersion),
            .GrenadeType = Read<std::int8_t>(unit + k_UnitGrenadeType),
            .ZoomLevel = Read<std::int8_t>(unit + k_UnitZoomLevel),
            .IsSeated = Read<std::uint32_t>(unit + k_ObjectParent) != 0xFFFFFFFF && Read<std::int16_t>(unit + k_UnitSeat) != -1,
            .Facing = { yaw, std::atan2(aimZ, std::hypot(aimX, aimY)) } };
    }

    auto BotService::GetNextWeaponSlot(const UnitClientState& state) const -> std::optional<std::int8_t>
    {
        if (!m_IsClientResolved || state.IsSeated) return std::nullopt;

        const std::int16_t slot{ m_NextWeaponSlot(state.Unit, state.WeaponSlot, k_SelectorStep) };
        if (slot < 0 || slot == state.WeaponSlot) return std::nullopt;

        return static_cast<std::int8_t>(slot);
    }

    auto BotService::GetNextGrenadeType(const UnitClientState& state) const -> std::optional<std::int8_t>
    {
        if (!m_IsClientResolved) return std::nullopt;

        const std::int16_t type{ m_NextGrenadeType(state.Unit, state.GrenadeType, k_SelectorStep) };
        if (type < 0 || type == state.GrenadeType) return std::nullopt;

        return static_cast<std::int8_t>(type);
    }

    auto BotService::GetNextZoomLevel(const UnitClientState& state) const -> std::optional<std::int8_t>
    {
        if (!m_IsClientResolved || !m_CanUnitZoom(state.Unit)) return std::nullopt;

        return static_cast<std::int8_t>(m_NextZoomLevel(state.Unit, state.ZoomLevel));
    }

    auto BotService::ResolveFunctions() -> bool
    {
        if (!m_IsResolved)
        {
            m_SetMachines = reinterpret_cast<SetMachinesFunction>(m_AOBService.FindPattern(Signature::k_SetMachines));
            m_ApplyPlayerAdded = reinterpret_cast<ApplyPlayerAddedFunction>(m_AOBService.FindPattern(Signature::k_ApplyPlayerAdded));
            m_ApplyPlayerConfiguration = reinterpret_cast<ApplyPlayerConfigurationFunction>(
                m_AOBService.FindPattern(Signature::k_ApplyPlayerConfiguration));

            m_ApplyPlayerUpdate = reinterpret_cast<ApplyPlayerUpdateFunction>(m_AOBService.FindPattern(Signature::k_ApplyPlayerUpdate));

            m_IsResolved = m_SetMachines && m_ApplyPlayerAdded && m_ApplyPlayerConfiguration;
            if (!m_IsResolved)
            {
                m_Log.Message("[BotService] ERROR: Failed to find {}. No bot can be added.", !m_SetMachines ? "SetMachines"
                    : (!m_ApplyPlayerAdded ? "ApplyPlayerAdded" : "ApplyPlayerConfiguration"));
            }

            if (!m_ApplyPlayerUpdate) m_Log.Message("[BotService] ERROR: Failed to find ApplyPlayerUpdate. No bot can be removed.");
        }

        if (!m_IsInteractionResolved)
        {
            m_GatherNearbyInteractions = reinterpret_cast<GatherInteractionsFunction>(
                m_AOBService.FindPattern(Signature::k_GatherNearbyInteractions));
            m_GatherAimedInteractions = reinterpret_cast<GatherInteractionsFunction>(
                m_AOBService.FindPattern(Signature::k_GatherAimedInteractions));

            m_IsInteractionResolved = m_GatherNearbyInteractions && m_GatherAimedInteractions;
            if (!m_IsInteractionResolved)
            {
                m_Log.Message("[BotService] ERROR: Failed to find {}. Bots will have no interaction.",
                    !m_GatherNearbyInteractions ? "GatherNearbyInteractions" : "GatherAimedInteractions");
            }
        }

        if (!m_IsClientResolved)
        {
            m_NextWeaponSlot = reinterpret_cast<UnitSelectorFunction>(m_AOBService.FindPattern(Signature::k_NextWeaponSlot));
            m_NextGrenadeType = reinterpret_cast<UnitSelectorFunction>(m_AOBService.FindPattern(Signature::k_NextGrenadeType));
            m_NextZoomLevel = reinterpret_cast<NextZoomLevelFunction>(m_AOBService.FindPattern(Signature::k_NextZoomLevel));
            m_CanUnitZoom = reinterpret_cast<CanUnitZoomFunction>(m_AOBService.FindPattern(Signature::k_CanUnitZoom));

            m_IsClientResolved = m_NextWeaponSlot && m_NextGrenadeType && m_NextZoomLevel && m_CanUnitZoom;
            if (!m_IsClientResolved)
            {
                m_Log.Message("[BotService] ERROR: Failed to find {}. Agents cannot switch weapon, grenade or zoom.",
                    !m_NextWeaponSlot ? "NextWeaponSlot" : (!m_NextGrenadeType ? "NextGrenadeType"
                    : (!m_NextZoomLevel ? "NextZoomLevel" : "CanUnitZoom")));
            }
        }

        return m_IsResolved && m_ApplyPlayerUpdate && m_IsInteractionResolved && m_IsClientResolved;
    }
}