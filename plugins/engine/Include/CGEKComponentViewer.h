#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

DECLARE_COMPONENT(viewer, 0x00000003)
    DECLARE_COMPONENT_VALUE(float, fieldofview)
    DECLARE_COMPONENT_VALUE(float, mindistance)
    DECLARE_COMPONENT_VALUE(float, maxdistance)
    DECLARE_COMPONENT_VALUE(float2, position)
    DECLARE_COMPONENT_VALUE(float2, size)
    DECLARE_COMPONENT_VALUE(CStringW, pass)
END_DECLARE_COMPONENT(viewer)
