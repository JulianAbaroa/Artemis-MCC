#pragma once

#include "ObjectClass.h"
#include <string>

// Identification of a game engine's object.
struct ObjectProfile
{
    ObjectClass PrimaryClass = ObjectClass::Invalid;

    // These flags allow us to know what game objects shares the same 'Tag Name'.
    bool HasBipd = false;   // biped.
    bool HasBloc = false;   // crate.
    bool HasColl = false;   // collision_model.
    bool HasCtrl = false;   // device_control.
    bool HasEqip = false;   // equipment.
    bool HasJmad = false;   // model_animation_graph.
    bool HasMach = false;   // machine.
    bool HasMode = false;   // render_model.
    bool HasPhmo = false;   // physics_model.
    bool HasProj = false;   // projectile.
    bool HasScen = false;   // scenery.
    bool HasScnr = false;   // scenario.
    bool HasVehi = false;   // vehicle.
    bool HasWeap = false;   // weapon.
};