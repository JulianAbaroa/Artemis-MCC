export module Engine.Control.Simulation.System:UnitContext;

import Control.Action.Type;
import Control.Client.Type;
import std;

export namespace Engine::Control::Simulation::System
{
    // What the unit facts of the client read from the engine (VW.9c.2): object and tag memory, and the engine functions. The engine side
    // implements it (BotService), the tests give bytes. A read or a call that cannot be done says so (false or nullopt) and the fact
    // stays out of UnitContext::Known.
    // note: Game thread only (the engine functions read the TLS).
    class UnitFactPort
    {
    public:
        virtual ~UnitFactPort() = default;

        // Reads bytes of the data of a live object at an offset.
        virtual auto ReadObject(std::uint32_t handle, std::size_t offset, std::span<std::uint8_t> bytes) const -> bool = 0;

        // return: The type byte of the object header (0 biped, 1 vehicle), or nullopt if the handle is not a live object.
        virtual auto GetObjectType(std::uint32_t handle) const -> std::optional<std::uint8_t> = 0;

        // Reads bytes of a player datum at an offset.
        virtual auto ReadPlayer(std::uint32_t handle, std::size_t offset, std::span<std::uint8_t> bytes) const -> bool = 0;

        // Reads bytes of the data of a tag (the low 16 bits of the handle index it) at an offset.
        virtual auto ReadTag(std::uint32_t tag, std::size_t offset, std::span<std::uint8_t> bytes) const -> bool = 0;

        // Reads bytes of a tag block: the data index is the one the tag gives, and the offset is in bytes from it.
        virtual auto ReadBlock(std::uint32_t index, std::size_t offset, std::span<std::uint8_t> bytes) const -> bool = 0;

        // return: Whether the game feature bit is set, or nullopt if the game options are not there.
        virtual auto IsFeatureOn(std::uint32_t bit) const -> std::optional<bool> = 0;

        // return: The magnification of the weapon of the unit at a zoom level (0x4856b8), or nullopt if the function was not found.
        virtual auto GetZoomMagnification(std::uint32_t unit, std::int16_t level) const -> std::optional<float> = 0;

        // return: The fraction the weapon takes off the forward or the side axis (0x4bc51c).
        virtual auto GetMovePenalty(std::uint32_t weapon, bool isForward, bool isZoomed) const -> std::optional<float> = 0;

        // return: The crouch gate of the unit (0x4ac810).
        virtual auto IsCrouchGate(std::uint32_t unit) const -> std::optional<bool> = 0;

        // return: Whether the unit is forced to crouch (0x4abee8).
        virtual auto IsForcedCrouch(std::uint32_t unit) const -> std::optional<bool> = 0;

        // return: Whether the engine clears the crouch toggle of the unit (0x48be48).
        virtual auto IsCrouchToggleBlocked(std::uint32_t unit) const -> std::optional<bool> = 0;

        // return: The next zoom level of the weapon from the level -1 (0x4bc624).
        virtual auto GetNextZoomLevel(std::uint32_t weapon) const -> std::optional<std::int16_t> = 0;

        // return: The handle of the frame of reference of the unit, 0xFFFFFFFF if none (0x5dddc).
        virtual auto GetFrameOfReference(std::uint32_t unit) const -> std::optional<std::uint32_t> = 0;

        // return: Whether the controls of the input user are locked (0x61830).
        virtual auto IsControlsLocked(std::int32_t inputUser, std::uint32_t localIndex) const -> std::optional<bool> = 0;

        // return: Whether a flinch or shake pushes the aim of the player (0xda824).
        virtual auto IsLookOffset(std::uint32_t player, float deltaTime) const -> std::optional<bool> = 0;

        // return: Whether the player is in the alternate control mode (0x715ac).
        virtual auto IsAltControl(std::uint32_t player) const -> std::optional<bool> = 0;

        // return: Whether the control profile of the player makes the boost come from the alternate event (0x70ed0).
        virtual auto IsBoostAltEvent(std::uint32_t player) const -> std::optional<bool> = 0;

        // return: The raw boost value after the tag function at +0x498 of the tag (0x174ed8 then 0x175524), or nullopt if it cannot be run.
        // param tag: Handle of the unit tag, or of the parent tag for a seat with the flag bit 2.
        // param value: Raw boost value, not zero.
        virtual auto EvaluateTriggerCurve(std::uint32_t tag, float value) const -> std::optional<float> = 0;
    };

    // Resolves every fact of the unit the client asks (D356, VW.9c.2) with the port. The facts of the unit come from its memory, its
    // weapon, its parent and their tags, or from the engine function the decompiled 0x5df80 calls. The user-level facts (controls lock,
    // look offset, alternate control, look rates, dual wield restrictions) come from the source.
    // param state: Client state of the unit (the unit handle, whether it sits).
    // param source: The user-level ids and the zoom level of the client.
    // param settings: The settings of the client (the ticks per second give the jump assist limit).
    // param deltaTime: Seconds of the call, the look offset function takes them.
    // return: The context. A fact the port could not resolve is outside Known. Facts that only matter with a unit are known as their
    // default when there is none.
    auto BuildUnitContext(const UnitFactPort& port, const ::Control::Action::Type::UnitClientState& state,
        const ::Control::Client::Type::ClientContextSource& source, const ::Control::Client::Type::ClientSettings& settings,
        float deltaTime) -> ::Control::Client::Type::UnitContext;
}