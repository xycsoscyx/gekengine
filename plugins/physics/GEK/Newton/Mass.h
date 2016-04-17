#pragma once

#include "GEK\Math\Vector4.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    struct MassComponent
    {
        float value;

        MassComponent(void);
        HRESULT save(Population::ComponentDefinition &componentData) const;
        HRESULT load(const Population::ComponentDefinition &componentData);

        inline operator float&()
        {
            return value;
        }

        inline operator const float&() const
        {
            return value;
        }


        inline float &operator = (float value)
        {
            this->value = value;
            return this->value;
        }
    };
}; // namespace Gek
