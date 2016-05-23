#pragma once

#include "GEK\Engine\Resources.h"

namespace Gek
{
    DECLARE_INTERFACE_IID(Material, "57D5B374-A559-44D0-B017-82034A136C16") : virtual public IUnknown
    {
        void initialize(IUnknown *initializerContext, LPCWSTR fileName);

        ShaderHandle getShader(void) const;
        const std::list<ResourceHandle> getResourceList(void) const;
    };

    DECLARE_INTERFACE_IID(MaterialRegistration, "0E7141AC-0CC5-4F5F-B96E-F10ED4155471");
}; // namespace Gek
