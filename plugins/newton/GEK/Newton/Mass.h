#pragma once

#include "GEK\Math\Vector4.h"
#include "GEK\Utility\Common.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    namespace Newton
    {
        namespace Mass
        {
            static const Handle identifier = 0x00000101;
            struct Data
            {
                float value;

                Data(void);
                HRESULT getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
                HRESULT setData(const std::unordered_map<CStringW, CStringW> &componentParameterList);

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
        }; // namespace Mass
    }; // namespace Newton
}; // namespace Gek
