#pragma once

#include <Windows.h>
#include "GEK\Math\Float3.h"

namespace Gek
{
    DECLARE_INTERFACE_IID(NewtonProcessor, "3D803A52-D4E7-46E5-86FE-DA2659A366F4") : virtual public IUnknown
    {
        STDMETHOD_(Math::Float3, getGravity)            (THIS_ const Math::Float3 &position) PURE;
    };

    DECLARE_INTERFACE_IID(NewtonObserver, "A92E6C11-2A4D-48E9-B6C6-A2F282BC76B7") : virtual public Observer
    {
        STDMETHOD_(void, onCollision)                   (THIS_ Entity *entity0, Entity *entity1, const Math::Float3 &position, const Math::Float3 &normal) PURE;
    };
}; // namespace Gek
