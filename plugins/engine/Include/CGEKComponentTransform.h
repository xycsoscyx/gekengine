#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

DECLARE_COMPONENT(transform)
    DECLARE_COMPONENT_DATA(float3, position)
    DECLARE_COMPONENT_DATA(quaternion, rotation)
END_DECLARE_COMPONENT(transform)
