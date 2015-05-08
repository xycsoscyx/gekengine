#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"

#include "GEKAPICLSIDs.h"
#include "GEKComponentsCLSIDs.h"

DECLARE_REGISTERED_CLASS(CGEKComponenttransform)
DECLARE_REGISTERED_CLASS(CGEKComponentcamera)
DECLARE_REGISTERED_CLASS(CGEKComponentpointlight)
DECLARE_REGISTERED_CLASS(CGEKComponentcolor)
DECLARE_REGISTERED_CLASS(CGEKComponentfollow)

DECLARE_REGISTERED_CLASS(CGEKComponentSystemFollow)

DECLARE_CONTEXT_SOURCE(Components)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentTransform, CGEKComponenttransform)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)

    ADD_CONTEXT_CLASS(CLSID_GEKComponentCamera, CGEKComponentcamera)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)

    ADD_CONTEXT_CLASS(CLSID_GEKComponentPointLight, CGEKComponentpointlight)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)

    ADD_CONTEXT_CLASS(CLSID_GEKComponentColor, CGEKComponentcolor)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)

    ADD_CONTEXT_CLASS(CLSID_GEKComponentFollow, CGEKComponentfollow)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)

    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemFollow, CGEKComponentSystemFollow)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
END_CONTEXT_SOURCE