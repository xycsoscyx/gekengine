#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"
#include "GEKEngineCLSIDs.h"
#include "GEKLogic.h"

DECLARE_REGISTERED_CLASS(CGEKPlayerState);
DECLARE_REGISTERED_CLASS(CGEKLightMoveState);

DECLARE_CONTEXT_SOURCE(Logic)
    ADD_CONTEXT_CLASS(CLSID_GEKPlayerState, CGEKPlayerState)
        ADD_CLASS_NAME("default_player_state")
    ADD_CONTEXT_CLASS(CLSID_GEKLightMoveState, CGEKLightMoveState)
        ADD_CLASS_NAME("light_move_state")
END_CONTEXT_SOURCE
