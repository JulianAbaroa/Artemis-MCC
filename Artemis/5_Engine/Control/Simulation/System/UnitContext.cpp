module Engine.Control.Simulation.System;

import :UnitContext;

import Common.Engine.Type;
import Control.Action.Type;
import Control.Client.Type;
import Engine.Control.Simulation.Type;
import std;

namespace
{
    using ::Control::Action::Type::UnitClientState;
    using ::Control::Client::Type::ClientContextSource;
    using ::Control::Client::Type::ClientSettings;
    using ::Control::Client::Type::UnitContext;
    using ::Control::Client::Type::UnitFact;
    using Engine::Control::Simulation::System::UnitFactPort;

    using ::Control::Client::Type::k_NoZoom;

    using Common::Engine::Type::k_MaxLocalUsers;
    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_NoInputUser;
    using Engine::Control::Simulation::Type::k_AltPitchKinds;
    using Engine::Control::Simulation::Type::k_AltPitchMask;
    using Engine::Control::Simulation::Type::k_FlagAbilityEngine;
    using Engine::Control::Simulation::Type::k_FlagCarriedDrop;
    using Engine::Control::Simulation::Type::k_FlagCarriedZoomReset;
    using Engine::Control::Simulation::Type::k_JumpAssistFeature;
    using Engine::Control::Simulation::Type::k_JumpAssistTicksDivisor;
    using Engine::Control::Simulation::Type::k_KindBlockFlags;
    using Engine::Control::Simulation::Type::k_KindBlockKind;
    using Engine::Control::Simulation::Type::k_KindBlockValidBit;
    using Engine::Control::Simulation::Type::k_KindNoCrouchToggle;
    using Engine::Control::Simulation::Type::k_KindNone;
    using Engine::Control::Simulation::Type::k_KindSeatAlternate;
    using Engine::Control::Simulation::Type::k_MaxBlockOffset;
    using Engine::Control::Simulation::Type::k_MaxParentChain;
    using Engine::Control::Simulation::Type::k_ObjectHeldState;
    using Engine::Control::Simulation::Type::k_ObjectParent;
    using Engine::Control::Simulation::Type::k_ObjectTag;
    using Engine::Control::Simulation::Type::k_ObjectTypeBiped;
    using Engine::Control::Simulation::Type::k_ObjectTypeVehicle;
    using Engine::Control::Simulation::Type::k_ParentAltPitchByte;
    using Engine::Control::Simulation::Type::k_PlayerLookRated;
    using Engine::Control::Simulation::Type::k_SeatFlagAlternate;
    using Engine::Control::Simulation::Type::k_SeatFlagParentWeapon;
    using Engine::Control::Simulation::Type::k_SeatSize;
    using Engine::Control::Simulation::Type::k_TagAbilityKind;
    using Engine::Control::Simulation::Type::k_TagAimDamp;
    using Engine::Control::Simulation::Type::k_TagFlagStride;
    using Engine::Control::Simulation::Type::k_TagFlagTable;
    using Engine::Control::Simulation::Type::k_TagKindCount;
    using Engine::Control::Simulation::Type::k_TagKindTable;
    using Engine::Control::Simulation::Type::k_TagSeatBlock;
    using Engine::Control::Simulation::Type::k_TagSeatCount;
    using Engine::Control::Simulation::Type::k_TagSelectionBiped;
    using Engine::Control::Simulation::Type::k_TagSelectionVehicle;
    using Engine::Control::Simulation::Type::k_TagTriggerCurveBit;
    using Engine::Control::Simulation::Type::k_TagTriggerCurveFlags;
    using Engine::Control::Simulation::Type::k_TriggerCurveForwardBit;
    using Engine::Control::Simulation::Type::k_TagUnitFlags;
    using Engine::Control::Simulation::Type::k_TagWeaponCrouchBit;
    using Engine::Control::Simulation::Type::k_TagWeaponFlags;
    using Engine::Control::Simulation::Type::k_TagWeaponTriggerBit;
    using Engine::Control::Simulation::Type::k_UnitAimDamp;
    using Engine::Control::Simulation::Type::k_UnitCarried;
    using Engine::Control::Simulation::Type::k_UnitInventory;
    using Engine::Control::Simulation::Type::k_UnitJumpCounter;
    using Engine::Control::Simulation::Type::k_UnitJumpFlagA;
    using Engine::Control::Simulation::Type::k_UnitJumpFlagB;
    using Engine::Control::Simulation::Type::k_UnitJumpFlags;
    using Engine::Control::Simulation::Type::k_UnitKindBlockOffset;
    using Engine::Control::Simulation::Type::k_UnitSeat;
    using Engine::Control::Simulation::Type::k_UnitStateOffset;
    using Engine::Control::Simulation::Type::k_UnitWeaponSlot;
    using Engine::Control::Simulation::Type::k_UnitWeaponSlotSecond;

