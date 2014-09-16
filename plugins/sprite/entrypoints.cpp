#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"
#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"
#include "GEKSprite.h"

DECLARE_REGISTERED_CLASS(CGEKComponentSprite)
DECLARE_REGISTERED_CLASS(CGEKComponentSystemSprite);

DECLARE_CONTEXT_SOURCE(Sprite)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSprite, CGEKComponentSprite)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemSprite, CGEKComponentSystemSprite)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
END_CONTEXT_SOURCE
