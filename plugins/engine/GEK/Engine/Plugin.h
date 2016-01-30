#pragma once

#include "GEK\Context\Observer.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Shapes\Frustum.h"

namespace Gek
{
    DECLARE_INTERFACE_IID(Plugin, "F025D5AE-BE84-4DF7-90C0-0E241EABB56B") : virtual public IUnknown
    {
        STDMETHOD(initialize)                       (THIS_ IUnknown *initializerContext, LPCWSTR fileName) PURE;

        STDMETHOD_(void, enable)                    (THIS_ VideoContext *context) PURE;
    };

    DECLARE_INTERFACE_IID(PluginRegistration, "379FF0E3-85F2-4138-A6E4-BCA93241D4F0");
}; // namespace Gek
