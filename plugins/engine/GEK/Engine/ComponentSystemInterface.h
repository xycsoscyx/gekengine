#pragma once

#include "GEK\Engine\PopulationInterface.h"

namespace Gek
{
    // {3E2E4549-FF48-46B7-80D9-F8D5BCA8EF4C}
    DEFINE_GUID(ComponentSystemType, 0x3e2e4549, 0xff48, 0x46b7, 0x80, 0xd9, 0xf8, 0xd5, 0xbc, 0xa8, 0xef, 0x4c);

    DECLARE_INTERFACE_IID_(ComponentSystemInterface, IUnknown, "EF25BBFD-7125-45DC-8B08-588B9A4C2278")
    {
        STDMETHOD(initialize)                       (THIS_ Population::SystemInterface *populationSystem) PURE;
    };
}; // namespace Gek
