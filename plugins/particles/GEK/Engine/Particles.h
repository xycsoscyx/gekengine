#pragma once

#include "GEK\Math\Vector4.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    struct ParticlesComponent
    {
        CStringW material;
        CStringW colorMap;
        CStringW sizeMap;
        CStringW lifeExpectancy;
        UINT32 size;

        ParticlesComponent(void);
        HRESULT save(Population::ComponentDefinition &componentData) const;
        HRESULT load(const Population::ComponentDefinition &componentData);
    };
}; // namespace Gek
