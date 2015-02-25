#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"

#include "GEKSystemCLSIDs.h"

DECLARE_REGISTERED_CLASS(CGEKVideoSystem)
DECLARE_REGISTERED_CLASS(CGEKAudioSystem)

DECLARE_CONTEXT_SOURCE(System)
    ADD_CONTEXT_CLASS(CLSID_GEKVideoSystem, CGEKVideoSystem)
    ADD_CONTEXT_CLASS(CLSID_GEKAudioSystem, CGEKAudioSystem)
END_CONTEXT_SOURCE