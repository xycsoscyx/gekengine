#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"

#include "GEKAPICLSIDs.h"
#include "GEKComponentsCLSIDs.h"
#include "GEKEngineCLSIDs.h"

DECLARE_REGISTERED_CLASS(CGEKComponentTransform)
DECLARE_REGISTERED_CLASS(CGEKComponentViewer)
DECLARE_REGISTERED_CLASS(CGEKComponentLight)
DECLARE_REGISTERED_CLASS(CGEKComponentModel)
DECLARE_REGISTERED_CLASS(CGEKComponentController)
DECLARE_REGISTERED_CLASS(CGEKComponentNewton)

DECLARE_REGISTERED_CLASS(CGEKComponentSystemController)
DECLARE_REGISTERED_CLASS(CGEKComponentSystemNewton)

DECLARE_CONTEXT_SOURCE(Components)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentTransform, CGEKComponentTransform)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)

    ADD_CONTEXT_CLASS(CLSID_GEKComponentViewer, CGEKComponentViewer)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)

    ADD_CONTEXT_CLASS(CLSID_GEKComponentLight, CGEKComponentLight)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)

    ADD_CONTEXT_CLASS(CLSID_GEKComponentModel, CGEKComponentModel)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)

    ADD_CONTEXT_CLASS(CLSID_GEKComponentController, CGEKComponentController)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemController, CGEKComponentSystemController)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)

    ADD_CONTEXT_CLASS(CLSID_GEKComponentNewton, CGEKComponentNewton)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemNewton, CGEKComponentSystemNewton)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
END_CONTEXT_SOURCE