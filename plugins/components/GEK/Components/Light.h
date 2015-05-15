#pragma once

#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    namespace Components
    {
        struct Light
        {
            float radius;

            Light(void);
            HRESULT getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
            HRESULT setData(const std::unordered_map<CStringW, CStringW> &componentParameterList);
        };
    }; // namespace Components
}; // namespace Gek
