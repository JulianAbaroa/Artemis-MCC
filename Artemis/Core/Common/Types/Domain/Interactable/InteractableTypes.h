#pragma once

// TODO: For now we CAN'T know if a biped is on driver seat or passanger seat, 
// it's an edge-case. Maybe when we get the vehi data, we could get know it somehow.
// TODO: Child objects doesnt have world-position. This means vehicles childs
// shows incorrect world-position. AI knows there's more than one seat, but
// it doesnt know where, spatially, in the world they are. What we can do with this?
// We can get the exact positions with:
// 'vehi' tag -> Seat Marker Name -> 'mode' tag -> seat position.
// This should work to obtain any local position inside any model (from 'mode' tag).

// TODO: We can get the bipeds bounding radius from the 'bipd' tag.
// TODO: We can get the exact geometry and collision in model-space for any object
// from 'coll' tag, but as static data. This is useful for static objects like
// 'bloc' or 'scen' object types.
// TODO: We can get the map geometry and collisions from the 'sbsp' tag, 
// (scenario_structure_bsp) which contains the vertices, surfaces, etc, of the map.
// 
// TODO: ObservationSystem — consumes InteractableState, PhmoState,
// InteractionTableState and PlayerTableState, etcetera, normalizes
// all values, and produces a fixed-dimension observation vector for the RL agent.

#pragma once

#include "Core/Common/Types/Domain/Tables/InteractionTypes.h"
//#include "Core/Common/Types/Domain/Map/VehiTypes.h"
#include <unordered_map>
#include <cstdint>
#include <string>
#include <vector>

// TagGroup, primary classification by magic tag group.
enum class TagGroup : uint8_t
{
    Unknown = 0,
    Crate,          // "bloc"
    Collision,      // "coll"
    Control,        // "ctrl"
    Equipment,      // "eqip"
    Animation,      // "jmad"
    Model,          // "mode"
    Physic,         // "phmo"
    Projectile,     // "proj"
    Scenery,        // "scen"
    Vehicle,        // "vehi"
    Weapon,         // "weap"
};

// TagProfile, sub-tags present in this object and calculated derivatives.
struct TagProfile
{
    TagGroup PrimaryGroup = TagGroup::Unknown;

    // Present sub-tags, populated by their respective systems when loading the map.
    bool HasBloc = false;   // crate.
    bool HasColl = false;   // collision_model.
    bool HasCtrl = false;   // device_control.
    bool HasEqip = false;   // equipment.
    bool HasJmad = false;   // model_animation_graph.
    bool HasMode = false;   // render_model.
    bool HasPhmo = false;   // physics_model.
    bool HasProj = false;   // projectile.
    bool HasScen = false;   // scenery.
    bool HasVehi = false;   // vehicle.
    bool HasWeap = false;   // weapon.
};

// InteractableBehavior, behavior layer derived from TagGroup + TagProfile.
enum class InteractableBehavior : uint8_t
{
    None,

    // Pick up object from the floor (weap/eqip without parent).
    Pickup,

    // Get into a vehicle with an available seat.
    EnterVehicle,

    // Avoid (projectiles, explosives).
    Avoid,

    // Activate mechanism (Reach has something like this?).
    Interact,
};

// How the player triggers the interaction.
enum class InteractableActivation : uint8_t
{
    // No direct interaction, world state entity.
    None,

    // Player must press the interact key while looking at the object.
    KeyPress,   

    // Triggered automatically when the player enters the object's hitbox.
    Proximity,
};

struct SeatStatus
{
    // Human-readable seat name derived from AISeatType
    // (e.g. "Driver", "Gunner"). Falls back to "Seat" if vehi
    // tag data is unavailable.
    std::string SeatName;

    // Semantic type from the vehi tag's AI Seat Type enum.
    // AISeatType::None when vehi data is unavailable.
    //AISeatType AISeatType = AISeatType::None;

    bool IsOccupied = false;
    uint32_t OccupyingBipedHandle = 0xFFFFFFFF;

    // World-space position of this seat, computed each tick as:
    //   vehicle.Position + rotate(LocalPosition, vehicle.Forward, vehicle.Up)
    // Falls back to vehicle root if LocalPosition was not resolved from mode.
    float SeatWorldPosition[3] = {};

    // Distance from the AI player to this specific seat's world position.
    // Computed in PassesVehicleFilter each tick using self.WeaponPosition.
    // Use this (not interactable.DistanceToPlayer) for seat-level navigation.
    float DistanceToPlayer = 0.0f;

};


struct AIInteractable
{
    uint32_t Handle;
    uint32_t ParentHandle;
    std::string TagName;

    TagGroup PrimaryGroup = TagGroup::Unknown;

    // Complete profile so that downstream systems can inspect it.
    TagProfile Profile;

    std::vector<InteractableBehavior> Behaviors;

    InteractableActivation Activation = InteractableActivation::None;

    float Position[3]{};
    float Forward[3]{};
    float Up[3]{};

    float DistanceToPlayer = 0.0f;

    // True if the engine currently has this object selected as the
    // interaction target for the self player.
    bool IsEngineSelected = false;

    // Occupancy is derived from the graph (bipeds are children of the vehicle node).
    // Seat position offsets will come from the vehicle tag when available;
    // For now, the root vehicle position is used as a fallback.
    std::vector<SeatStatus> Seats;

    // Handles of child interactables (e.g. seats of a vehicle).
    // Populated only when PrimaryGroup == Vehicle.
    std::vector<uint32_t> ChildHandles;
};