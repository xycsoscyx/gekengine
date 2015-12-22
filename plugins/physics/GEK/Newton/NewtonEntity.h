#pragma once

#include <Windows.h>
#include "GEK\Engine\Entity.h"
#include <Newton.h>

namespace Gek
{
    DECLARE_INTERFACE(NewtonEntity) : virtual public IUnknown
    {
        STDMETHOD_(Entity *, getEntity)                 (THIS) const PURE;
        STDMETHOD_(NewtonBody *, getNewtonBody)         (THIS) const PURE;
    };
}; // namespace Gek
