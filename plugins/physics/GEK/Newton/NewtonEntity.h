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

        STDMETHOD_(void, onApplyForceAndTorque)         (THIS_ dFloat frameTime, int threadHandle) { };
        STDMETHOD_(void, onSetTransform)                (THIS_ const dFloat* const matrixData, int threadHandle) { };
        STDMETHOD_(void, onPreUpdate)                   (THIS_ dFloat frameTime, int threadHandle) { };
        STDMETHOD_(void, onPostUpdate)                  (THIS_ dFloat frameTime, int threadHandle) { };
    };
}; // namespace Gek
