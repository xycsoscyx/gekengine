#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"
#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"
#include "GEKModel.h"

DECLARE_REGISTERED_CLASS(CGEKComponentmodel)
DECLARE_REGISTERED_CLASS(CGEKComponentSystemModel);

DECLARE_CONTEXT_SOURCE(Model)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentModel, CGEKComponentmodel)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemModel, CGEKComponentSystemModel)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
END_CONTEXT_SOURCE