    template<typename T>
    auto ReadObjectValue(const UnitFactPort& port, std::uint32_t handle, std::size_t offset) -> std::optional<T>
    {
        T value{};
        if (!port.ReadObject(handle, offset, std::span<std::uint8_t>{ reinterpret_cast<std::uint8_t*>(&value), sizeof(T) })) return std::nullopt;
        return value;
    }

    template<typename T>
    auto ReadTagValue(const UnitFactPort& port, std::uint32_t tag, std::size_t offset) -> std::optional<T>
    {
        T value{};
        if (!port.ReadTag(tag, offset, std::span<std::uint8_t>{ reinterpret_cast<std::uint8_t*>(&value), sizeof(T) })) return std::nullopt;
        return value;
    }

    template<typename T>
    auto ReadBlockValue(const UnitFactPort& port, std::uint32_t index, std::size_t offset) -> std::optional<T>
    {
        T value{};
        if (!port.ReadBlock(index, offset, std::span<std::uint8_t>{ reinterpret_cast<std::uint8_t*>(&value), sizeof(T) })) return std::nullopt;
        return value;
    }

    auto IsBitSet(std::uint32_t value, std::uint32_t bit) -> bool
    {
        return ((value >> bit) & 1u) != 0;
    }

    // return: The tag handle of an object (the first uint16 of its data).
    auto GetTag(const UnitFactPort& port, std::uint32_t object) -> std::optional<std::uint32_t>
    {
        const std::optional<std::uint16_t> tag{ ReadObjectValue<std::uint16_t>(port, object, k_ObjectTag) };
        if (!tag) return std::nullopt;
        return static_cast<std::uint32_t>(*tag);
    }

    // return: A flag of the flag table of the tag.
    auto IsTagFlag(const UnitFactPort& port, std::uint32_t tag, std::int32_t flag) -> std::optional<bool>
    {
        const std::optional<std::uint32_t> value{ ReadTagValue<std::uint32_t>(port, tag, k_TagFlagTable + (static_cast<std::size_t>(flag) * k_TagFlagStride)) };
        if (!value) return std::nullopt;
        return *value != 0;
    }

    // return: The index of the first non zero entry of the kind table of the tag, k_KindNone if all are zero.
    auto GetKind(const UnitFactPort& port, std::uint32_t tag) -> std::optional<std::uint32_t>
    {
        for (std::size_t index{ 0 }; index < k_TagKindCount; ++index)
        {
            const std::optional<std::uint32_t> value{ ReadTagValue<std::uint32_t>(port, tag, k_TagKindTable + (index * k_TagFlagStride)) };
            if (!value) return std::nullopt;
            if (*value != 0) return static_cast<std::uint32_t>(index);
        }

        return k_KindNone;
    }

    // return: A flag bit of a seat of the parent, false when the seat is outside the table.
    auto IsSeatFlag(const UnitFactPort& port, std::uint32_t parent, std::int16_t seat, std::uint32_t bit) -> std::optional<bool>
    {
        const std::optional<std::uint32_t> tag{ GetTag(port, parent) };
        if (!tag) return std::nullopt;

        const std::optional<std::int32_t> count{ ReadTagValue<std::int32_t>(port, *tag, k_TagSeatCount) };
        if (!count) return std::nullopt;
        if (seat < 0 || seat >= *count) return false;

        const std::optional<std::uint32_t> block{ ReadTagValue<std::uint32_t>(port, *tag, k_TagSeatBlock) };
        if (!block) return std::nullopt;

        const std::optional<std::uint32_t> flags{ ReadBlockValue<std::uint32_t>(port, *block, static_cast<std::size_t>(seat) * k_SeatSize) };
        if (!flags) return std::nullopt;
        return IsBitSet(*flags, bit);
    }

