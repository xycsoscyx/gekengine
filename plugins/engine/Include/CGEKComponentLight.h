#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

DECLARE_COMPONENT(light, 0x00000002)
    DECLARE_COMPONENT_VALUE(float3, color)
    DECLARE_COMPONENT_VALUE(float, range)
END_DECLARE_COMPONENT(light)
