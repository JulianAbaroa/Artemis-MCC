module Platform.Simulation.System;

import Platform.Memory.Type;
import std;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    using Platform::Simulation::Type::SessionSnapshot;
    using Platform::Simulation::Type::SessionPlayer;
    using Platform::Simulation::Type::MachineIdentifier;
    using Platform::Simulation::Type::Bot;
    using Platform::Simulation::Type::FakeMachine;
    using Platform::Simulation::Type::SetMachinesFunction;
    using Platform::Simulation::Type::ApplyPlayerAddedFunction;
    using Platform::Simulation::Type::ApplyPlayerConfigurationFunction;
    using Platform::Simulation::Type::TeamRequest;
    using Platform::Simulation::Type::GatherInteractionsFunction;
    using Platform::Simulation::Type::UnitSelectorFunction;
    using Platform::Simulation::Type::NextZoomLevelFunction;
    using Platform::Simulation::Type::CanUnitZoomFunction;
    using Platform::Simulation::Type::UnitClientState;

    using Platform::Simulation::Type::k_MaxMachines;
    using Platform::Simulation::Type::k_MaxPlayers;
    using Platform::Simulation::Type::k_MaxPlayersPerMachine;
    using Platform::Simulation::Type::k_ObjectsSlot;
    using Platform::Simulation::Type::k_PlayersSlot;
    using Platform::Simulation::Type::k_PlayersGlobalsSlot;
    using Platform::Simulation::Type::k_GameOptionsSlot;
    using Platform::Simulation::Type::k_DataArrayCapacity;
    using Platform::Simulation::Type::k_DataArrayElementSize;
    using Platform::Simulation::Type::k_DataArrayData;
    using Platform::Simulation::Type::k_PlayerSalt;
    using Platform::Simulation::Type::k_PlayerFlags;
    using Platform::Simulation::Type::k_PlayerIdentifier;
    using Platform::Simulation::Type::k_PlayerMachineIdentifier;
    using Platform::Simulation::Type::k_PlayerMachineIndex;
    using Platform::Simulation::Type::k_PlayerUserIndex;
    using Platform::Simulation::Type::k_PlayerController;
    using Platform::Simulation::Type::k_PlayerUnit;
    using Platform::Simulation::Type::k_ObjectHeaderSize;
    using Platform::Simulation::Type::k_ObjectHeaderData;
    using Platform::Simulation::Type::k_UnitFlags;
    using Platform::Simulation::Type::k_UnitActionSlot;
    using Platform::Simulation::Type::k_PlayerGamerTag;
    using Platform::Simulation::Type::k_PlayerTeam;
    using Platform::Simulation::Type::k_PlayerConfiguration;
    using Platform::Simulation::Type::k_ConfigurationTeam;
    using Platform::Simulation::Type::k_MaxTeams;
    using Platform::Simulation::Type::k_GamerTagLength;
    using Platform::Simulation::Type::k_MachineMask;
    using Platform::Simulation::Type::k_MachineIdentifiers;
    using Platform::Simulation::Type::k_LocalMachineIndex;
    using Platform::Simulation::Type::k_OptionsPlayers;
    using Platform::Simulation::Type::k_OptionsPlayerSize;
    using Platform::Simulation::Type::k_OptionsPlayerConfiguration;
    using Platform::Simulation::Type::k_PlayerConfigurationSize;
    using Platform::Simulation::Type::k_PlayerUpdateSize;
    using Platform::Simulation::Type::k_UpdatePlayerIndex;
    using Platform::Simulation::Type::k_UpdateIdentifier;
    using Platform::Simulation::Type::k_UpdateType;
    using Platform::Simulation::Type::k_UpdateMachine;
    using Platform::Simulation::Type::k_UpdateUserIndex;
    using Platform::Simulation::Type::k_UpdateController;
    using Platform::Simulation::Type::k_UpdateJoinedInProgress;
    using Platform::Simulation::Type::k_UpdateConfiguration;
    using Platform::Simulation::Type::k_PlayerUpdateAdded;
    using Platform::Simulation::Type::k_InteractionRecordCount;
    using Platform::Simulation::Type::k_InteractionRecordSize;
    using Platform::Simulation::Type::k_InteractionRecordObject;
    using Platform::Simulation::Type::k_UnitWeaponSlot;
    using Platform::Simulation::Type::k_UnitInventoryVersion;
    using Platform::Simulation::Type::k_UnitGrenadeType;
    using Platform::Simulation::Type::k_UnitZoomLevel;
    using Platform::Simulation::Type::k_ObjectParent;
    using Platform::Simulation::Type::k_UnitSeat;

    // Step of the weapon and grenade selectors: the next one, as a local client asks for it (0x61a38).
    constexpr std::int16_t k_SelectorStep{ 1 };

    // Bots one instance can hold: 4 per fake machine, on every machine slot except the first one (the local machine).
    constexpr std::size_t k_MaxBots{ (k_MaxMachines - 1) * k_MaxPlayersPerMachine };

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

