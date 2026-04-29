#pragma once

#include <cstdint>
#include <string>

// TagGroup, primary classification by magic tag group.
enum class TagGroup : uint8_t
{
	Unknown = 0,

	// Interactable.
	Weapon,				// "weap".
	Equipment,			// "eqip".
	Vehicle,			// "vehi".
	DeviceControl,		// "ctrl".

	// Navigation.
	Projectile,			// "proj".
	Scenery,			// "scen".
	Crate,				// "bloc".
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