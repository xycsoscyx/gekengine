#pragma once

#include "GEK\Math\Vector4.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    namespace Components
    {
        struct Color
        {
            Gek::Math::Float4 value;

            Color(void);
            HRESULT getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
            HRESULT setData(const std::unordered_map<CStringW, CStringW> &componentParameterList);

            inline operator Gek::Math::Float4&()
            {
                return value;
            }

            inline operator const Gek::Math::Float4&() const
            {
                return value;
            }

            inline Gek::Math::Float4 &operator = (const Gek::Math::Float4 &value)
            {
                this->value = value;
                return this->value;
            }
        };
    }; // namespace Components
}; // namespace Gek
