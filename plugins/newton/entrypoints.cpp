#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"

#include "GEKAPICLSIDs.h"
#include "GEKNewtonCLSIDs.h"
#include "GEKEngineCLSIDs.h"

DECLARE_REGISTERED_CLASS(CGEKComponentdynamicbody)
DECLARE_REGISTERED_CLASS(CGEKComponentplayer)

DECLARE_REGISTERED_CLASS(CGEKComponentSystemNewton)

DECLARE_CONTEXT_SOURCE(Newton)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentDynamicBody, CGEKComponentdynamicbody)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentPlayer, CGEKComponentplayer)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemNewton, CGEKComponentSystemNewton)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
END_CONTEXT_SOURCE