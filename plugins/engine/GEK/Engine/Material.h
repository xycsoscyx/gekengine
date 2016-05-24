#pragma once

#include "GEK\Engine\Resources.h"

namespace Gek
{
    GEK_INTERFACE(Material)
    {
        virtual void initialize(IUnknown *initializerContext, const wchar_t *fileName) = 0;

        virtual ShaderHandle getShader(void) const = 0;
        virtual const std::list<ResourceHandle> getResourceList(void) const = 0;
    };
}; // namespace Gek