namespace Platform::Simulation::System
{
    auto BotService::Update(std::uintptr_t threadContext) -> void
    {
        if (threadContext == 0) return;

        this->PublishSnapshot(threadContext);

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
            m_LogsService.Message("[BotService] ERROR: Failed to add a bot: the engine functions were not found.");
            return false;
        }

        const std::size_t count{ m_BotStore.GetBotCount() };
        if (count >= k_MaxBots)
        {
            m_LogsService.Message("[BotService] WARNING: No machine slot left for another bot.");
            return false;
        }

        const auto players{ Read<std::uintptr_t>(threadContext + k_PlayersSlot) };
        const auto globals{ Read<std::uintptr_t>(threadContext + k_PlayersGlobalsSlot) };
        const auto options{ Read<std::uintptr_t>(threadContext + k_GameOptionsSlot) };
        if (players == 0 || globals == 0 || options == 0)
        {
            m_LogsService.Message("[BotService] ERROR: Failed to read the players, the machines or the game options of the engine.");
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
            m_LogsService.Message("[BotService] ERROR: Failed to find {}.", !localPlayer ? "the local player" : "a free player slot");
            return false;
        }

        const std::size_t number{ count + 1 };
        const std::size_t machineIndex{ (k_MaxMachines - 1) - (count / k_MaxPlayersPerMachine) };
        const std::size_t slot{ count % k_MaxPlayersPerMachine };

        if (!this->EnsureFakeMachine(globals, machineIndex)) return false;

        std::array<std::uint8_t, k_PlayerUpdateSize> update{};
        const auto playerIndex{ static_cast<std::int32_t>(*freeIndex) };
        const std::uint64_t identifier{ k_BotIdentifierBase | number };
        const auto userIndex{ static_cast<std::int32_t>(slot) };

        MachineIdentifier machine{};
        std::memcpy(machine.Bytes.data(), reinterpret_cast<const void*>(globals + k_MachineIdentifiers
            + (machineIndex * sizeof(MachineIdentifier))), sizeof(machine));

        std::memcpy(update.data() + k_UpdatePlayerIndex, &playerIndex, sizeof(playerIndex));
        std::memcpy(update.data() + k_UpdateIdentifier, &identifier, sizeof(identifier));
        std::memcpy(update.data() + k_UpdateType, &k_PlayerUpdateAdded, sizeof(k_PlayerUpdateAdded));
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

        const std::wstring name{ std::format(L"Bot {}", number) };
        const std::size_t names{ RenameConfiguration(configuration, gamerTag, name) };
        const std::size_t identifiers{ ReplaceIdentifier(configuration, Read<std::uint64_t>(localDatum + k_PlayerIdentifier),
            identifier) };

        if (team) configuration[k_ConfigurationTeam] = static_cast<std::uint8_t>(*team);

        if (!m_ApplyPlayerAdded(update.data()))
        {
            m_LogsService.Message("[BotService] ERROR: The engine refused bot {} (player {}, machine {}, user {}).", number,
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

        m_LogsService.Message("[BotService] INFO: Bot {} added as player {} on machine {} (user {}). Configuration: {} names and {}"
            " identifiers replaced.", number, *freeIndex, machineIndex, slot, names, identifiers);
        return true;
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
                m_LogsService.Message("[BotService] ERROR: Machine slot {} belongs to a real machine.", machineIndex);
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
            m_LogsService.Message("[BotService] ERROR: The engine did not take fake machine {} (is the SetMachines hook installed?).",
                machineIndex);
            return false;
        }

        m_LogsService.Message("[BotService] INFO: Fake machine {} added. Machine mask {:05X}.", machineIndex, mask);
        return true;
    }

