#pragma once

#include "GEK\Math\Color.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    struct ColorComponent
    {
        Gek::Math::Color value;

        ColorComponent(void);
        HRESULT save(Population::ComponentDefinition &componentData) const;
        HRESULT load(const Population::ComponentDefinition &componentData);
    };
}; // namespace Gek
