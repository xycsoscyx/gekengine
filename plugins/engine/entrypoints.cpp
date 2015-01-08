#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"

#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"
#include "GEKAPICLSIDs.h"

DECLARE_REGISTERED_CLASS(CGEKComponenttransform)
DECLARE_REGISTERED_CLASS(CGEKComponentviewer)
DECLARE_REGISTERED_CLASS(CGEKComponentlight)

DECLARE_REGISTERED_CLASS(CGEKRenderMaterial)
DECLARE_REGISTERED_CLASS(CGEKRenderFilter)
DECLARE_REGISTERED_CLASS(CGEKRenderSystem)
DECLARE_REGISTERED_CLASS(CGEKPopulationSystem)
DECLARE_REGISTERED_CLASS(CGEKEngine)

DECLARE_CONTEXT_SOURCE(Engine)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentTransform, CGEKComponenttransform)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)

    ADD_CONTEXT_CLASS(CLSID_GEKComponentViewer, CGEKComponentviewer)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)

    ADD_CONTEXT_CLASS(CLSID_GEKComponentLight, CGEKComponentlight)
        ADD_CLASS_TYPE(CLSID_GEKComponentType)

    ADD_CONTEXT_CLASS(CLSID_GEKRenderMaterial, CGEKRenderMaterial)

    ADD_CONTEXT_CLASS(CLSID_GEKRenderFilter, CGEKRenderFilter)

    ADD_CONTEXT_CLASS(CLSID_GEKRenderSystem, CGEKRenderSystem)

    ADD_CONTEXT_CLASS(CLSID_GEKPopulationSystem, CGEKPopulationSystem)

    ADD_CONTEXT_CLASS(CLSID_GEKEngine, CGEKEngine)
END_CONTEXT_SOURCE
