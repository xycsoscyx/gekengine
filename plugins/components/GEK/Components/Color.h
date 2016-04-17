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

        inline operator Gek::Math::Color&()
        {
            return value;
        }

        inline operator const Gek::Math::Color&() const
        {
            return value;
        }

        inline Gek::Math::Color &operator = (const Gek::Math::Color &value)
        {
            this->value = value;
            return this->value;
        }
    };
}; // namespace Gek
