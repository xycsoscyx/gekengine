#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

DECLARE_COMPONENT(light)
    DECLARE_COMPONENT_DATA(float3, color)
    DECLARE_COMPONENT_DATA(float, range)
END_DECLARE_COMPONENT(light)