    // return: The handle of the weapon in the hands (k_NoHandle if none).
    auto GetWeapon(const UnitFactPort& port, std::uint32_t unit) -> std::optional<std::uint32_t>
    {
        const std::optional<std::int8_t> slot{ ReadObjectValue<std::int8_t>(port, unit, k_UnitWeaponSlot) };
        if (!slot) return std::nullopt;
        if (*slot < 0) return k_NoHandle;

        return ReadObjectValue<std::uint32_t>(port, unit, k_UnitInventory + (static_cast<std::size_t>(*slot) * sizeof(std::uint32_t)));
    }

    // return: A flag bit of the tag of an object, false when the handle is none.
    auto IsWeaponFlag(const UnitFactPort& port, std::uint32_t weapon, std::uint32_t bit) -> std::optional<bool>
    {
        if (weapon == k_NoHandle) return false;

        const std::optional<std::uint32_t> tag{ GetTag(port, weapon) };
        if (!tag) return std::nullopt;

        const std::optional<std::uint32_t> flags{ ReadTagValue<std::uint32_t>(port, *tag, k_TagWeaponFlags) };
        if (!flags) return std::nullopt;
        return IsBitSet(*flags, bit);
    }

    // return: The block of the unit an int16 offset in the unit points to, as an offset from the unit, or nullopt if the offset is not sane.
    auto GetBlockOffset(const UnitFactPort& port, std::uint32_t unit, std::size_t pointerOffset) -> std::optional<std::size_t>
    {
        const std::optional<std::int16_t> offset{ ReadObjectValue<std::int16_t>(port, unit, pointerOffset) };
        if (!offset || *offset < 0 || *offset > k_MaxBlockOffset) return std::nullopt;
        return static_cast<std::size_t>(*offset);
    }

    // Collects the resolved facts.
    struct Resolver
    {
        const UnitFactPort& Port;
        const UnitClientState& State;
        const ClientContextSource& Source;
        const ClientSettings& Settings;
        UnitContext& Context;
        std::uint32_t Known{ 0 };

        auto Mark(UnitFact fact) -> void
        {
            Known |= static_cast<std::uint32_t>(fact);
        }

        // Takes a value for a fact and marks it, or leaves the fact unknown.
        template<typename T>
        auto Take(UnitFact fact, const std::optional<T>& value, T& target) -> bool
        {
            if (!value) return false;

            target = *value;
            this->Mark(fact);
            return true;
        }
    };

    auto ResolveUser(Resolver& resolver) -> void
    {
        const ClientContextSource& source{ resolver.Source };
        UnitContext& context{ resolver.Context };
        const UnitFactPort& port{ resolver.Port };

        if (source.PlayerHandle == k_NoHandle)
        {
            context.IsLookRated = false;
            resolver.Mark(UnitFact::LookRated);
            context.IsAltControl = false;
            resolver.Mark(UnitFact::AltControl);
        }
        else
        {
            std::uint8_t lookRated{ 0 };
            if (port.ReadPlayer(source.PlayerHandle, k_PlayerLookRated, std::span<std::uint8_t>{ &lookRated, 1 }))
            {
                context.IsLookRated = lookRated != 0;
                resolver.Mark(UnitFact::LookRated);
            }

            resolver.Take(UnitFact::AltControl, port.IsAltControl(source.PlayerHandle), context.IsAltControl);
        }

        // note: 0x61830 indexes a table by the input user: a value outside the local users stays unknown, it is never passed to the engine. A source with
        // no input user (an agent) has no lock to ask, so its controls are free.
        if (source.InputUser == k_NoInputUser)
        {
            context.IsControlLocked = false;
            resolver.Mark(UnitFact::ControlLocked);
        }
        else if (source.InputUser >= 0 && std::cmp_less(source.InputUser, k_MaxLocalUsers))
        {
            resolver.Take(UnitFact::ControlLocked, port.IsControlsLocked(source.InputUser, source.LocalIndex), context.IsControlLocked);
        }

        if (source.DualWieldFlags) context.DualWieldFlags = *source.DualWieldFlags;

        // note: Without a player the engine would index the profile table with 0xFFFF, so the boost uses the default event.
        if (source.PlayerHandle == k_NoHandle)
        {
            context.IsBoostAltEvent = false;
            resolver.Mark(UnitFact::BoostAltEvent);
        }
        else
        {
            resolver.Take(UnitFact::BoostAltEvent, port.IsBoostAltEvent(source.PlayerHandle), context.IsBoostAltEvent);
        }
    }

