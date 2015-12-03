#pragma once

#include "GEK\Context\Observer.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Shape\Frustum.h"

namespace Gek
{
    DECLARE_INTERFACE(Shader);

    DECLARE_INTERFACE_IID(Material, "57D5B374-A559-44D0-B017-82034A136C16") : virtual public IUnknown
    {
        STDMETHOD(initialize)                       (THIS_ IUnknown *initializerContext, LPCWSTR fileName) PURE;
        STDMETHOD_(Shader *, getShader)             (THIS) PURE;

        STDMETHOD_(void, enable)                    (THIS_ VideoContext *context, LPCVOID passData) PURE;
    };

    DECLARE_INTERFACE_IID(MaterialRegistration, "0E7141AC-0CC5-4F5F-B96E-F10ED4155471");
}; // namespace Gek
