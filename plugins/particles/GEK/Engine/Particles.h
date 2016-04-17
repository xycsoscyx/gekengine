#pragma once

#include "GEK\Math\Vector4.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    struct ParticlesComponent
    {
        ParticlesComponent(void);
        HRESULT save(Population::ComponentDefinition &componentData) const;
        HRESULT load(const Population::ComponentDefinition &componentData);
    };
}; // namespace Gek
