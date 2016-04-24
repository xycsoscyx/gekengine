#pragma once

#include "GEK\Math\Float2.h"
#include "GEK\Math\Float4.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    struct ParticlesComponent
    {
        CStringW material;
        CStringW colorMap;
        CStringW sizeMap;
        Math::Float2 lifeExpectancy;
        UINT32 density;

        ParticlesComponent(void);
        HRESULT save(Population::ComponentDefinition &componentData) const;
        HRESULT load(const Population::ComponentDefinition &componentData);
    };
}; // namespace Gek
