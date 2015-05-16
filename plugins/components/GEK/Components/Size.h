#pragma once

#include "GEK\Math\Vector3.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    namespace Components
    {
        namespace Size
        {
            static const void *Identifier = nullptr;
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
        }; // namespace Size
    }; // namespace Components
}; // namespace Gek
