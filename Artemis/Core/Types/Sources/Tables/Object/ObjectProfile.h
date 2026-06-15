#pragma once

#include "ObjectClass.h"

// Identification of a game engine's object.
struct ObjectProfile
{
    ObjectClass Class = ObjectClass::Invalid;

    bool HasBipd = false;
    bool HasBloc = false;
    bool HasColl = false;
    bool HasCtrl = false;
    bool HasEqip = false;
    bool HasHlmt = false;
    bool HasLbsp = false;
    bool HasMach = false;
    bool HasMode = false;
    bool HasPhmo = false;
    bool HasPlay = false;
    bool HasProj = false;
    bool HasSbsp = false;
    bool HasScen = false;
    bool HasScnr = false;
    bool HasSldt = false;
    bool HasVehi = false;
    bool HasWeap = false;
    bool HasZone = false;
};