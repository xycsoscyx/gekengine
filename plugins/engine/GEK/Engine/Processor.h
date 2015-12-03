#pragma once

#include <Windows.h>

namespace Gek
{
    DECLARE_INTERFACE_IID(ProcessorType, "3E2E4549-FF48-46B7-80D9-F8D5BCA8EF4C");

    DECLARE_INTERFACE_IID(Processor, "EF25BBFD-7125-45DC-8B08-588B9A4C2278") : virtual public IUnknown
    {
        STDMETHOD(initialize)                       (THIS_ IUnknown *initializerContext) PURE;
    };
}; // namespace Gek