    // Seat facts: the parent of the unit, the alternate pitch and the alternate seat events.
    auto ResolveSeat(Resolver& resolver, std::uint32_t unit) -> void
    {
        const UnitFactPort& port{ resolver.Port };
        UnitContext& context{ resolver.Context };

        if (!resolver.State.IsSeated)
        {
            resolver.Mark(UnitFact::SeatFlags);
            resolver.Mark(UnitFact::AltPitch);
            return;
        }

        const std::optional<std::uint32_t> parent{ ReadObjectValue<std::uint32_t>(port, unit, k_ObjectParent) };
        const std::optional<std::int16_t> seat{ ReadObjectValue<std::int16_t>(port, unit, k_UnitSeat) };
        if (!parent || !seat) return;

        const std::optional<std::uint32_t> tag{ GetTag(port, *parent) };
        const std::optional<std::uint32_t> kind{ tag ? GetKind(port, *tag) : std::nullopt };
        const std::optional<bool> isFlagged{ IsSeatFlag(port, *parent, *seat, k_SeatFlagAlternate) };
        if (!kind || !isFlagged) return;

        context.IsSeatFlagged = *isFlagged;
        context.IsParentKind2 = *kind == k_KindSeatAlternate;
        resolver.Mark(UnitFact::SeatFlags);

        const std::optional<std::uint8_t> altByte{ ReadObjectValue<std::uint8_t>(port, *parent, k_ParentAltPitchByte) };
        if (!altByte) return;

        const bool isKindAllowed{ *kind < 32 && IsBitSet(k_AltPitchKinds, *kind) };
        context.IsAltPitch = (*altByte & k_AltPitchMask) == 1 && isKindAllowed && *isFlagged;
        resolver.Mark(UnitFact::AltPitch);
    }

    // The zoom item. A unit in a seat whose flag bit 3 is set asks the weapon of its parent (0005.c 12755), not its own.
    auto ResolveZoomItem(Resolver& resolver, std::uint32_t unit, std::uint32_t ownWeapon) -> void
    {
        const UnitFactPort& port{ resolver.Port };
        UnitContext& context{ resolver.Context };

        std::uint32_t weapon{ ownWeapon };
        if (resolver.State.IsSeated)
        {
            const std::optional<std::uint32_t> parent{ ReadObjectValue<std::uint32_t>(port, unit, k_ObjectParent) };
            const std::optional<std::int16_t> seat{ ReadObjectValue<std::int16_t>(port, unit, k_UnitSeat) };
            if (!parent || !seat) return;

            const std::optional<bool> isParentWeapon{ IsSeatFlag(port, *parent, *seat, k_SeatFlagParentWeapon) };
            if (!isParentWeapon) return;

            if (*isParentWeapon)
            {
                const std::optional<std::uint32_t> parentWeapon{ GetWeapon(port, *parent) };
                if (!parentWeapon) return;
                weapon = *parentWeapon;
            }
        }

        if (weapon == k_NoHandle)
        {
            context.HasZoomItem = false;
            context.IsZoomItemWithoutLevels = false;
            resolver.Mark(UnitFact::ZoomItem);
            return;
        }

        const std::optional<std::int16_t> next{ port.GetNextZoomLevel(weapon) };
        if (!next) return;

        context.HasZoomItem = true;
        context.IsZoomItemWithoutLevels = *next == -1;
        resolver.Mark(UnitFact::ZoomItem);
    }

