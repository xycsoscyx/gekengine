#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

DECLARE_COMPONENT(viewer)
    DECLARE_COMPONENT_DATA(float, fov)
    DECLARE_COMPONENT_DATA(float, mindistance)
    DECLARE_COMPONENT_DATA(float, maxdistance)
    DECLARE_COMPONENT_DATA(float4, viewport)
    DECLARE_COMPONENT_DATA(CStringW, pass)
END_DECLARE_COMPONENT(viewer)
