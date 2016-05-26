#pragma once

#include "GEK\Math\Float4.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    struct RigidBodyComponent
    {
        wstring shape;
        wstring surface;

        RigidBodyComponent(void);
        HRESULT save(Population::ComponentDefinition &componentData) const;
        HRESULT load(const Population::ComponentDefinition &componentData);
    };
}; // namespace Gek
