#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"
#include "GEKEngineCLSIDs.h"
#include "GEKLogic.h"

DECLARE_REGISTERED_CLASS(CGEKPlayerState);

DECLARE_CONTEXT_SOURCE(Logic)
    ADD_CONTEXT_CLASS(CLSID_GEKPlayerState, CGEKPlayerState)
        ADD_CLASS_NAME("default_player_state")
END_CONTEXT_SOURCE
