#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"
#include "GEKEngineCLSIDs.h"
#include "GEKModels.h"

DECLARE_REGISTERED_CLASS(CGEKStaticCollision);
DECLARE_REGISTERED_CLASS(CGEKStaticModel)
DECLARE_REGISTERED_CLASS(CGEKFactory)

DECLARE_CONTEXT_SOURCE(Models)
    ADD_CONTEXT_CLASS(CLSID_GEKStaticCollision, CGEKStaticCollision)
    ADD_CONTEXT_CLASS(CLSID_GEKStaticModel, CGEKStaticModel)
    ADD_CONTEXT_CLASS(CLSID_GEKFactory, CGEKFactory)
        ADD_CLASS_TYPE(CLSID_GEKFactoryType)
END_CONTEXT_SOURCE
