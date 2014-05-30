#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"

#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"
#include "GEKAPICLSIDs.h"

DECLARE_REGISTERED_CLASS(CGEKRenderFilter)
DECLARE_REGISTERED_CLASS(CGEKRenderManager)
DECLARE_REGISTERED_CLASS(CGEKResourceManager)
DECLARE_REGISTERED_CLASS(CGEKTextureProvider)
DECLARE_REGISTERED_CLASS(CGEKPopulationManager)
DECLARE_REGISTERED_CLASS(CGEKEngine)

DECLARE_CONTEXT_SOURCE(Engine)
    ADD_CONTEXT_CLASS(CLSID_GEKRenderFilter, CGEKRenderFilter)
    ADD_CONTEXT_CLASS(CLSID_GEKRenderManager, CGEKRenderManager)
    ADD_CONTEXT_CLASS(CLSID_GEKResourceManager, CGEKResourceManager)
    ADD_CONTEXT_CLASS(CLSID_GEKTextureProvider, CGEKTextureProvider)
        ADD_CLASS_TYPE(CLSID_GEKResourceProviderType)
    ADD_CONTEXT_CLASS(CLSID_GEKPopulationManager, CGEKPopulationManager)
    ADD_CONTEXT_CLASS(CLSID_GEKEngine, CGEKEngine)
END_CONTEXT_SOURCE
