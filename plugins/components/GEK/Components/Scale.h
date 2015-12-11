#pragma once

#include "GEK\Math\Vector3.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    struct ScaleComponent
    {
        Math::Float3 value;

        ScaleComponent(void);
        HRESULT save(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
        HRESULT load(const std::unordered_map<CStringW, CStringW> &componentParameterList);

        inline operator Math::Float3&()
        {
            return value;
        }

        inline operator const Math::Float3&() const
        {
            return value;
        }


        inline Math::Float3 &operator = (Math::Float3 value)
        {
            this->value = value;
            return this->value;
        }
    };
}; // namespace Gek