    // Weapon facts: move penalty, crouch flag, zoom item, trigger flag and dual wield.
    auto ResolveWeapon(Resolver& resolver, std::uint32_t unit) -> void
    {
        const UnitFactPort& port{ resolver.Port };
        UnitContext& context{ resolver.Context };

        const std::optional<std::int8_t> second{ ReadObjectValue<std::int8_t>(port, unit, k_UnitWeaponSlotSecond) };
        const std::optional<std::int8_t> first{ ReadObjectValue<std::int8_t>(port, unit, k_UnitWeaponSlot) };
        if (first && second && resolver.Source.DualWieldFlags)
        {
            context.IsDualWield = *first != -1 && *second != -1;
            resolver.Mark(UnitFact::DualWield);
        }

        const std::optional<std::uint32_t> weapon{ GetWeapon(port, unit) };
        if (!weapon) return;

        const bool isZoomed{ resolver.Source.ClientZoomLevel != k_NoZoom };
        if (*weapon == k_NoHandle)
        {
            context.MovePenaltyForward = 0.0f;
            context.MovePenaltySide = 0.0f;
            resolver.Mark(UnitFact::MovePenalty);
        }
        else
        {
            const std::optional<float> forward{ port.GetMovePenalty(*weapon, true, isZoomed) };
            const std::optional<float> side{ port.GetMovePenalty(*weapon, false, isZoomed) };
            if (forward && side)
            {
                context.MovePenaltyForward = *forward;
                context.MovePenaltySide = *side;
                resolver.Mark(UnitFact::MovePenalty);
            }
        }

        ResolveZoomItem(resolver, unit, *weapon);
        resolver.Take(UnitFact::WeaponCrouchFlag, IsWeaponFlag(port, *weapon, k_TagWeaponCrouchBit), context.IsWeaponCrouchFlag);
        resolver.Take(UnitFact::WeaponTriggerFlag, IsWeaponFlag(port, *weapon, k_TagWeaponTriggerBit), context.IsWeaponTriggerFlag);
    }

    // Crouch facts of a biped.
    auto ResolveCrouch(Resolver& resolver, std::uint32_t unit, std::optional<std::uint8_t> type) -> void
    {
        const UnitFactPort& port{ resolver.Port };
        UnitContext& context{ resolver.Context };

        const bool isBiped{ type && *type == k_ObjectTypeBiped };
        if (type && !isBiped)
        {
            resolver.Mark(UnitFact::CrouchGate);
            resolver.Mark(UnitFact::ForcedCrouch);
        }
        else if (type)
        {
            resolver.Take(UnitFact::CrouchGate, port.IsCrouchGate(unit), context.IsCrouchGate);
            resolver.Take(UnitFact::ForcedCrouch, port.IsForcedCrouch(unit), context.IsForcedCrouch);
        }

        const std::optional<bool> isBlocked{ port.IsCrouchToggleBlocked(unit) };
        const std::optional<std::size_t> block{ GetBlockOffset(port, unit, k_UnitKindBlockOffset) };
        if (!isBlocked || !block) return;

        const std::optional<std::uint32_t> flags{ ReadObjectValue<std::uint32_t>(port, unit, *block + k_KindBlockFlags) };
        const std::optional<std::uint32_t> kind{ ReadObjectValue<std::uint32_t>(port, unit, *block + k_KindBlockKind) };
        if (!flags || !kind) return;

        const bool isKindBlocked{ IsBitSet(*flags, k_KindBlockValidBit) && *kind == k_KindNoCrouchToggle };
        context.IsCrouchToggleBlocked = *isBlocked || isKindBlocked;
        resolver.Mark(UnitFact::CrouchToggleBlocked);
    }

