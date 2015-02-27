#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"

#include "GEKSystemCLSIDs.h"

DECLARE_REGISTERED_CLASS(CGEKInputSystem)
DECLARE_REGISTERED_CLASS(CGEKAudioSystem)
DECLARE_REGISTERED_CLASS(CGEKVideoSystem)

DECLARE_CONTEXT_SOURCE(System)
    ADD_CONTEXT_CLASS(CLSID_GEKInputSystem, CGEKInputSystem)
    ADD_CONTEXT_CLASS(CLSID_GEKAudioSystem, CGEKAudioSystem)
    ADD_CONTEXT_CLASS(CLSID_GEKVideoSystem, CGEKVideoSystem)
END_CONTEXT_SOURCE