#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"
#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"
#include "GEKModels.h"

DECLARE_REGISTERED_CLASS(CGEKStaticCollision);
DECLARE_REGISTERED_CLASS(CGEKStaticModel)
DECLARE_REGISTERED_CLASS(CGEKStaticProvider)

DECLARE_CONTEXT_SOURCE(Models)
    ADD_CONTEXT_CLASS(CLSID_GEKStaticCollision, CGEKStaticCollision)
    ADD_CONTEXT_CLASS(CLSID_GEKStaticModel, CGEKStaticModel)
    ADD_CONTEXT_CLASS(CLSID_GEKStaticProvider, CGEKStaticProvider)
        ADD_CLASS_TYPE(CLSID_GEKResourceProviderType)
END_CONTEXT_SOURCE