    auto BotService::ApplyTeam(std::uintptr_t threadContext, const TeamRequest& request) -> void
    {
        if (!m_IsResolved || request.Team < 0 || static_cast<std::size_t>(request.Team) >= k_MaxTeams) return;

        const auto players{ Read<std::uintptr_t>(threadContext + k_PlayersSlot) };
        if (players == 0) return;

        const std::uintptr_t datum{ GetPlayerDatum(players, request.PlayerIndex) };
        const auto salt{ datum != 0 ? Read<std::uint16_t>(datum + k_PlayerSalt) : std::uint16_t{ 0 } };
        if (salt == 0)
        {
            m_LogsService.Message("[BotService] WARNING: Player {} is gone, its team was not changed.", request.PlayerIndex);
            return;
        }

        std::array<std::uint8_t, k_PlayerConfigurationSize> configuration{};
        std::memcpy(configuration.data(), reinterpret_cast<const void*>(datum + k_PlayerConfiguration), configuration.size());
        configuration[k_ConfigurationTeam] = static_cast<std::uint8_t>(request.Team);

        const std::uint32_t handle{ MakePlayerHandle(salt, request.PlayerIndex) };
        m_ApplyPlayerConfiguration(handle, configuration.data());

        m_LogsService.Message("[BotService] INFO: Player {} moved to team {} (now {}).", request.PlayerIndex, request.Team,
            Read<std::int8_t>(datum + k_PlayerTeam));
    }

    auto BotService::GetUnitActionSlot(std::uintptr_t threadContext, std::size_t playerIndex) const -> std::optional<std::uint8_t>
    {
        if (threadContext == 0) return std::nullopt;

        const auto players{ Read<std::uintptr_t>(threadContext + k_PlayersSlot) };
        if (players == 0) return std::nullopt;

        const std::uintptr_t datum{ GetPlayerDatum(players, playerIndex) };
        if (datum == 0 || Read<std::uint16_t>(datum + k_PlayerSalt) == 0) return std::nullopt;

        const std::uintptr_t unit{ GetObjectData(threadContext, Read<std::uint32_t>(datum + k_PlayerUnit)) };
        if (unit == 0) return std::nullopt;

        return Read<std::uint8_t>(unit + k_UnitActionSlot);
    }

    auto BotService::WriteInteraction(std::uintptr_t threadContext, std::size_t playerIndex, std::span<std::uint8_t> interaction) const
        -> bool
    {
        if (!m_IsInteractionResolved || threadContext == 0) return false;
        if (interaction.size() != k_InteractionRecordCount * k_InteractionRecordSize) return false;

        const auto players{ Read<std::uintptr_t>(threadContext + k_PlayersSlot) };
        if (players == 0) return false;

        const std::uintptr_t datum{ GetPlayerDatum(players, playerIndex) };
        const auto salt{ datum != 0 ? Read<std::uint16_t>(datum + k_PlayerSalt) : std::uint16_t{ 0 } };
        if (salt == 0) return false;

        // note: Same steps as 0x4f280: clear the records, then fill them only if the player has a unit (the steps read it unchecked).
        std::ranges::fill(interaction, std::uint8_t{ 0 });
        for (std::size_t record{ 0 }; record < k_InteractionRecordCount; ++record)
        {
            std::memset(interaction.data() + (record * k_InteractionRecordSize) + k_InteractionRecordObject, 0xFF,
                sizeof(std::uint32_t));
        }

        if (Read<std::uint32_t>(datum + k_PlayerUnit) == 0xFFFFFFFF) return true;

        const std::uint32_t handle{ MakePlayerHandle(salt, playerIndex) };
        m_GatherNearbyInteractions(handle, interaction.data());
        m_GatherAimedInteractions(handle, interaction.data());
        return true;
    }