    // Facts of the object the unit holds: the carried object and the ability.
    auto ResolveHeld(Resolver& resolver, std::uint32_t unit) -> void
    {
        const UnitFactPort& port{ resolver.Port };
        UnitContext& context{ resolver.Context };

        const std::optional<std::uint32_t> held{ ReadObjectValue<std::uint32_t>(port, unit, k_UnitCarried) };
        if (!held) return;

        if (*held == k_NoHandle)
        {
            resolver.Mark(UnitFact::Carried);
            resolver.Mark(UnitFact::Ability);
            return;
        }

        const std::optional<std::uint32_t> tag{ GetTag(port, *held) };
        const std::optional<std::uint32_t> state{ ReadObjectValue<std::uint32_t>(port, *held, k_ObjectHeldState) };
        if (!tag || !state) return;

        const std::optional<bool> isDrop{ IsTagFlag(port, *tag, k_FlagCarriedDrop) };
        const std::optional<bool> isReset{ IsTagFlag(port, *tag, k_FlagCarriedZoomReset) };
        if (isDrop && isReset)
        {
            const bool isHeld{ *state != k_NoHandle };
            context.IsCarriedDrop = *isDrop && isHeld;
            context.IsCarriedZoomReset = *isReset && isHeld;
            resolver.Mark(UnitFact::Carried);
        }

        const std::optional<std::uint8_t> kind{ ReadTagValue<std::uint8_t>(port, *tag, k_TagAbilityKind) };
        const std::optional<bool> isEngine{ IsTagFlag(port, *tag, k_FlagAbilityEngine) };
        if (kind && isEngine)
        {
            context.Ability.IsPresent = true;
            context.Ability.Kind = *kind;
            context.Ability.NeedsEngine = *isEngine;
            resolver.Mark(UnitFact::Ability);
        }
    }

    // Boost facts of the tag that holds the trigger curve: the forward push flag and the value of this call through the tag function.
    // note: Without the curve bit they are the defaults, the client does not read them. The engine only runs the function for a raw value
    // that is not zero. The event the raw value comes from depends on the control profile, so the value stays unknown without it.
    auto ResolveBoost(Resolver& resolver, std::uint32_t curveTag) -> void
    {
        const UnitFactPort& port{ resolver.Port };
        UnitContext& context{ resolver.Context };

        if (!context.IsTriggerCurve)
        {
            resolver.Mark(UnitFact::TriggerCurveValue);
            resolver.Mark(UnitFact::TriggerCurveForward);
            return;
        }

        const std::optional<std::uint32_t> flags{ ReadTagValue<std::uint32_t>(port, curveTag, k_TagTriggerCurveFlags) };
        if (flags)
        {
            context.IsTriggerCurveForward = IsBitSet(*flags, k_TriggerCurveForwardBit);
            resolver.Mark(UnitFact::TriggerCurveForward);
        }

        if ((resolver.Known & static_cast<std::uint32_t>(UnitFact::BoostAltEvent)) == 0) return;

        const float raw{ context.IsBoostAltEvent ? resolver.Source.BoostAltValue : resolver.Source.BoostValue };
        if (raw == 0.0f)
        {
            context.TriggerCurveValue = 0.0f;
            resolver.Mark(UnitFact::TriggerCurveValue);
            return;
        }

        resolver.Take(UnitFact::TriggerCurveValue, port.EvaluateTriggerCurve(curveTag, raw), context.TriggerCurveValue);
    }

