#pragma once

#include <Windows.h>

namespace Gek
{
    namespace Context
    {
        DECLARE_INTERFACE(Interface);

        DECLARE_INTERFACE_IID_(UserInterface, IUnknown, "C66EB343-8E2B-47CE-BEF7-09F7B57AF7FD")
        {
            STDMETHOD_(void, registerContext)           (THIS_ Interface *context) PURE;
        };
    }; // namespace Context
}; // namespace Gek
