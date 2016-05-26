#pragma once

#include "GEK\Math\Float4.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    struct StaticBodyComponent
    {
        wstring shape;

        StaticBodyComponent(void);
        HRESULT save(Population::ComponentDefinition &componentData) const;
        HRESULT load(const Population::ComponentDefinition &componentData);
    };
}; // namespace Gek
