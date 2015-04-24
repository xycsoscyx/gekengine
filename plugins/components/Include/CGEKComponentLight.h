#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

DECLARE_COMPONENT(pointlight, 0x00000010)
    DECLARE_COMPONENT_VALUE(float, radius)
END_DECLARE_COMPONENT(pointlight)
