#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

DECLARE_COMPONENT(viewer)
    DECLARE_COMPONENT_DATA(float, fieldofview)
    DECLARE_COMPONENT_DATA(float, minviewdistance)
    DECLARE_COMPONENT_DATA(float, maxviewdistance)
    DECLARE_COMPONENT_DATA(float4, viewport)
    DECLARE_COMPONENT_DATA(CStringW, pass)
END_DECLARE_COMPONENT(viewer)
