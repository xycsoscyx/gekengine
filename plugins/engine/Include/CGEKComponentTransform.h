#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

DECLARE_COMPONENT(transform, CStringW, 0x00000001)
    DECLARE_COMPONENT_VALUE(float3, position)
    DECLARE_COMPONENT_VALUE(quaternion, rotation)
END_DECLARE_COMPONENT(transform)