    // Facts of the tag of the unit: trigger curve, aim damping and the selection list of its root.
    auto ResolveUnitTag(Resolver& resolver, std::uint32_t unit) -> void
    {
        const UnitFactPort& port{ resolver.Port };
        UnitContext& context{ resolver.Context };

        const std::optional<std::uint32_t> tag{ GetTag(port, unit) };

        // note: A seat with the flag bit 2 swaps the tag for the one of the parent before the trigger curve bit is read (0005.c 11971).
        std::optional<std::uint32_t> curveTag{ tag };
        if (resolver.State.IsSeated)
        {
            const std::optional<std::uint32_t> parent{ ReadObjectValue<std::uint32_t>(port, unit, k_ObjectParent) };
            const std::optional<std::int16_t> seat{ ReadObjectValue<std::int16_t>(port, unit, k_UnitSeat) };
            const std::optional<bool> isParentTag{ parent && seat ? IsSeatFlag(port, *parent, *seat, k_SeatFlagAlternate) : std::nullopt };
            if (isParentTag && *isParentTag) curveTag = GetTag(port, *parent);
            else if (!isParentTag) curveTag = std::nullopt;
        }

        if (curveTag)
        {
            const std::optional<std::uint32_t> flags{ ReadTagValue<std::uint32_t>(port, *curveTag, k_TagUnitFlags) };
            if (flags)
            {
                context.IsTriggerCurve = IsBitSet(*flags, k_TagTriggerCurveBit);
                resolver.Mark(UnitFact::TriggerCurve);
                ResolveBoost(resolver, *curveTag);
            }
        }

        if (tag)
        {

            const std::optional<float> tagDamp{ ReadTagValue<float>(port, *tag, k_TagAimDamp) };
            const std::optional<float> unitDamp{ ReadObjectValue<float>(port, unit, k_UnitAimDamp) };
            if (tagDamp && unitDamp)
            {
                context.AimDampTag = *tagDamp;
                context.AimDampUnit = *unitDamp;
                resolver.Mark(UnitFact::AimDamp);
            }
        }

        // note: The selection list is the one of the root of the parent chain, a biped or a vehicle.
        std::uint32_t root{ unit };
        for (std::size_t hop{ 0 }; hop <= k_MaxParentChain; ++hop)
        {
            const std::optional<std::uint32_t> parent{ ReadObjectValue<std::uint32_t>(port, root, k_ObjectParent) };
            if (!parent) return;
            if (*parent == k_NoHandle) break;
            if (hop == k_MaxParentChain) return;
            root = *parent;
        }

        const std::optional<std::uint8_t> rootType{ port.GetObjectType(root) };
        const std::optional<std::uint32_t> rootTag{ GetTag(port, root) };
        if (!rootType || !rootTag) return;

        std::int32_t entries{ 0 };
        if (*rootType == k_ObjectTypeBiped || *rootType == k_ObjectTypeVehicle)
        {
            const std::size_t list{ *rootType == k_ObjectTypeBiped ? k_TagSelectionBiped : k_TagSelectionVehicle };
            const std::optional<std::int32_t> count{ ReadTagValue<std::int32_t>(port, *rootTag, list) };
            if (!count) return;
            entries = *count;
        }

        context.SelectionEntries = entries > 0 ? static_cast<std::uint32_t>(entries) : 0;
        resolver.Mark(UnitFact::SelectionEntries);
    }

    // The jump assist: the feature bit, the object state, two flag bits and the counter.
    auto ResolveJumpAssist(Resolver& resolver, std::uint32_t unit, std::optional<std::uint8_t> type) -> void
    {
        const UnitFactPort& port{ resolver.Port };
        auto& assist{ resolver.Context.JumpAssist };

        const std::optional<bool> isFeature{ port.IsFeatureOn(k_JumpAssistFeature) };
        if (!isFeature) return;

        assist.Applies = *isFeature;
        if (!assist.Applies)
        {
            resolver.Mark(UnitFact::JumpAssist);
            return;
        }

        // note: The engine hands back the object only for a biped (GetTagPointerWithPermissionCheck(unit, 1)), so a unit of another type
        // has no object and nothing else is read.
        if (!type) return;
        if (*type != k_ObjectTypeBiped)
        {
            assist.HasObject = false;
            resolver.Mark(UnitFact::JumpAssist);
            return;
        }

        const std::optional<std::size_t> stateBlock{ GetBlockOffset(port, unit, k_UnitStateOffset) };
        const std::optional<std::uint64_t> flags{ ReadObjectValue<std::uint64_t>(port, unit, k_UnitJumpFlags) };
        const std::optional<std::int8_t> counter{ ReadObjectValue<std::int8_t>(port, unit, k_UnitJumpCounter) };
        if (!stateBlock || !flags || !counter) return;

        const std::optional<std::uint32_t> state{ ReadObjectValue<std::uint32_t>(port, unit, *stateBlock) };
        if (!state) return;

        assist.HasObject = true;
        assist.IsStateOne = *state == 1;
        assist.IsFlagA = ((*flags >> k_UnitJumpFlagA) & 1u) != 0;
        assist.IsFlagB = ((*flags >> k_UnitJumpFlagB) & 1u) != 0;
        assist.Counter = *counter;
        assist.LimitHalf = resolver.Settings.TicksPerSecond / k_JumpAssistTicksDivisor;
        resolver.Mark(UnitFact::JumpAssist);
    }
}

