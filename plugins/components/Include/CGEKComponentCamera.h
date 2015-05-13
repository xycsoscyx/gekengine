#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"

DECLARE_COMPONENT(camera, 0x00000100)
    DECLARE_COMPONENT_VALUE(float, field_of_view)
    DECLARE_COMPONENT_VALUE(float, minimum_distance)
    DECLARE_COMPONENT_VALUE(float, maximum_distance)
    DECLARE_COMPONENT_VALUE(Math::Float4, viewport)
END_DECLARE_COMPONENT(camera)
