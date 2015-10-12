#pragma once

#include "GEK\Math\Vector3.h"
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
                struct Data
                {
                    float value;

                    Data(void);
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
            }; // namespace Size
        }; // namespace Components
    }; // namespace Engine
}; // namespace Gek