    auto BotService::GetUnitClientState(std::uintptr_t threadContext, std::size_t playerIndex) const -> std::optional<UnitClientState>
    {
        if (threadContext == 0) return std::nullopt;

        const auto players{ Read<std::uintptr_t>(threadContext + k_PlayersSlot) };
        if (players == 0) return std::nullopt;

        const std::uintptr_t datum{ GetPlayerDatum(players, playerIndex) };
        if (datum == 0 || Read<std::uint16_t>(datum + k_PlayerSalt) == 0) return std::nullopt;

        const auto unitHandle{ Read<std::uint32_t>(datum + k_PlayerUnit) };
        const std::uintptr_t unit{ GetObjectData(threadContext, unitHandle) };
        if (unit == 0) return std::nullopt;

        return UnitClientState{
            .Unit = unitHandle,
            .WeaponSlot = Read<std::int8_t>(unit + k_UnitWeaponSlot),
            .InventoryVersion = Read<std::uint16_t>(unit + k_UnitInventoryVersion),
            .GrenadeType = Read<std::int8_t>(unit + k_UnitGrenadeType),
            .ZoomLevel = Read<std::int8_t>(unit + k_UnitZoomLevel),
            .IsSeated = Read<std::uint32_t>(unit + k_ObjectParent) != 0xFFFFFFFF && Read<std::int16_t>(unit + k_UnitSeat) != -1 };
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
            m_SetMachines = reinterpret_cast<SetMachinesFunction>(m_AOBService.FindPattern(Signature::SetMachines));
            m_ApplyPlayerAdded = reinterpret_cast<ApplyPlayerAddedFunction>(m_AOBService.FindPattern(Signature::ApplyPlayerAdded));
            m_ApplyPlayerConfiguration = reinterpret_cast<ApplyPlayerConfigurationFunction>(
                m_AOBService.FindPattern(Signature::ApplyPlayerConfiguration));

            m_IsResolved = m_SetMachines && m_ApplyPlayerAdded && m_ApplyPlayerConfiguration;
            if (!m_IsResolved)
            {
                m_LogsService.Message("[BotService] ERROR: Failed to find {}. No bot can be added.", !m_SetMachines ? "SetMachines"
                    : (!m_ApplyPlayerAdded ? "ApplyPlayerAdded" : "ApplyPlayerConfiguration"));
            }
        }

        if (!m_IsInteractionResolved)
        {
            m_GatherNearbyInteractions = reinterpret_cast<GatherInteractionsFunction>(
                m_AOBService.FindPattern(Signature::GatherNearbyInteractions));
            m_GatherAimedInteractions = reinterpret_cast<GatherInteractionsFunction>(
                m_AOBService.FindPattern(Signature::GatherAimedInteractions));

            m_IsInteractionResolved = m_GatherNearbyInteractions && m_GatherAimedInteractions;
            if (!m_IsInteractionResolved)
            {
                m_LogsService.Message("[BotService] ERROR: Failed to find {}. Bots will have no interaction.",
                    !m_GatherNearbyInteractions ? "GatherNearbyInteractions" : "GatherAimedInteractions");
            }
        }

        if (!m_IsClientResolved)
        {
            m_NextWeaponSlot = reinterpret_cast<UnitSelectorFunction>(m_AOBService.FindPattern(Signature::NextWeaponSlot));
            m_NextGrenadeType = reinterpret_cast<UnitSelectorFunction>(m_AOBService.FindPattern(Signature::NextGrenadeType));
            m_NextZoomLevel = reinterpret_cast<NextZoomLevelFunction>(m_AOBService.FindPattern(Signature::NextZoomLevel));
            m_CanUnitZoom = reinterpret_cast<CanUnitZoomFunction>(m_AOBService.FindPattern(Signature::CanUnitZoom));

            m_IsClientResolved = m_NextWeaponSlot && m_NextGrenadeType && m_NextZoomLevel && m_CanUnitZoom;
            if (!m_IsClientResolved)
            {
                m_LogsService.Message("[BotService] ERROR: Failed to find {}. Agents cannot switch weapon, grenade or zoom.",
                    !m_NextWeaponSlot ? "NextWeaponSlot" : (!m_NextGrenadeType ? "NextGrenadeType"
                    : (!m_NextZoomLevel ? "NextZoomLevel" : "CanUnitZoom")));
            }
        }

        return m_IsResolved && m_IsInteractionResolved && m_IsClientResolved;
    }
}