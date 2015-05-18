#pragma once

#include "GEK\Engine\EngineInterface.h"

namespace Gek
{
    namespace Engine
    {
        namespace System
        {
            DECLARE_INTERFACE_IID(Type, "3E2E4549-FF48-46B7-80D9-F8D5BCA8EF4C");

            DECLARE_INTERFACE_IID_(Interface, IUnknown, "EF25BBFD-7125-45DC-8B08-588B9A4C2278")
            {
                STDMETHOD(initialize)                       (THIS_ Engine::Initializer *engine) PURE;
            };
        };
    };
}; // namespace Gek
