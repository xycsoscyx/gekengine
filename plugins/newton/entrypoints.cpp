#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"

#include "GEKAPICLSIDs.h"
#include "GEKNewtonCLSIDs.h"
#include "GEKEngineCLSIDs.h"

DECLARE_REGISTERED_CLASS(CGEKComponentnewton)

DECLARE_REGISTERED_CLASS(CGEKComponentSystemNewton)

DECLARE_CONTEXT_SOURCE(Newton)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentNewton, CGEKComponentnewton)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemNewton, CGEKComponentSystemNewton)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
END_CONTEXT_SOURCE