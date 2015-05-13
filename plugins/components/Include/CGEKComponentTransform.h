#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"

DECLARE_COMPONENT(transform, 0x00000001)
    DECLARE_COMPONENT_VALUE(Math::Float3, position)
    DECLARE_COMPONENT_VALUE(Math::Quaternion, rotation)
END_DECLARE_COMPONENT(transform)
