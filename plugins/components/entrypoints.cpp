#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"

#include "GEKAPICLSIDs.h"
#include "GEKComponentsCLSIDs.h"
#include "GEKEngineCLSIDs.h"

DECLARE_REGISTERED_CLASS(CGEKComponentSystemTransform)
DECLARE_REGISTERED_CLASS(CGEKComponentSystemNewton)
DECLARE_REGISTERED_CLASS(CGEKComponentSystemModel)
DECLARE_REGISTERED_CLASS(CGEKComponentSystemLight)
DECLARE_REGISTERED_CLASS(CGEKComponentSystemViewer)
DECLARE_REGISTERED_CLASS(CGEKComponentSystemLogic)

DECLARE_CONTEXT_SOURCE(Components)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemTransform, CGEKComponentSystemTransform)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemNewton, CGEKComponentSystemNewton)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemStaticMesh, CGEKComponentSystemModel)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemLight, CGEKComponentSystemLight)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemViewer, CGEKComponentSystemViewer)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemLogic, CGEKComponentSystemLogic)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
END_CONTEXT_SOURCE