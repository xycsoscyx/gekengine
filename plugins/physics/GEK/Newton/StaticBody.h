#pragma once

#include "GEK\Math\Vector4.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    struct StaticBodyComponent
    {
        CStringW shape;

        StaticBodyComponent(void);
        HRESULT save(Population::ComponentDefinition &componentData) const;
        HRESULT load(const Population::ComponentDefinition &componentData);
    };
}; // namespace Gek
