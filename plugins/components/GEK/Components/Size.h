#pragma once

#include "GEK\Math\Vector3.h"
#include "GEK\Utility\Common.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    namespace Engine
    {
        namespace Components
        {
            namespace Size
            {
                static const Handle identifier = 0x00000005;
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
    }; // namespace Engine
}; // namespace Gek
