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

class TagGroupReader;

class MapTagGroupSystem
{
public:
    void LoadForMap();
    void Cleanup();

private:
    void LoadHlmtFamily(FILE* file, TagGroupReader& reader);
    void LoadDirectTags(FILE* file, TagGroupReader& reader);
};