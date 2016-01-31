#pragma once

#include "GEK\Engine\Resources.h"

namespace Gek
{
    DECLARE_INTERFACE_IID(Material, "57D5B374-A559-44D0-B017-82034A136C16") : virtual public IUnknown
    {
        STDMETHOD(initialize)                                           (THIS_ IUnknown *initializerContext, LPCWSTR fileName) PURE;

        STDMETHOD_(ShaderHandle, getShader)                             (THIS) PURE;
        STDMETHOD_(const std::list<ResourceHandle>, getResourceList)    (THIS) PURE;
    };

    DECLARE_INTERFACE_IID(MaterialRegistration, "0E7141AC-0CC5-4F5F-B96E-F10ED4155471");
}; // namespace Gek
