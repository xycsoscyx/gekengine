#pragma once

#include <Windows.h>

namespace Gek
{
    namespace Context
    {
        DECLARE_INTERFACE(Interface);

        namespace User
        {
            DECLARE_INTERFACE_IID(Interface, "C66EB343-8E2B-47CE-BEF7-09F7B57AF7FD") : virtual public IUnknown
            {
                STDMETHOD_(void, registerContext)           (THIS_ Context::Interface *context) PURE;
            };
        }; // namespace User
    }; // namespace Context
}; // namespace Gek
