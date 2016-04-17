#pragma once

#include "GEK\Math\Vector4.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    struct RigidBodyComponent
    {
        CStringW shape;
        CStringW surface;

        RigidBodyComponent(void);
        HRESULT save(Population::ComponentDefinition &componentData) const;
        HRESULT load(const Population::ComponentDefinition &componentData);
    };
}; // namespace Gek
