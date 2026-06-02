#pragma once

// Hlmt-derived tags.
#include "Generated/Phmo/PhmoObject.h"
#include "Generated/Phmo/PhmoDescriptor.h"
#include "Generated/Coll/CollObject.h"
#include "Generated/Coll/CollDescriptor.h"
#include "Generated/Mode/ModeObject.h"
#include "Generated/Mode/ModeDescriptor.h"
#include "Generated/Jmad/JmadObject.h"
#include "Generated/Jmad/JmadDescriptor.h"

// Direct TagTable tags.
#include "Generated/Vehi/VehiObject.h"
#include "Generated/Vehi/VehiDescriptor.h"
#include "Generated/Bloc/BlocObject.h"
#include "Generated/Bloc/BlocDescriptor.h"
#include "Generated/Sbsp/SbspObject.h"
#include "Generated/Sbsp/SbspDescriptor.h"
#include "Generated/Weap/WeapObject.h"
#include "Generated/Weap/WeapDescriptor.h"
#include "Generated/Proj/ProjObject.h"
#include "Generated/Proj/ProjDescriptor.h"
#include "Generated/Bipd/BipdObject.h"
#include "Generated/Bipd/BipdDescriptor.h"
#include "Generated/Eqip/EqipObject.h"
#include "Generated/Eqip/EqipDescriptor.h"
#include "Generated/Scen/ScenObject.h"
#include "Generated/Scen/ScenDescriptor.h"
#include "Generated/Scnr/ScnrObject.h"
#include "Generated/Scnr/ScnrDescriptor.h"
#include "Generated/Ctrl/CtrlObject.h"
#include "Generated/Ctrl/CtrlDescriptor.h"
#include "Generated/Mach/MachObject.h"
#include "Generated/Mach/MachDescriptor.h"
#include "Generated/Zone/ZoneObject.h"
#include "Generated/Zone/ZoneDescriptor.h"
#include "Generated/Play/PlayObject.h"
#include "Generated/Play/PlayDescriptor.h"
#include "Generated/Lbsp/LbspObject.h"
#include "Generated/Lbsp/LbspDescriptor.h"
#include "Generated/Sldt/SldtObject.h"
#include "Generated/Sldt/SldtDescriptor.h"

class TagGroupReader;

class State_MapReader;
class System_MapReader;
class System_Logs;
class State_MapPhmo;
class State_MapColl;
class State_MapMode;
class State_MapJmad;
class State_MapVehi;
class State_MapBloc;
class State_MapSbsp;
class State_MapWeap;
class State_MapProj;
class State_MapBipd;
class State_MapEqip;
class State_MapScen;
class State_MapScnr;
class State_MapCtrl;
class State_MapMach;
class State_MapZone;
class State_MapPlay;
class State_MapSldt;
class State_MapLbsp;
class System_MapReader;
class System_Logs;

struct System_TagGroupReader_Dependencies
{
    State_MapReader& State_Map;
    State_MapPhmo& State_MapPhmo;
    State_MapColl& State_MapColl;
    State_MapMode& State_MapMode;
    State_MapJmad& State_MapJmad;
    State_MapVehi& State_MapVehi;
    State_MapBloc& State_MapBloc;
    State_MapSbsp& State_MapSbsp;
    State_MapWeap& State_MapWeap;
    State_MapProj& State_MapProj;
    State_MapBipd& State_MapBipd;
    State_MapEqip& State_MapEqip;
    State_MapScen& State_MapScen;
    State_MapScnr& State_MapScnr;
    State_MapCtrl& State_MapCtrl;
    State_MapMach& State_MapMach;
    State_MapZone& State_MapZone;
    State_MapPlay& State_MapPlay;
    State_MapSldt& State_MapSldt;
    State_MapLbsp& State_MapLbsp;
    System_MapReader& System_Header;
    System_Logs& System_Logs;
};

class System_TagGroupReader
{
public:
    System_TagGroupReader(System_TagGroupReader_Dependencies dependencies) :
        m_Deps(dependencies) {}
    ~System_TagGroupReader() = default;

    void LoadForMap();

    void Cleanup();

private:
    System_TagGroupReader_Dependencies m_Deps;

    void LoadHlmtFamily(FILE* file, TagGroupReader& reader);
    void LoadDirectTags(FILE* file, TagGroupReader& reader);
};