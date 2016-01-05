#pragma once

#include <Windows.h>
#include "GEK\Math\Vector3.h"

namespace Gek
{
    DECLARE_INTERFACE(NewtonProcessor) : virtual public IUnknown
    {
        STDMETHOD_(Math::Float3 , getGravity)           (THIS_ const Math::Float3 &position) PURE;
    };
}; // namespace Gek
