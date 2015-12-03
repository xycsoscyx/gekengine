#pragma once

#include "GEK\Math\Vector4.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    struct MassComponent
    {
        float value;

        MassComponent(void);
        HRESULT save(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
        HRESULT load(const std::unordered_map<CStringW, CStringW> &componentParameterList);

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
