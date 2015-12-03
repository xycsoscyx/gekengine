#pragma once

#include <Windows.h>

namespace Gek
{
    DECLARE_INTERFACE(Context);

    DECLARE_INTERFACE_IID(ContextUser, "C66EB343-8E2B-47CE-BEF7-09F7B57AF7FD") : virtual public IUnknown
    {
        STDMETHOD_(void, registerContext)           (THIS_ Context *context) PURE;
    };
}; // namespace Gek
