#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"
#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"
#include "GEKModels.h"

DECLARE_REGISTERED_CLASS(CGEKStaticCollision);
DECLARE_REGISTERED_CLASS(CGEKStaticModel)
DECLARE_REGISTERED_CLASS(CGEKStaticFactory)
DECLARE_REGISTERED_CLASS(CGEKSpriteModel)
DECLARE_REGISTERED_CLASS(CGEKSpriteFactory)

DECLARE_CONTEXT_SOURCE(Models)
    ADD_CONTEXT_CLASS(CLSID_GEKStaticCollision, CGEKStaticCollision)
    ADD_CONTEXT_CLASS(CLSID_GEKStaticModel, CGEKStaticModel)
    ADD_CONTEXT_CLASS(CLSID_GEKStaticFactory, CGEKStaticFactory)
        ADD_CLASS_TYPE(CLSID_GEKFactoryType)
    ADD_CONTEXT_CLASS(CLSID_GEKSpriteModel, CGEKSpriteModel)
    ADD_CONTEXT_CLASS(CLSID_GEKSpriteFactory, CGEKSpriteFactory)
        ADD_CLASS_TYPE(CLSID_GEKFactoryType)
END_CONTEXT_SOURCE
