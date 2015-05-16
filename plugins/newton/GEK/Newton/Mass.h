#pragma once

#include "GEK\Math\Vector4.h"
#include "GEK\Engine\ComponentInterface.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    namespace Components
    {
        namespace Mass
        {
            static const Handle identifier = 12;
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
    }; // namespace Components
}; // namespace Gek