namespace Engine::Control::Simulation::System
{
    auto BuildUnitContext(const UnitFactPort& port, const UnitClientState& state, const ClientContextSource& source,
        const ClientSettings& settings, float deltaTime) -> UnitContext
    {
        UnitContext context{ .Known = 0 };
        Resolver resolver{ .Port = port, .State = state, .Source = source, .Settings = settings, .Context = context };

        context.HasUnit = state.Unit != k_NoHandle;
        context.IsSeated = state.IsSeated;
        resolver.Mark(UnitFact::HasUnit);
        resolver.Mark(UnitFact::IsSeated);

        ResolveUser(resolver);
        resolver.Take(UnitFact::LookOffset, source.PlayerHandle != k_NoHandle ? port.IsLookOffset(source.PlayerHandle, deltaTime)
            : std::optional<bool>{ false }, context.IsLookOffset);

        if (!context.HasUnit)
        {
            // note: The client reads none of these without a unit, or reads the default the engine gives.
            for (const UnitFact fact : { UnitFact::Biped, UnitFact::SeatFlags, UnitFact::AltPitch, UnitFact::ZoomMagnification,
                UnitFact::MovePenalty, UnitFact::TriggerCurve, UnitFact::AimDamp, UnitFact::CrouchGate, UnitFact::WeaponCrouchFlag,
                UnitFact::ForcedCrouch, UnitFact::CrouchToggleBlocked, UnitFact::Carried, UnitFact::Ability, UnitFact::DualWield,
                UnitFact::ZoomItem, UnitFact::WeaponTriggerFlag, UnitFact::FrameOfReference, UnitFact::SelectionEntries,
                UnitFact::JumpAssist, UnitFact::TriggerCurveValue, UnitFact::TriggerCurveForward })
            {
                resolver.Mark(fact);
            }

            // note: The restrictions byte is read from the user settings, not from the unit, so without it the fact stays unknown.
            if (!source.DualWieldFlags) resolver.Known &= ~static_cast<std::uint32_t>(UnitFact::DualWield);

            context.IsBiped = false;
            context.Known = resolver.Known;
            return context;
        }

        const std::uint32_t unit{ state.Unit };
        const std::optional<std::uint8_t> type{ port.GetObjectType(unit) };
        if (type)
        {
            context.IsBiped = *type == k_ObjectTypeBiped;
            resolver.Mark(UnitFact::Biped);
        }

        if (source.ClientZoomLevel == k_NoZoom)
        {
            resolver.Mark(UnitFact::ZoomMagnification);
        }
        else
        {
            resolver.Take(UnitFact::ZoomMagnification, port.GetZoomMagnification(unit, source.ClientZoomLevel), context.ZoomMagnification);
        }

        ResolveSeat(resolver, unit);
        ResolveWeapon(resolver, unit);
        ResolveCrouch(resolver, unit, type);
        ResolveHeld(resolver, unit);
        ResolveUnitTag(resolver, unit);
        ResolveJumpAssist(resolver, unit, type);

        // note: Only a biped has a frame of reference.
        if (type && *type != k_ObjectTypeBiped)
        {
            resolver.Mark(UnitFact::FrameOfReference);
        }
        else if (type)
        {
            const std::optional<std::uint32_t> frame{ port.GetFrameOfReference(unit) };
            if (frame)
            {
                context.IsFrameOfReference = *frame != k_NoHandle;
                resolver.Mark(UnitFact::FrameOfReference);
            }
        }

        context.Known = resolver.Known;
        return context;
    }
}