#pragma once

#include "GEK\Math\Float2.h"
#include "GEK\Math\Float4.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    struct ParticlesComponent
    {
        wstring material;
        wstring colorMap;
        Math::Float2 lifeExpectancy;
        Math::Float2 size;
        UINT32 density;

        ParticlesComponent(void);
        void save(Population::ComponentDefinition &componentData) const;
        void load(const Population::ComponentDefinition &componentData);
    };
}; // namespace Gek
