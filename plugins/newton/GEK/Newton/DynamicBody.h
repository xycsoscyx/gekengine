#pragma once

#include "GEK\Math\Vector4.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    namespace Newton
    {
        namespace DynamicBody
        {
            struct Data
            {
                CStringW shape;
                CStringW surface;

                Data(void);
                HRESULT save(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
                HRESULT load(const std::unordered_map<CStringW, CStringW> &componentParameterList);
            };
        }; // namespace DynamicBody
    }; // namespace Newton
}; // namespace Gek
