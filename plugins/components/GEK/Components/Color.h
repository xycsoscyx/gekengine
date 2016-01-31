#pragma once

#include "GEK\Math\Color.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    struct ColorComponent
    {
        Gek::Math::Color value;

        ColorComponent(void);
        HRESULT save(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
        HRESULT load(const std::unordered_map<CStringW, CStringW> &componentParameterList);

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
