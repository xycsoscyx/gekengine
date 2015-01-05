#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"
#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"
#include "GEKFlames.h"

DECLARE_REGISTERED_CLASS(CGEKComponentflames)
DECLARE_REGISTERED_CLASS(CGEKComponentSystemFlames);

DECLARE_CONTEXT_SOURCE(Flames)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentFlames, CGEKComponentflames)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemFlames, CGEKComponentSystemFlames)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
END_CONTEXT_SOURCE
