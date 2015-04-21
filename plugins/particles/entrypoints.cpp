#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"
#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"
#include "GEKParticles.h"

DECLARE_REGISTERED_CLASS(CGEKComponentSystemGravity);
DECLARE_REGISTERED_CLASS(CGEKComponentSystemOffset);
DECLARE_REGISTERED_CLASS(CGEKComponentSystemParticle);

DECLARE_CONTEXT_SOURCE(Flames)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemGravity, CGEKComponentSystemGravity)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemOffset, CGEKComponentSystemOffset)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemParticle, CGEKComponentSystemParticle)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
END_CONTEXT_SOURCE
