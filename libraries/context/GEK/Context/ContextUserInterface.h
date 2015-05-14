#pragma once

#include <Windows.h>

namespace Gek
{
    DECLARE_INTERFACE(ContextInterface);

    DECLARE_INTERFACE_IID_(ContextUserInterface, IUnknown, "C66EB343-8E2B-47CE-BEF7-09F7B57AF7FD")
    {
        STDMETHOD_(void, registerContext)           (THIS_ ContextInterface *context) PURE;
    };
}; // namespace Gek
