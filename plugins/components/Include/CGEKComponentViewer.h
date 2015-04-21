#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

DECLARE_COMPONENT(viewer, 0x00000003)
    DECLARE_COMPONENT_VALUE(float, field_of_view)
    DECLARE_COMPONENT_VALUE(float, minimum_distance)
    DECLARE_COMPONENT_VALUE(float, maximum_distance)
    DECLARE_COMPONENT_VALUE(float4, viewport)
    DECLARE_COMPONENT_VALUE(CStringW, pass)
END_DECLARE_COMPONENT(viewer)
