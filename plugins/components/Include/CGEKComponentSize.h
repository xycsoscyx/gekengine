#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"

DECLARE_COMPONENT(size, float, 0x00000012)
    DECLARE_COMPONENT_VALUE(float, minimum)
    DECLARE_COMPONENT_VALUE(float, maximum)
END_DECLARE_COMPONENT(size)
